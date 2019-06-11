#include <cstring>
#include <map>
#include <set>
#include <zlib.h>
#include "HicReader.hh"
#include "RemoteSeekableStream.hh"
#include "FileSeekableStream.hh"
#include "MemorySeekableStream.hh"
#include "Debug.hh"
using namespace std;

struct QueryInterval {
	Chromosome* chromosome;
	int32_t start;
	int32_t end;
	int32_t tid() const { return chromosome->index;}
	};

// pointer structure for reading blocks or matrices, holds the size and position 
struct indexEntry {
  int size;
  long position;
};

class HicQuery
	{
	public:
		QueryInterval interval1;
		QueryInterval interval2;
		std::string norm;
		std::string unit;
		int32_t resolution;
		//
		int64_t myFilePos;	
		indexEntry indexEntry1;
		indexEntry indexEntry2;
		
		vector<double> c1Norm;
  		vector<double> c2Norm;


		  int32_t blockBinCount;
	 	  int32_t blockColumnCount;

		map<int32_t,indexEntry> blockMap;
		set<int> blocksSet;
	};


static void readNormalizationVector(SeekableStream* fin, indexEntry* entry,vector<double>& norm) {
      if( entry->position <= 0L) return;
      fin->seek(entry->position);
      char* buffer = new char[entry->size];
      fin->readFully(buffer, entry->size);
      MemorySeekableStream bufferin(buffer,entry->size);

	int32_t nValues= bufferin.readInt();
	norm.reserve(nValues);
	  for (int32_t i = 0; i < nValues; i++) {
	    double d = bufferin.readDouble();
	    norm.push_back(d);
	  }
       delete[] buffer;
       }

static bool readMatrixZoomData(SeekableStream* fin, HicQuery* q) {
  string unit = fin->readString();
  fin->readInt();//tmp

  fin->readFloat(); // sumCounts
  fin->readFloat(); // occupiedCellCount
  fin->readFloat(); // stdDev
  fin->readFloat(); // percent95
  int32_t binSize = fin->readInt();
  int32_t blockBinCount = fin->readInt();
  int32_t blockColumnCount = fin->readInt();;
  
  bool storeBlockData = false;
  if (q->unit==unit && q->resolution==binSize) {
    q->blockBinCount = blockBinCount;
    q->blockColumnCount = blockColumnCount;
    storeBlockData = true;
  }
  
  int32_t nBlocks = fin->readInt();


  for (int32_t b = 0; b < nBlocks; b++) {
    int32_t blockNumber = fin->readInt();
    int64_t filePosition = fin->readLong();
    int32_t blockSizeInBytes = fin->readInt();
    indexEntry entry;
    entry.size = blockSizeInBytes;
    entry.position = filePosition;
    if (storeBlockData) q->blockMap[blockNumber] = entry;
  }
  return storeBlockData;
}


static bool readMatrix(SeekableStream* fin, HicQuery* q)
	{
	 fin->seek(q->myFilePos);
	 fin->readInt();//c1
	 fin->readInt();//c2
	 int32_t nRes = fin->readInt();
	 int32_t i=0;
	 bool found=false;
	 while (i<nRes && !found) {
	    found = readMatrixZoomData(fin, q);
	    i++;
	    }
        return found;
	}

// gets the blocks that need to be read for this slice of the data.  needs blockbincount, blockcolumncount, and whether
// or not this is intrachromosomal.
static void getBlockNumbersForRegionFromBinPosition(HicQuery* q) {
   int32_t col1 = q->interval1.start / q->blockBinCount;
   int32_t col2 = ( q->interval1.end + 1) / q->blockBinCount;
   int32_t row1 = q->interval2.start / q->blockBinCount;
   int32_t row2 = (q->interval2.end + 1) / q->blockBinCount;
   
   // first check the upper triangular matrix
   for (int32_t r = row1; r <= row2; r++) {
     for (int32_t c = col1; c <= col2; c++) {
       int32_t blockNumber = r * q->blockColumnCount + c;
       q->blocksSet.insert(blockNumber);
     }
   }
   // check region part that overlaps with lower left triangle
   // but only if intrachromosomal
   if ( q->interval1.tid() ==  q->interval2.tid()) {
     for (int32_t r = col1; r <= col2; r++) {
       for (int32_t c = row1; c <= row2; c++) {
	 int32_t blockNumber = r * q->blockColumnCount + c;
	 q->blocksSet.insert(blockNumber);
       }
     }
   }
}


// this is the meat of reading the data.  takes in the block number and returns the set of contact records corresponding to
// that block.  the block data is compressed and must be decompressed using the zlib library functions
void readBlock(SeekableStream* fin,HicQuery* q, int blockNumber,int version) {
  auto iter = q->blockMap.find(blockNumber);
  if(iter == q->blockMap.end()) return;
  indexEntry idx = iter->second;
  if (idx.size == 0) {
    return;
    }

  char* compressedBytes = new char[idx.size];
  char* uncompressedBytes = new char[idx.size*10]; //biggest seen so far is 3

  
  fin->seek(idx.position);
  fin->readFully(compressedBytes, idx.size);
  
  // Decompress the block
  // zlib struct
  z_stream infstream;
  infstream.zalloc = Z_NULL;
  infstream.zfree = Z_NULL;
  infstream.opaque = Z_NULL;
  infstream.avail_in = (uInt)(idx.size); // size of input
  infstream.next_in = (Bytef *)compressedBytes; // input char array
  infstream.avail_out = (uInt)idx.size*10; // size of output
  infstream.next_out = (Bytef *)uncompressedBytes; // output char array
  // the actual decompression work.
  inflateInit(&infstream);
  inflate(&infstream, Z_NO_FLUSH);
  inflateEnd(&infstream);
  int uncompressedSize=infstream.total_out;

  // create stream from buffer for ease of use
  MemorySeekableStream bufferin(uncompressedBytes,uncompressedSize);

  int32_t nRecords = bufferin.readInt();

  // different versions have different specific formats
  if (version < 7) {
    for (int32_t i = 0; i < nRecords; i++) {
      int32_t binX = bufferin.readInt();
      int32_t binY = bufferin.readInt();
      float counts = bufferin.readFloat();
       /*
      contactRecord record;
      record.binX = binX;
      record.binY = binY;
      record.counts = counts;
      v[i] = record;*/
    }
  } 
  else {
    int32_t binXOffset = bufferin.readInt();
    int32_t binYOffset = bufferin.readInt();

    char useShort = bufferin.readChar();
    char type= bufferin.readChar();
    int index=0;
    if (type == 1) {
      // List-of-rows representation
      int16_t rowCount = bufferin.readShort();

      for (int32_t i = 0; i < rowCount; i++) {
	short y  = bufferin.readShort();
	int binY = y + binYOffset;
	short colCount  = bufferin.readShort();

	for (int j = 0; j < colCount; j++) {
	  short x = bufferin.readShort();
	  int binX = binXOffset + x;
	  float counts;
	  if (useShort == 0) { // yes this is opposite of usual
	    short c = bufferin.readShort();
	    counts = c;
	  } 
	  else {
	    counts = bufferin.readFloat();
	  }
		/*
	  contactRecord record;
	  record.binX = binX;
	  record.binY = binY;
	  record.counts = counts;
	  v[index]=record;
	  index++;*/
	}
      }
    }
    else if (type == 2) { // have yet to find test file where this is true, possibly entirely deprecated
      int32_t nPts = bufferin.readInt(); 
      int32_t w =bufferin.readInt(); 

      for (int32_t i = 0; i < nPts; i++) {
	//int idx = (p.y - binOffset2) * w + (p.x - binOffset1);
	int row = i / w;
	int col = i - row * w;
	int bin1 = binXOffset + col;
	int bin2 = binYOffset + row;

	float counts;
	if (useShort == 0) { // yes this is opposite of the usual
	  short c = bufferin.readShort();
	  if (c != -32768) {
		/*
	    contactRecord record;
	    record.binX = bin1;
	    record.binY = bin2;
	    record.counts = c;
	    v[index]=record;
	    index++;*/
	  }
	} 
	else {
	 counts =  bufferin.readFloat();
	  if (counts != 0x7fc00000) { // not sure this works
	    //	  if (!Float.isNaN(counts)) {
	    /*contactRecord record;
	    record.binX = bin1;
	    record.binY = bin2;
	    record.counts = counts;
	    v[index]=record;
	    index++;*/
	  }
	}
      }
    }
  }
  delete compressedBytes;
  delete uncompressedBytes; // don't forget to delete your heap arrays in C++!
}

HicReader::HicReader(const char* s):source(s) {
	if(strncmp(s,"http://",7)==0 || strncmp(s,"https://",8)==0) {
		fin = new RemoteSeekableStream(s);
		}
	else
		{
		fin = new FileSeekableStream(s);
		}
	char magic[4];
	fin->readFully(magic,4);
	if(!(magic[0]=='H' && magic[1]=='I' &&  magic[2]=='C' &&  magic[3]==0)) {
		THROW_ERROR("Cannot read magic for HIC file " << this->source);
		}

	this->version = fin->readInt();
	if(this->version< 6) THROW_ERROR("Unsupported version : " << this->version);

	this->master = fin->readLong();
  
  	this->build = fin->readString();

  
    int nattributes = fin->readInt();
  
    for (int i=0; i<nattributes; i++) {
    	string key = fin->readString();
    	string value = fin->readString();
  		this->attributes.insert(make_pair(key,value));
  		}
  	
  	
  int nChrs = fin->readInt();
  for (int i=0; i<nChrs; i++) {
	  	Chromosome* contig = new Chromosome;
		contig->name = fin->readString();
		contig->length = fin->readInt();
		contig->index = i;
		chromosomes.push_back(contig);
		name2chrom.insert(make_pair(contig->name,contig)); 
		}
  } 

HicReader::~HicReader() {
	if(fin!=0) delete fin;
	for(size_t i=0;i< chromosomes.size();i++) delete chromosomes[i];
	}

Chromosome* HicReader::find_chromosome_by_name(const string s) const {
	std::map<std::string,Chromosome*>::const_iterator r = name2chrom.find(s);
	return r==name2chrom.end()?NULL:(Chromosome*)r->second;
	}

bool HicReader::parseInterval(string s,void* intervalptr) const {
	QueryInterval* interval = (QueryInterval*)intervalptr;
	std::string::size_type colon = s.find(':');
	if(colon == string::npos) { //whole chrom
		interval->chromosome = find_chromosome_by_name(s);
		if( interval->chromosome == NULL) {
			DEBUG("unknown chromosome in " << s);
			return false;
			}
		interval->start = 0;
		interval->end = interval->chromosome->length;
		return true;
		}
	else
		{
		string chrom = s.substr(0,colon);
		interval->chromosome = find_chromosome_by_name(s);
		if( interval->chromosome == NULL) {
			DEBUG("unknown chromosome in " << s);
			return false;
			}
		
		std::string::size_type hyphen = s.find('-',colon+1);
		if(hyphen == string::npos) hyphen = s.find(':',colon+1);
		if(hyphen == string::npos) {
			DEBUG("bad interval " << s);
			return false;
			}
		interval->start = atoi(s.substr(colon+1,(hyphen-colon)).c_str());
		interval->end = atoi(s.substr(hyphen+1).c_str());
		if(interval->start <0 || interval->end < 0 || interval->start >= interval->end) {
			DEBUG("bad interval " << s);
			return false;
			}
		return true;
		}
	}

bool HicReader::query(const char* interval1,const char* interval2,norm_t norm,unit_t unit,resolution_t resolution,query_callback_t) {
	if(interval1==NULL) return false;
	if(interval2==NULL) return false;	
	HicQuery q;
	q.norm = norm;
	q.unit = unit;
	q.resolution = resolution;

	 if(!this->parseInterval(interval1,&(q.interval1))) return false;
	 if(!this->parseInterval(interval2,&(q.interval2))) return false;

	 if(q.interval1.tid()< q.interval2.tid() || 
		( q.interval1.tid() == q.interval2.tid() && q.interval1.start < q.interval2.start)
		) {
	 	swap(q.interval1,q.interval2);
	 	}
	if (!queryFooter((void*)&q)) return false;


	


  	if (norm != "NONE") {
		readNormalizationVector(this->fin,&(q.indexEntry1),q.c1Norm);
	        readNormalizationVector(this->fin,&(q.indexEntry2),q.c2Norm);
		}
	if (!readMatrix(this->fin,&q)) return false;

	getBlockNumbersForRegionFromBinPosition(&q);
	return true;
	}


// reads the footer from the master pointer location. takes in the chromosomes,
// norm, unit (BP or FRAG) and resolution or binsize, and sets the file 
// position of the matrix and the normalization vectors for those chromosomes 
// at the given normalization and resolution
bool HicReader::queryFooter(void* qptr) {
  HicQuery* q = (HicQuery*)qptr;
  q->myFilePos = -1L;

  fin->seek(master);
  fin->readInt();//nBytes

  
  int32_t nEntries = fin->readInt();


  for (int i=0; i<nEntries; i++) {
    string str = fin->readString();
    string::size_type u = str.find('_');
    if(u==string::npos) THROW_ERROR("no '_' in " << str);
    int32_t tid1 = std::stoi(str.substr(0,u));
    int32_t tid2 = std::stoi(str.substr(u+1));

    int64_t fpos  = fin->readLong();
    fin->readInt(); // sizeinbytes 
    if (tid1 == q->interval1.tid() && tid2 == q->interval2.tid()) {
      q->myFilePos = fpos;
    }
  }
  if (q->myFilePos <= 0L) return false;

  if (q->norm=="NONE") return true; // no need to read norm vector index
 
  // read in and ignore expected value maps; don't store; reading these to 
  // get to norm vector index
  int32_t nExpectedValues = fin->readInt();

  for (int i=0; i<nExpectedValues; i++) {
   fin->readString();//str
   fin->readInt();//binSize

    int32_t nValues = fin->readInt();
    for (int32_t j=0; j<nValues; j++) {
     fin->readDouble();//v
    }

    int32_t nNormalizationFactors = fin->readInt();
    for (int32_t j=0; j<nNormalizationFactors; j++) {
      fin->readInt();// tid
      fin->readDouble();//v
    }
  }
  
  nExpectedValues = fin->readInt();
  for (int32_t i=0; i<nExpectedValues; i++) {
    fin->readString(); //typeString
    fin->readString(); //unit
    fin->readInt(); //binSize
    int32_t nValues = fin->readInt();
    for (int j=0; j<nValues; j++) {
       fin->readDouble();//v
    }
    int32_t nNormalizationFactors = fin->readInt();
    for (int32_t j=0; j<nNormalizationFactors; j++) {
      fin->readInt();//chrIdx
      fin->readDouble();//v
    }
  }
  // Index of normalization vectors
  nEntries = fin->readInt();
  q->indexEntry1.position = -1L;
  q->indexEntry2.position = -1L;

  

  for (int32_t i = 0; i < nEntries; i++) {
    string normtype = fin->readString();
    int32_t chrIdx = fin->readInt();
    string unit1 = fin->readString();
    int32_t resolution1 = fin->readInt();
    int64_t filePosition = fin->readLong();
    int sizeInBytes = fin->readInt();
    if (chrIdx == q->interval1.tid() && normtype == q->norm && unit1 == q->unit && resolution1 == q->resolution) {
      q->indexEntry1.position=filePosition;
      q->indexEntry1.size=sizeInBytes;
    }
    if (chrIdx == q->interval2.tid() && normtype == q->norm && unit1 == q->unit && resolution1 == q->resolution) {
      q->indexEntry2.position=filePosition;
      q->indexEntry2.size=sizeInBytes;
    }
  }
  if (q->indexEntry1.position <= 0L || q->indexEntry2.position <= 0L) {
    return false;
  }

return true;
}



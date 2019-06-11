#include <cstring>
#include "HicReader.hh"
#include "RemoteSeekableStream.hh"
#include "FileSeekableStream.hh"
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
	};

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



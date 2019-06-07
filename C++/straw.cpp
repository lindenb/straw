/*
  The MIT License (MIT)
 
  Copyright (c) 2011-2016 Broad Institute, Aiden Lab
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
*/
#include <cstring>
#include <iostream>
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>
#include <set>
#include <vector>
#include <streambuf>

#include "zlib.h"
#include "straw.h"
#include "RemoteSeekableStream.hh"
#include "FileSeekableStream.hh"
using namespace std;

/*
  Straw: fast C++ implementation of dump. Not as fully featured as the
  Java version. Reads the .hic file, finds the appropriate matrix and slice
  of data, and outputs as text in sparse upper triangular format.

  Currently only supporting matrices.

  Usage: straw <NONE/VC/VC_SQRT/KR> <hicFile(s)> <chr1>[:x1:x2] <chr2>[:y1:y2] <BP/FRAG> <binsize> 
 */
// this is for creating a stream from a byte array for ease of use

// version number
int version;

// map of block numbers to pointers
map <int, indexEntry> blockMap;

long total_bytes;


void getline(SeekableStream* fin,string& key,int ) {
key=fin->readString();
}

// returns whether or not this is valid HiC file
bool readMagicString(SeekableStream* fin) {
  string str= fin->readString();
  return str[0]=='H' && str[1]=='I' && str[2]=='C';
}

// reads the header, storing the positions of the normalization vectors and returning the master pointer
long readHeader(SeekableStream* fin, string chr1, string chr2, int &c1pos1, int &c1pos2, int &c2pos1, int &c2pos2, int &chr1ind, int &chr2ind) {
  if (!readMagicString(fin)) {
    cerr << "Hi-C magic string is missing, does not appear to be a hic file" << endl;
    exit(1);
  }

  version = fin->readInt();
  if (version < 6) {
    cerr << "Version " << version << " no longer supported" << endl;
    exit(1);
  }
  long master = fin->readLong();

  string genome = fin->readString();
  int nattributes = fin->readInt();

  // reading and ignoring attribute-value dictionary
  for (int i=0; i<nattributes; i++) {
    string key = fin->readString();
    string value = fin->readString();
  }
  int nChrs = fin->readInt();
  
  // chromosome map for finding matrix
  bool found1 = false;
  bool found2 = false;
  for (int i=0; i<nChrs; i++) {
    string name = fin->readString();
    int length = fin->readInt();
    
    if (name==chr1) {
      found1=true;
      chr1ind = i;
      if (c1pos1 == -100) {
	c1pos1 = 0;
	c1pos2 = length;
      }
    }
    if (name==chr2) {
      found2=true;
      chr2ind = i;
      if (c2pos1 == -100) {
	c2pos1 = 0;
	c2pos2 = length;
      }
    }
  }
  if (!found1 || !found2) {
    cerr << "One of the chromosomes wasn't found in the file. Check that the chromosome name matches the genome." << endl;
    exit(1);
  }
  return master;
}

// reads the footer from the master pointer location. takes in the chromosomes,
// norm, unit (BP or FRAG) and resolution or binsize, and sets the file 
// position of the matrix and the normalization vectors for those chromosomes 
// at the given normalization and resolution
void readFooter(SeekableStream* fin, long master, int c1, int c2, string norm, string unit, int resolution, long &myFilePos, indexEntry &c1NormEntry, indexEntry &c2NormEntry) {
  int nBytes  = fin->readInt();
  stringstream ss;
  ss << c1 << "_" << c2;
  string key = ss.str();
  
  int nEntries = fin->readInt();
  bool found = false;
  for (int i=0; i<nEntries; i++) {
    string str = fin->readString();
    long fpos  = fin->readLong();
    int sizeinbytes  = fin->readInt();
    if (str == key) {
      myFilePos = fpos;
      found=true;
    }
  }
  if (!found) {
    cerr << "File doesn't have the given chr_chr map" << endl;
    exit(1);
  }

  if (norm=="NONE") return; // no need to read norm vector index
 
  // read in and ignore expected value maps; don't store; reading these to 
  // get to norm vector index
  int nExpectedValues;
  fin->read((char*)&nExpectedValues, sizeof(int));
  for (int i=0; i<nExpectedValues; i++) {
    string str = fin->readString();
    int binSize = fin->readInt();

    int nValues = fin->readInt();
    for (int j=0; j<nValues; j++) {
      double v = fin->readDouble();
    }

    int nNormalizationFactors;
    fin->read((char*)&nNormalizationFactors, sizeof(int));
    for (int j=0; j<nNormalizationFactors; j++) {
      int chrIdx = fin->readInt();
      double v= fin->readDouble();
    }
  }
  fin->read((char*)&nExpectedValues, sizeof(int));
  for (int i=0; i<nExpectedValues; i++) {
    string str;
    getline(fin, str, '\0'); //typeString
    getline(fin, str, '\0'); //unit
    int binSize;
    fin->read((char*)&binSize, sizeof(int));

    int nValues;
    fin->read((char*)&nValues, sizeof(int));
    for (int j=0; j<nValues; j++) {
      double v;
      fin->read((char*)&v, sizeof(double));
    }
    int nNormalizationFactors;
    fin->read((char*)&nNormalizationFactors, sizeof(int));
    for (int j=0; j<nNormalizationFactors; j++) {
      int chrIdx;
      fin->read((char*)&chrIdx, sizeof(int));
      double v;
      fin->read((char*)&v, sizeof(double));
    }
  }
  // Index of normalization vectors
  fin->read((char*)&nEntries, sizeof(int));
  bool found1 = false;
  bool found2 = false;
  for (int i = 0; i < nEntries; i++) {
    string normtype;
    getline(fin, normtype, '\0'); //normalization type
    int chrIdx;
    fin->read((char*)&chrIdx, sizeof(int));
    string unit1;
    getline(fin, unit1, '\0'); //unit
    int resolution1;
    fin->read((char*)&resolution1, sizeof(int));
    long filePosition;
    fin->read((char*)&filePosition, sizeof(long));
    int sizeInBytes;
    fin->read((char*)&sizeInBytes, sizeof(int));
    if (chrIdx == c1 && normtype == norm && unit1 == unit && resolution1 == resolution) {
      c1NormEntry.position=filePosition;
      c1NormEntry.size=sizeInBytes;
      found1 = true;
    }
    if (chrIdx == c2 && normtype == norm && unit1 == unit && resolution1 == resolution) {
      c2NormEntry.position=filePosition;
      c2NormEntry.size=sizeInBytes;
      found2 = true;
    }
  }
  if (!found1 || !found2) {
    cerr << "File did not contain " << norm << " normalization vectors for one or both chromosomes at " << resolution << " " << unit << endl;
    exit(1);
  }
}

// reads the raw binned contact matrix at specified resolution, setting the block bin count and block column count 
bool readMatrixZoomData(SeekableStream* fin, string myunit, int mybinsize, int &myBlockBinCount, int &myBlockColumnCount) {
  string unit;
  getline(fin, unit, '\0' ); // unit
  int tmp;
  fin->read((char*)&tmp, sizeof(int)); // Old "zoom" index -- not used
  float tmp2;
  fin->read((char*)&tmp2, sizeof(float)); // sumCounts
  fin->read((char*)&tmp2, sizeof(float)); // occupiedCellCount
  fin->read((char*)&tmp2, sizeof(float)); // stdDev
  fin->read((char*)&tmp2, sizeof(float)); // percent95
  int binSize;
  fin->read((char*)&binSize, sizeof(int));
  int blockBinCount;
  fin->read((char*)&blockBinCount, sizeof(int));
  int blockColumnCount;
  fin->read((char*)&blockColumnCount, sizeof(int));
  
  bool storeBlockData = false;
  if (myunit==unit && mybinsize==binSize) {
    myBlockBinCount = blockBinCount;
    myBlockColumnCount = blockColumnCount;
    storeBlockData = true;
  }
  
  int nBlocks;
  fin->read((char*)&nBlocks, sizeof(int));

  for (int b = 0; b < nBlocks; b++) {
    int blockNumber;
    fin->read((char*)&blockNumber, sizeof(int));
    long filePosition;
    fin->read((char*)&filePosition, sizeof(long));
    int blockSizeInBytes;
    fin->read((char*)&blockSizeInBytes, sizeof(int));
    indexEntry entry;
    entry.size = blockSizeInBytes;
    entry.position = filePosition;
    if (storeBlockData) blockMap[blockNumber] = entry;
  }
  return storeBlockData;
}

// goes to the specified file pointer and finds the raw contact matrix at specified resolution, calling readMatrixZoomData.
// sets blockbincount and blockcolumncount
void readMatrix(SeekableStream * fin, long myFilePosition, string unit, int resolution, int &myBlockBinCount, int &myBlockColumnCount) {
  fin->seek(myFilePosition);
  int c1,c2;
  fin->read((char*)&c1, sizeof(int)); //chr1
  fin->read((char*)&c2, sizeof(int)); //chr2
  int nRes;
  fin->read((char*)&nRes, sizeof(int));
  int i=0;
  bool found=false;
  while (i<nRes && !found) {
    found = readMatrixZoomData(fin, unit, resolution, myBlockBinCount, myBlockColumnCount);
    i++;
  }
  if (!found) {
    cerr << "Error finding block data" << endl;
    exit(1);
  }
}
// gets the blocks that need to be read for this slice of the data.  needs blockbincount, blockcolumncount, and whether
// or not this is intrachromosomal.
set<int> getBlockNumbersForRegionFromBinPosition(int* regionIndices, int blockBinCount, int blockColumnCount, bool intra) {
   int col1 = regionIndices[0] / blockBinCount;
   int col2 = (regionIndices[1] + 1) / blockBinCount;
   int row1 = regionIndices[2] / blockBinCount;
   int row2 = (regionIndices[3] + 1) / blockBinCount;
   
   set<int> blocksSet;
   // first check the upper triangular matrix
   for (int r = row1; r <= row2; r++) {
     for (int c = col1; c <= col2; c++) {
       int blockNumber = r * blockColumnCount + c;
       blocksSet.insert(blockNumber);
     }
   }
   // check region part that overlaps with lower left triangle
   // but only if intrachromosomal
   if (intra) {
     for (int r = col1; r <= col2; r++) {
       for (int c = row1; c <= row2; c++) {
	 int blockNumber = r * blockColumnCount + c;
	 blocksSet.insert(blockNumber);
       }
     }
   }

   return blocksSet;
}

struct membuf : std::streambuf
{
membuf(char* begin, char* end) {
    this->setg(begin, begin, end);
  }
};

// this is the meat of reading the data.  takes in the block number and returns the set of contact records corresponding to
// that block.  the block data is compressed and must be decompressed using the zlib library functions
vector<contactRecord> readBlock(SeekableStream* fin, int blockNumber) {
  indexEntry idx = blockMap[blockNumber];
  if (idx.size == 0) {
    vector<contactRecord> v;
    return v;
  }
  char* compressedBytes = new char[idx.size];
  char* uncompressedBytes = new char[idx.size*10]; //biggest seen so far is 3

    fin->seek(idx.position);
    fin->read(compressedBytes, idx.size);
  
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
  membuf sbuf(uncompressedBytes, uncompressedBytes + uncompressedSize);
  istream bufferin(&sbuf);
  int nRecords;
  bufferin.read((char*)&nRecords, sizeof(int));
  vector<contactRecord> v(nRecords);
  // different versions have different specific formats
  if (version < 7) {
    for (int i = 0; i < nRecords; i++) {
      int binX, binY;
      bufferin.read((char*)&binX, sizeof(int));
      bufferin.read((char*)&binY, sizeof(int));
      float counts;
      bufferin.read((char*)&counts, sizeof(float));
      contactRecord record;
      record.binX = binX;
      record.binY = binY;
      record.counts = counts;
      v[i] = record;
    }
  } 
  else {
    int binXOffset, binYOffset;
    bufferin.read((char*)&binXOffset, sizeof(int));
    bufferin.read((char*)&binYOffset, sizeof(int));
    char useShort;
    bufferin.read((char*)&useShort, sizeof(char));
    char type;
    bufferin.read((char*)&type, sizeof(char));
    int index=0;
    if (type == 1) {
      // List-of-rows representation
      short rowCount;
      bufferin.read((char*)&rowCount, sizeof(short));
      for (int i = 0; i < rowCount; i++) {
	short y;
	bufferin.read((char*)&y, sizeof(short));
	int binY = y + binYOffset;
	short colCount;
	bufferin.read((char*)&colCount, sizeof(short));
	for (int j = 0; j < colCount; j++) {
	  short x;
	  bufferin.read((char*)&x, sizeof(short));
	  int binX = binXOffset + x;
	  float counts;
	  if (useShort == 0) { // yes this is opposite of usual
	    short c;
	    bufferin.read((char*)&c, sizeof(short));
	    counts = c;
	  } 
	  else {
	    bufferin.read((char*)&counts, sizeof(float));
	  }
	  contactRecord record;
	  record.binX = binX;
	  record.binY = binY;
	  record.counts = counts;
	  v[index]=record;
	  index++;
	}
      }
    }
    else if (type == 2) { // have yet to find test file where this is true, possibly entirely deprecated
      int nPts;
      bufferin.read((char*)&nPts, sizeof(int));
      short w;
      bufferin.read((char*)&w, sizeof(short));

      for (int i = 0; i < nPts; i++) {
	//int idx = (p.y - binOffset2) * w + (p.x - binOffset1);
	int row = i / w;
	int col = i - row * w;
	int bin1 = binXOffset + col;
	int bin2 = binYOffset + row;

	float counts;
	if (useShort == 0) { // yes this is opposite of the usual
	  short c;
	  bufferin.read((char*)&c, sizeof(short));
	  if (c != -32768) {
	    contactRecord record;
	    record.binX = bin1;
	    record.binY = bin2;
	    record.counts = c;
	    v[index]=record;
	    index++;
	  }
	} 
	else {
	  bufferin.read((char*)&counts, sizeof(float));
	  if (counts != 0x7fc00000) { // not sure this works
	    //	  if (!Float.isNaN(counts)) {
	    contactRecord record;
	    record.binX = bin1;
	    record.binY = bin2;
	    record.counts = counts;
	    v[index]=record;
	    index++;
	  }
	}
      }
    }
  }
  delete compressedBytes;
  delete uncompressedBytes; // don't forget to delete your heap arrays in C++!
  return v;
}

// reads the normalization vector from the file at the specified location
vector<double> readNormalizationVector(istream& bufferin) {
  int nValues;
  bufferin.read((char*)&nValues, sizeof(int));
  vector<double> values(nValues);
  //  bool allNaN = true;

  for (int i = 0; i < nValues; i++) {
    double d;
    bufferin.read((char*)&d, sizeof(double));
    values[i] = d;
    /* if (!Double.isNaN(values[i])) {
      allNaN = false;
      }*/
  }
  //  if (allNaN) return null;
  return values;
}

int straw(string norm, string fname, int binsize, string chr1loc, string chr2loc, string unit, vector<XYCount> &results)
{

  if (!(norm=="NONE"||norm=="VC"||norm=="VC_SQRT"||norm=="KR")) {
    cerr << "Norm specified incorrectly, must be one of <NONE/VC/VC_SQRT/KR>" << endl; 
    cerr << "Usage: straw <NONE/VC/VC_SQRT/KR> <hicFile(s)> <chr1>[:x1:x2] <chr2>[:y1:y2] <BP/FRAG> <binsize>" << endl;
    return EXIT_FAILURE;
  }
  if (!(unit=="BP"||unit=="FRAG")) {
    cerr << "Norm specified incorrectly, must be one of <BP/FRAG>" << endl; 
    cerr << "Usage: straw <NONE/VC/VC_SQRT/KR> <hicFile(s)> <chr1>[:x1:x2] <chr2>[:y1:y2] <BP/FRAG> <binsize>" << endl;
    return EXIT_FAILURE;
  }

  // parse chromosome positions
  stringstream ss(chr1loc);
  string chr1, chr2, x, y;
  int c1pos1=-100, c1pos2=-100, c2pos1=-100, c2pos2=-100;
  getline(ss, chr1, ':');
  if (getline(ss, x, ':') && getline(ss, y, ':')) {
    c1pos1 = stoi(x);
    c1pos2 = stoi(y);
  }
  stringstream ss1(chr2loc);
  getline(ss1, chr2, ':');
  if (getline(ss1, x, ':') && getline(ss1, y, ':')) {
    c2pos1 = stoi(x);
    c2pos2 = stoi(y);
  }  
  int chr1ind, chr2ind;

  // HTTP code
  string prefix="http";
  SeekableStream* fin;

  // read header into buffer; 100K should be sufficient
  

  long master;
  if (std::strncmp(fname.c_str(), prefix.c_str(), prefix.size()) == 0) {
    fin= new RemoteSeekableStream(fname.c_str());

  }
  else {
    fin= new FileSeekableStream(fname.c_str());
  }
    master = readHeader(fin, chr1, chr2, c1pos1, c1pos2, c2pos1, c2pos2, chr1ind, chr2ind);
  // from header have size of chromosomes, set region to read
  int c1=min(chr1ind,chr2ind);
  int c2=max(chr1ind,chr2ind);
  int origRegionIndices[4]; // as given by user
  int regionIndices[4]; // used to find the blocks we need to access
  // reverse order if necessary
  if (chr1ind > chr2ind) {
    origRegionIndices[0] = c2pos1;
    origRegionIndices[1] = c2pos2;
    origRegionIndices[2] = c1pos1;
    origRegionIndices[3] = c1pos2;
    regionIndices[0] = c2pos1 / binsize;
    regionIndices[1] = c2pos2 / binsize;
    regionIndices[2] = c1pos1 / binsize;
    regionIndices[3] = c1pos2 / binsize;
  }
  else {
    origRegionIndices[0] = c1pos1;
    origRegionIndices[1] = c1pos2;
    origRegionIndices[2] = c2pos1;
    origRegionIndices[3] = c2pos2;
    regionIndices[0] = c1pos1 / binsize;
    regionIndices[1] = c1pos2 / binsize;
    regionIndices[2] = c2pos1 / binsize;
    regionIndices[3] = c2pos2 / binsize;
  }

  indexEntry c1NormEntry, c2NormEntry;
  long myFilePos;



    fin->seek(master);
    readFooter(fin, master, c1, c2, norm, unit, binsize, myFilePos, c1NormEntry, c2NormEntry); 
  
  // readFooter will assign the above variables


  vector<double> c1Norm;
  vector<double> c2Norm;

  if (norm != "NONE") {
    char* buffer3;
    
      buffer3 = new char[c1NormEntry.size];
      fin->seek(c1NormEntry.position);
      fin->readFully(buffer3, c1NormEntry.size);
    
    membuf sbuf3(buffer3, buffer3 + c1NormEntry.size);
    istream bufferin(&sbuf3);
    c1Norm = readNormalizationVector(bufferin);

    char* buffer4;
    
      buffer4 = new char[c2NormEntry.size];
      fin->seek(c2NormEntry.position);
      fin->readFully(buffer4, c2NormEntry.size);
    
    membuf sbuf4(buffer4, buffer4 + c2NormEntry.size);
    istream bufferin2(&sbuf4);
    c2Norm = readNormalizationVector(bufferin2);
    delete buffer3;
    delete buffer4;
  }

  int blockBinCount, blockColumnCount;
  
    readMatrix(fin, myFilePos, unit, binsize, blockBinCount, blockColumnCount); 
  
  set<int> blockNumbers = getBlockNumbersForRegionFromBinPosition(regionIndices, blockBinCount, blockColumnCount, c1==c2); 

  // getBlockIndices
  vector<contactRecord> records;
  for (set<int>::iterator it=blockNumbers.begin(); it!=blockNumbers.end(); ++it) {
    // get contacts in this block
    records = readBlock(fin, *it);
    for (vector<contactRecord>::iterator it2=records.begin(); it2!=records.end(); ++it2) {
      contactRecord rec = *it2;
      
      int x = rec.binX * binsize;
      int y = rec.binY * binsize;
      float c = rec.counts;
      if (norm != "NONE") {
	c = c / (c1Norm[rec.binX] * c2Norm[rec.binY]);
      }

      if ((x >= origRegionIndices[0] && x <= origRegionIndices[1] &&
	   y >= origRegionIndices[2] && y <= origRegionIndices[3]) ||
	  // or check regions that overlap with lower left
	  ((c1==c2) && y >= origRegionIndices[0] && y <= origRegionIndices[1] && x >= origRegionIndices[2] && x <= origRegionIndices[3])) {
	XYCount data;
	data.x = x;
	data.y = y;
	data.count = c;
	results.push_back(data);
	//printf("%d\t%d\t%.14g\n", x, y, c);
      }
    }
  }
      //      free(chunk.memory);      
      /* always cleanup */
      // curl_easy_cleanup(curl);
      //    curl_global_cleanup();
   return EXIT_SUCCESS;
}

HiCReader::HiCReader(const char* fname) {

	}
HiCReader::~HiCReader() {
	for(size_t i=0;i< chromosomes.size();i++) {
		delete chromosomes[i];
		}
	}



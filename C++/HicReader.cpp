#include <cstring>
#include "HicReader.hh"
#include "RemoteSeekableStream.hh"
#include "FileSeekableStream.hh"
#include "Debug.hh"
using namespace std;



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

bool HicReader::parseInterval(string s,QueryInterval* interval) const {
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

	void HicReader::query(HicQuery* q) {
	 if (q==NULL) return;
	 QueryInterval interval1;
	 QueryInterval interval2;
	 
	 if (q->interval1str != NULL) {
	 	if(!this->parseInterval(q->interval1str,&interval1)) return;
	 	q->interval1 = &interval1;
	 } else
	 	{
	 	q->interval1 = NULL;
	 	}

	if (q->interval2str != NULL) {
	 	if(!this->parseInterval(q->interval2str,&interval2)) return;
		q->interval2 = &interval2;
	 } else
	 	{
	 	q->interval2 = NULL;
	 	}
	 if(q->interval1!=NULL && q->interval2!=NULL && q->interval1->tid()< q->interval2->tid()) {
	 	swap(q->interval1,q->interval2);
	 	}
	}


// reads the footer from the master pointer location. takes in the chromosomes,
// norm, unit (BP or FRAG) and resolution or binsize, and sets the file 
// position of the matrix and the normalization vectors for those chromosomes 
// at the given normalization and resolution
void HicReader::queryFooter(HicQuery* q) {
 fin->seek(master);
  int32_t nBytes  = fin->readInt();
  stringstream ss;
  ss << c1 << "_" << c2;
  string key = ss.str();
  
  int32_t nEntries = fin->readInt();
  bool found = false;
  for (int i=0; i<nEntries; i++) {
    string str = fin->readString();
    int64_t fpos  = fin->readLong();
    int32_t sizeinbytes  = fin->readInt();
    if (str == key) {
      myFilePos = fpos;
      found=true;
    }
  }
  if (!found) return;

  if (q->norm=="NONE") return; // no need to read norm vector index
 
  // read in and ignore expected value maps; don't store; reading these to 
  // get to norm vector index
  int32_t nExpectedValues = fin->readInt();

  for (int i=0; i<nExpectedValues; i++) {
    string str = fin->readString();
    int32_t binSize = fin->readInt();

    int32_t nValues = fin->readInt();
    for (int j=0; j<nValues; j++) {
      double v = fin->readDouble();
    }

    int32_t nNormalizationFactors = fin->readInt();
    for (int j=0; j<nNormalizationFactors; j++) {
      int32_t chrIdx = fin->readInt();
      double v= fin->readDouble();
    }
  }
  
  nExpectedValues = fin->readInt();
  for (int32_t i=0; i<nExpectedValues; i++) {
    fin->readString(); //typeString
    fin->readString(); //unit
    int32_t binSize = fin->readInt();
    int32_t nValues = fin->readInt();
    for (int j=0; j<nValues; j++) {
      double v = fin->readDouble();
    }
    int32_t nNormalizationFactors = fin->readInt();
    for (int j=0; j<nNormalizationFactors; j++) {
      int32_t chrIdx  =fin->readInt();
      double v = fin->readDouble();
    }
  }
  // Index of normalization vectors
  int32_t nEntries = fin->readInt();
  bool found1 = false;
  bool found2 = false;
  for (int i = 0; i < nEntries; i++) {
    string normtype = fin->readString();
    int32_t chrIdx = fin->readInt();
    string unit1 = fin->readString()
    int32_t resolution1 = fin->readInt();
    int64_t filePosition = fin->readLong();
    int sizeInBytes = fin->readInt();
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
  
  
  }
}



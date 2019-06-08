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



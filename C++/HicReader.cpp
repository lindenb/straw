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
	std::map<std::string,Chromosome*>::iterator r = name2chrom.find(s);
	return r==name2chrom.end()?NULL:(Chromosome*)r->second;
	}

bool HicReader::parseInterval(const char* str,QueryInterval* interval) const {
	std::string s(str);
	std::string::size_type colon = s.find(':');
	if(colon == string::npos) //whole chrom {
		interval->chromosome = find_chromosome_by_name(s);
		if( interval->chromosome == NULL) {
			DEBUG("unknown chromosome in " << str);
			return false;
			}
		interval->start = 0;
		interval->end = interval->chromosome->length;
		return true;
		}
	else
		{
		string chrom = str.substr(0,colon);
		interval->chromosome = find_chromosome_by_name(s);
		if( interval->chromosome == NULL) {
			DEBUG("unknown chromosome in " << str);
			return false;
			}
		
		std::string::size_type hyphen = s.find('-',colon+1);
		if(hyphen == string::npos) hyphen = s.find(':',colon+1);
		if(hyphen == string::npos) {
			DEBUG("bad interval " << str);
			return false;
			}
		interval->start = atoi(str.substr(colon+1,(hyphen-colon)));
		interval->end = atoi(str.substr(hyphen+1));
		if(interval->start >= interval->end) {
			DEBUG("bad interval " << str);
			return false;
			}
		return true;
		}
	}


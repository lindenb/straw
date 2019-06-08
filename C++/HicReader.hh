#ifndef HIC_READER_HH
#define HIC_READER_HH

#include <map>
#include <string>
#include <vector>
#include "SeekableStream.hh"

class HicReader {
private:
	class Chromosome {
	public:
		std::string name;
		int length;
		int index;
	};
   std::map<std::string,Chromosome*> name2chrom;
   std::vector<Chromosome*> chromosomes;
   SeekableStream* bufferin;
   int version;
   std::string build;
   std::map<std::string,std::string> attributes;
   long master;
public:
	HicReader(const char* s);
	~HicReader();
};

#endif



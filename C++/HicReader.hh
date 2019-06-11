#ifndef HIC_READER_HH
#define HIC_READER_HH

#include <map>
#include <string>
#include <vector>
#include "SeekableStream.hh"

typedef std::string norm_t;
typedef int32_t resolution_t;
typedef std::string unit_t;
typedef void (*query_callback_t)(void*) ;

class Chromosome {
	public:
		std::string name;
		std::int32_t length;
		std::int32_t index;
	};




class HicReader {
private:
	
   std::string source;
   std::map<std::string,Chromosome*> name2chrom;
   std::vector<Chromosome*> chromosomes;
   SeekableStream* fin;
   std::int32_t version;
   std::string build;
   std::map<std::string,std::string> attributes;
   std::int64_t master;
   
   Chromosome* find_chromosome_by_name(std::string c) const;
public:
	HicReader(const char* s);
	~HicReader();

	bool query(const char* interval1,const char* interval2,norm_t norm,unit_t unit,resolution_t binSize,query_callback_t);
private:
	bool parseInterval(std::string str,void* interval) const;
	bool queryFooter(void* q);
};

#endif



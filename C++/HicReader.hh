#ifndef HIC_READER_HH
#define HIC_READER_HH

#include <map>
#include <string>
#include <vector>
#include "SeekableStream.hh"

enum Normalisation {
	VC
	};
	
enum Unit {
	BP,FRAG
	};

class Chromosome {
	public:
		std::string name;
		std::int32_t length;
		std::int32_t index;
	};

struct QueryInterval
	{
	Chromosome* chromosome;
	std::int32_t start;
	std::int32_t end;
	std::int32_t tid() const {
		return chromosome->index;
		}
	};


class HicQuery
	{
	public:
		const char* interval1str;
		const char* interval2str;
		QueryInterval* interval1;
		QueryInterval* interval2;
		std::string norm;
		std::string unit;
		int32_t resolution		
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
	bool parseInterval(std::string str,QueryInterval* interval) const;
	void query(HicQuery* q);
private:
	void queryFooter(HicQuery* q);
};

#endif



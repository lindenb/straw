#include "HicReader.hh"

using namespace std;

HicReader::HicReader(const char* s) {
	}

HicReader::~HicReader() {
	if(bufferin!=0) delete bufferin;
	for(size_t i=0;i< chromosomes.size();i++) delete chromosomes[i];
	}



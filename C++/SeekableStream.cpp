#include <cstdlib>
#include <climits>
#include "Debug.hh"
#include "SeekableStream.hh"

using namespace std;

SeekableStream::SeekableStream() {
	}
SeekableStream::~SeekableStream() {
	}



std::string  SeekableStream::readString() {
	std::ostringstream oss;
	int c;
	while((c=this->read())!=0)
		{
		if(c==EOF) THROW_ERROR("Cannot read string : EOF exception");
		if(c<0) THROW_ERROR("negative char " << c);
		if(c>SCHAR_MAX) THROW_ERROR("> SCHAR_MAX " << c);
		oss << (char)c;
		}	
	return oss.str();
	}

size_t SeekableStream::read(void* s,std::size_t len) {
	size_t i =0UL;
	unsigned char* p=(unsigned char*)s;
	while(i<len)
		{
		int c = this->read();
		if(c==EOF)  break;
		p[i] = (unsigned char) c;
		i++;
		}
	return i;
	}

void SeekableStream::readFully(void* s,std::size_t len) {
	if(read(s,len)!=len) THROW_ERROR("Cannot read " << len << " : EOF exception");
	}

#define READ_FUN(FUN,TYPE) TYPE SeekableStream::read##FUN () {\
	TYPE value;\
	this->readFully((void*)&value,sizeof(TYPE));\
	return value;\
	}
READ_FUN(Int,int32_t);
READ_FUN(Long,int64_t);
READ_FUN(Short,int16_t);
READ_FUN(Char,char);
READ_FUN(Float,float);
READ_FUN(Double,double);
		
	

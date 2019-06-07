#include <cstdlib>
#include <sstream>
#include <iostream>
#include "SeekableStream.hh"

using namespace std;

SeekableStream::SeekableStream() {
	}
SeekableStream::~SeekableStream() {
	}



std::string  SeekableStream::readString() {
	std::ostringstream oss;
	
	for(;;)
		{
		int c = this->read();
		if(c==EOF)  {
			cerr << "EOF exception" << endl;
			exit(EXIT_FAILURE);
			}
		if(c=='\0') break;
		oss << (char)c;
		}	
	return oss.str();
	}

size_t SeekableStream::read(void* s,std::size_t len) {
	size_t i =0UL;
	char* p=(char*)s;
	while(i<len)
		{
		int c = this->read();
		if(c==EOF)  break;
		p[i]=(char)c;
		i++;
		}
	return i;
	}

void SeekableStream::readFully(void* s,std::size_t len) {
	if(read(s,len)!=len)  {
		cerr << "EOF exception" << endl;
		exit(EXIT_FAILURE);
		}
	}

#define READ_FUN(FUN,TYPE) TYPE SeekableStream::read##FUN () {\
	TYPE value;\
	this->readFully((void*)&value,sizeof(TYPE));\
	return value;\
	}
READ_FUN(Int,int);
READ_FUN(Long,long);
READ_FUN(Short,short);
READ_FUN(Char,char);
READ_FUN(Float,float);
READ_FUN(Double,double);
		
	

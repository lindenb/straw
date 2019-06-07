#include <cerrno>
#include <iostream>
#include <cstring>
#include "FileSeekableStream.hh"

using namespace std;

FileSeekableStream::FileSeekableStream(const char* f) {
in = fopen(f,"rb");
if(in==NULL) {
	cerr << "Cannot open " << f << " " << strerror(errno);
	}
}


FileSeekableStream::~FileSeekableStream() {
fclose(in);
}
	
int FileSeekableStream::read() {
return fgetc(in);
}

void FileSeekableStream::seek(long offset) {
	if(fseek(in,offset,SEEK_SET)!=0) {
		cerr << "Cannot fseek " << offset << " " << strerror(errno);
		}
	}

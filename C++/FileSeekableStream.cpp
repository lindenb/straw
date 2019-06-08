#include <cerrno>
#include "Debug.hh"
#include <cstring>
#include "FileSeekableStream.hh"

using namespace std;

FileSeekableStream::FileSeekableStream(const char* f) {
in = fopen(f,"rb");
if(in==NULL) THROW_ERROR("Cannot open " << f << " " << strerror(errno));
}


FileSeekableStream::~FileSeekableStream() {
fclose(in);
}
	
int FileSeekableStream::read() {
return fgetc(in);
}

void FileSeekableStream::seek(long offset) {
	if(fseek(in,offset,SEEK_SET)!=0) THROW_ERROR("Cannot fseek " << offset << " " << strerror(errno));
	}

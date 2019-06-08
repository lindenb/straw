#include <cerrno>
#include "Debug.hh"
#include <cstring>
#include "MemorySeekableStream.hh"

using namespace std;

MemorySeekableStream::MemorySeekableStream(const char* buffer,std::size_t len):buffer(buffer),len(len),offset(0UL) {
}


MemorySeekableStream::~MemorySeekableStream() {
}
	
int MemorySeekableStream::read() {
if(offset>=len) return -1;
int c=buffer[offset];
offset++;
return c;
}

void MemorySeekableStream::seek(long p) {
	if(p< 0) THROW(range_error,"negative offset " << p);
	if(p> (long)len) THROW(range_error,"offset " << p << " > " <<  len);
	offset=(size_t)p;
	}

#ifndef MEMORY_SEEKABLE_STREAM_HH
#define MEMORY_SEEKABLE_STREAM_HH
#include <cstdlib>
#include "SeekableStream.hh"


class MemorySeekableStream : public SeekableStream {
private:
    const char* buffer;
    std::size_t len;
    std::size_t offset;
public:
	MemorySeekableStream(const char* buffer,std::size_t len);
	virtual ~MemorySeekableStream();
	virtual int read();
	virtual void seek(long offset);
};

#endif


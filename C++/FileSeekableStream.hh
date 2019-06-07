#ifndef FILE_SEEKABLE_STREAM_HH
#define FILE_SEEKABLE_STREAM_HH
#include <cstdlib>
#include <cstdio>
#include "SeekableStream.hh"


class FileSeekableStream : public SeekableStream {
private:
    std::FILE* in;
public:
	FileSeekableStream(const char* url);
	virtual ~FileSeekableStream();
	virtual int read();
	virtual void seek(long offset);
};

#endif


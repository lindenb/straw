#ifndef SEEKABLE_STREAM_HH
#define SEEKABLE_STREAM_HH

#include <cstddef>
#include <string>

class SeekableStream {
public:
	SeekableStream();
	virtual ~SeekableStream();
	virtual int read() = 0;
	virtual void seek(long offset)=0;
	void readFully(void* s,std::size_t len);
	char readChar();
	short readShort();
	int readInt();
	long readLong();
	float readFloat();
	double readDouble();
	std::string readString();
};

#endif

#ifndef SEEKABLE_STREAM_HH
#define SEEKABLE_STREAM_HH

#include <cstddef>
#include <string>
#include <cstdint>

class SeekableStream {
public:
	SeekableStream();
	virtual ~SeekableStream();
	virtual int read() = 0;
	virtual void seek(long offset)=0;
	size_t read(void* s,std::size_t len);
	void readFully(void* s,std::size_t len);
	char readChar();
	std::int16_t readShort();
	std::int32_t readInt();
	std::int64_t readLong();
	float readFloat();
	double readDouble();
	std::string readString();
};

#endif

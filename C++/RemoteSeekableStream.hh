#ifndef REMOTE_SEEKABLE_STREAM_HH
#define REMOTE_SEEKABLE_STREAM_HH
#include <cstdlib>
#include <iostream>
#include <curl/curl.h>
#include "SeekableStream.hh"


class RemoteSeekableStream : public SeekableStream {
private:
    CURL* curl;
	char* buffer;
	std::size_t buffer_size;
	std::size_t buffer_curr;
	std::size_t buffer_end;
	unsigned long offset;
	bool refill();
public:
	RemoteSeekableStream(const char* url);
	virtual ~RemoteSeekableStream();
	virtual int read();
	void setbuf(const char* s, std::size_t len);
	virtual void seek(long offset);
};

#endif


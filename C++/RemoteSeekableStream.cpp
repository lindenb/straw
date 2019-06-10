#include <sstream>
#include <cstdio>
#include <cstring>
#include "Debug.hh"
#include "RemoteSeekableStream.hh"
using namespace std;

static size_t header_callback(char *buffer,   size_t size,   size_t nitems,   void *userdata) {
//RemoteSeekableStream* obj = (RemoteSeekableStream*)userdata;


return size*nitems;
} 

// callback for libcurl. data written to this buffer
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
 
 RemoteSeekableStream* obj = (RemoteSeekableStream*)userp;
 obj->setbuf((char*)contents,size*nmemb);
 return  size * nmemb;
}

void RemoteSeekableStream::setbuf(const char* s, std::size_t len) {
	size_t max_fill= std::min(len,this->buffer_size - this->buffer_end);
	memcpy(&this->buffer[buffer_end],s,max_fill);
	buffer_end+=max_fill;
	}

bool RemoteSeekableStream::refill() {
	buffer_curr=0;
	buffer_end= 0;
	std::ostringstream oss;
	oss << offset << "-" << offset + buffer_size;
    string s(oss.str());
   
    curl_easy_setopt(curl, CURLOPT_RANGE, s.c_str());
    
    CURLcode res = ::curl_easy_perform(this->curl);
	if (res != CURLE_OK) THROW_ERROR("curl_easy_perform() failed: " << curl_easy_strerror(res));
	
	return buffer_end > 0;
	}
void RemoteSeekableStream::seek(long offset) {
	this->offset = offset;
	this->buffer_curr  = this->buffer_end;
	}

int RemoteSeekableStream::read() {
	if(buffer_curr>=buffer_end) {
		if(!refill()) return -1;
		}
	char c =buffer[buffer_curr];
	buffer_curr++;
	offset++;
	return c;
	}

RemoteSeekableStream::RemoteSeekableStream(const char* url) {
	buffer_size = 10000;
	buffer = new char[buffer_size];
	buffer_curr = 0UL;
	buffer_end = 0UL;
	offset = 0L;
	this->curl = ::curl_easy_init();
	if(this->curl == NULL) THROW_ERROR("cannot ::curl_easy_init");
	  		
	curl_easy_setopt(curl, CURLOPT_URL, url);
	//curl_easy_setopt (curl, CURLOPT_VERBOSE, 1L); 
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA, this);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "straw");
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
	}

RemoteSeekableStream::~RemoteSeekableStream() {
 curl_easy_cleanup(this->curl);
 delete[] buffer;
 }
	
	/*
int main(int argc,char** argv) {
RemoteSeekableStream in("https://raw.githubusercontent.com/danlimsk/bioingorwithpython/6fd606cfcc85fd0d5cce6b11c4bbb4d67e395fda/data/rebase-bionet-format.txt");
int c;
while((c=in.read())!=-1) {

	fputc(c,stdout);
	} 
return 0;
}	*/


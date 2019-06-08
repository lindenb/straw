CFLAGS=-Wall -g -std=c++0x
LIBS=-lcurl -lz
CC=g++

straw: main.o straw.o SeekableStream.o FileSeekableStream.o RemoteSeekableStream.o MemorySeekableStream.o HicReader.o
	$(CC) -o $@ $^ $(LIBS)

ori.straw: ori.main.cpp ori.straw.cpp
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)


test: straw
	./straw VC ~/jeter.hic  "1:100000000:100010000"  "1:100000000:100010000" BP 2500

ori.test: ori.straw
	./ori.straw VC ~/jeter.hic  "1:100000000:100010000"  "1:100000000:100010000" BP 2500


a.out : SeekableStream.o FileSeekableStream.o RemoteSeekableStream.o
	$(CC) -o $@ $^ $(LIBS)

HicReader.o: HicReader.cpp HicReader.hh  SeekableStream.hh
	$(CC) -c $(CFLAGS) -o $@ $<

FileSeekableStream.o: FileSeekableStream.cpp FileSeekableStream.hh  SeekableStream.hh
	$(CC) -c $(CFLAGS) -o $@ $<

RemoteSeekableStream.o: RemoteSeekableStream.cpp RemoteSeekableStream.hh  SeekableStream.hh
	$(CC) -c $(CFLAGS) -o $@ $<

SeekableStream.o: SeekableStream.cpp SeekableStream.hh
	$(CC) -c $(CFLAGS) -o $@ $<

MemorySeekableStream.o: MemorySeekableStream.cpp MemorySeekableStream.hh
	$(CC) -c $(CFLAGS) -o $@ $<

straw.o: straw.cpp  straw.h
	$(CC) -c $(CFLAGS) -o $@  $<

main.o: main.cpp  straw.h
	$(CC) -c $(CFLAGS) -o $@  $<


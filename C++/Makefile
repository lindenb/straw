CFLAGS=-Wall -O3 -std=c++0x
LIBS=-lcurl -lz
CC=g++

a.out : SeekableStream.o FileSeekableStream.o RemoteSeekableStream.o
	$(CC) -o $@ $^ $(LIBS)

FileSeekableStream.o: FileSeekableStream.cpp FileSeekableStream.hh  SeekableStream.hh
	$(CC) -c $(CFLAGS) -o $@ $<

RemoteSeekableStream.o: RemoteSeekableStream.cpp RemoteSeekableStream.hh  SeekableStream.hh
	$(CC) -c $(CFLAGS) -o $@ $<

SeekableStream.o: SeekableStream.cpp SeekableStream.hh
	$(CC) -c $(CFLAGS) -o $@ $<

straw: main.o straw.o
	$(CC) -o $@ $^ $(LIBS)

straw.o: straw.cpp  straw.h
	$(CC) -c $(CFLAGS) -o $@  $<

main.o: main.cpp  straw.h
	$(CC) -c $(CFLAGS) -o $@  $<


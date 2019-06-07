CFLAGS=-Wall -O3 -std=c++0x
LIBS=-lcurl -lz
CC=g++

straw: main.o straw.o SeekableStream.o FileSeekableStream.o RemoteSeekableStream.o
	$(CC) -o $@ $^ $(LIBS)

a.out : SeekableStream.o FileSeekableStream.o RemoteSeekableStream.o
	$(CC) -o $@ $^ $(LIBS)

FileSeekableStream.o: FileSeekableStream.cpp FileSeekableStream.hh  SeekableStream.hh
	$(CC) -c $(CFLAGS) -o $@ $<

RemoteSeekableStream.o: RemoteSeekableStream.cpp RemoteSeekableStream.hh  SeekableStream.hh
	$(CC) -c $(CFLAGS) -o $@ $<

SeekableStream.o: SeekableStream.cpp SeekableStream.hh
	$(CC) -c $(CFLAGS) -o $@ $<



straw.o: straw.cpp  straw.h
	$(CC) -c $(CFLAGS) -o $@  $<

main.o: main.cpp  straw.h
	$(CC) -c $(CFLAGS) -o $@  $<


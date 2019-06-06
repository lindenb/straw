CFLAGS=-Wall -O3 -std=c++0x
LIBS=-lcurl -lz
CC=g++
straw: main.o straw.o
	$(CC) -o $@ $^ $(LIBS)

straw.o: straw.cpp  straw.h
	$(CC) -c $(CFLAGS) -o $@  $<

main.o: main.cpp  straw.h
	$(CC) -c $(CFLAGS) -o $@  $<


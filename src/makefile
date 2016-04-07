# the compiler to use
CC=g++
# options pass to the compiler
CFLAGS=-g -Wall -lpthread

all: station bridge

station: station.cpp DataIO.cpp ByteIO.cpp
	$(CC) station.cpp DataIO.cpp ByteIO.cpp -o $@ $(CFLAGS)

bridge: bridge.cpp
	$(CC) bridge.cpp DataIO.cpp ByteIO.cpp -o $@ $(CFLAGS)

clean:
	rm -rf *o station bridge
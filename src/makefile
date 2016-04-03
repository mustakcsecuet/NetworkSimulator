CC     = gcc
CFLAGS = -g
INCLUDES = ether.h ip.h
CFLAGS = -c -g

all: bridge station 

bridge: bridge.o
	gcc -g bridge.o -o bridge

station: station.o
	gcc -g station.o -o station

clean : 
	rm -f bridge station *.o

%.o : %.c $(INCLUDES)
	$(CC) $(CFLAGS) $<


CC=gcc
CFLAGS=-Wall -I/usr/include/libusb-1.0 -lusb-1.0

all: ink

ink: ink.o
	$(CC) $(CFLAGS) ink.o -o ink

ink.o: ink.c
	$(CC) -Wall -c ink.c

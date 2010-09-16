CC=gcc
CFLAGS=-Wall -I/usr/include/libusb-1.0 -lusb-1.0

all: escp-usb

escp-usb: escp-usb.o
	$(CC) $(CFLAGS) $< -o $@

escp-usb.o: escp-usb.c
	$(CC) -Wall -c $<

# Makefile for libpca9685 and example

# parameters:
CC = gcc
CFLAGS = -ansi -pedantic -Wall -g

# targets:
all: libpca9685.so.0.1

libpca9685.so.0.1: libpca9685.o
	$(CC) -shared -Wl,-soname,libpca9685.so -o libpca9685.so.0.1 \
	      libpca9685.o

libpca9685.o: pca9685.c pca9685.h
	$(CC) $(CFLAGS) -c pca9685.c -o libpca9685.o


.PHONEY: clean
clean:
	rm -f libpca9685.o libpca9685.so.0.1

.PHONEY: install
install:
	install -m 0755 -d /usr/local/include
	install -m 0644 pca9685.h /usr/local/include
	install -m 0755 -d /usr/local/lib
	install -m 0755 libpca9685.so.0.1 /usr/local/lib/libpca9685.so.0.1
	ln -sf /usr/local/lib/libpca9685.so.0.1 /usr/local/lib/libpca9685.so
	ldconfig

.PHONEY: uninstall
uninstall:
	rm -f /usr/local/include/pca9685.h
	rm -f /usr/local/lib/libpca9685.*
	ldconfig

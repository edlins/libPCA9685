# Makefile for libPCA9685 example

# name and version
NAME=PCA9685demo

# constants
SRC=$(NAME).c
OBJ=$(NAME).o
EXE=$(NAME)

# parameters:
CC = gcc
CFLAGS = -pedantic -Wall -D_BSD_SOURCE -std=c11 -lncurses

# targets:
all: PCA9685demo quickstart

PCA9685demo: PCA9685demo.o
	$(CC) $(CFLAGS) -o PCA9685demo PCA9685demo.o -lPCA9685

PCA9685demo.o: PCA9685demo.c
	$(CC) $(CFLAGS) -o PCA9685demo.o -c PCA9685demo.c

quickstart: quickstart.o
	$(CC) $(CFLAGS) -o quickstart quickstart.o -lPCA9685

quickstart.o: quickstart.c
	$(CC) $(CFLAGS) -o quickstart.o -c quickstart.c

.PHONEY: clean
clean:
	rm -f PCA9685demo.o PCA9685demo quickstart.o quickstart

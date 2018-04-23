# Makefile for quickstart example

# constants
SRC=$(NAME).c
OBJ=$(NAME).o
EXE=$(NAME)

# parameters:
CC = gcc
CFLAGS = -pedantic -Wall -D_BSD_SOURCE -std=c11 -lncurses

# targets:
all: quickstart

quickstart: quickstart.o
	$(CC) $(CFLAGS) -o quickstart quickstart.o -lPCA9685

quickstart.o: quickstart.c
	$(CC) $(CFLAGS) -o quickstart.o -c quickstart.c

.PHONEY: clean
clean:
	rm -f quickstart.o quickstart

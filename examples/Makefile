# Makefile for libpca9685 and example

# parameters:
CC = gcc
CFLAGS = -ansi -pedantic -Wall -g -D_BSD_SOURCE

# targets:
all: picontrol

picontrol: picontrol.o
	$(CC) $(CFLAGS) -o picontrol picontrol.o -lpca9685

picontrol.o: picontrol.c
	$(CC) $(CFLAGS) -c picontrol.c


.PHONEY: clean
clean:
	rm -f picontrol picontrol.o

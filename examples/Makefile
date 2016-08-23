# Makefile for libPCA9685 example

# parameters:
CC = gcc
CFLAGS = -ansi -pedantic -Wall -g -D_BSD_SOURCE -std=c11 -lncurses

# targets:
all: picontrol

picontrol: picontrol.o
	$(CC) $(CFLAGS) -o picontrol picontrol.o -lPCA9685

picontrol.o: picontrol.c
	$(CC) $(CFLAGS) -c picontrol.c


.PHONEY: clean
clean:
	rm -f picontrol picontrol.o

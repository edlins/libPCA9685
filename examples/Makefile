# Makefile for libPCA9685 example

# name and version
NAME=PCA9685demo

# constants
SRC=$(NAME).c
OBJ=$(NAME).o
EXE=$(NAME)

# parameters:
CC = gcc
CFLAGS = -ansi -pedantic -Wall -g -D_BSD_SOURCE -std=c11 -lncurses

# targets:
all: $(EXE)

$(EXE): $(OBJ)
	$(CC) $(CFLAGS) -o $(EXE) $(OBJ) -lPCA9685

$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c $(SRC)


.PHONEY: clean
clean:
	rm -f $(OBJ) $(EXE)

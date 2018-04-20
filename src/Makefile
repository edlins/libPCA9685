# Makefile for libPCA9685

# name and version (all other values derived)
NAME=PCA9685
VERS_MAJ=0
VERS_MIN=6



# constants
VERSION=$(VERS_MAJ).$(VERS_MIN)
SRC=$(NAME).c
HDR=$(NAME).h
LIB=lib$(NAME)
LIBFILE=$(LIB).o
DYNLIBFILE=$(LIB).so
DYNLIBFILEVERS=$(LIB).so.$(VERSION)

# parameters:
CC = gcc
CFLAGS = -pedantic -Wall -std=c11

# targets:
all: $(DYNLIBFILEVERS)

$(DYNLIBFILEVERS): $(LIBFILE)
	$(CC) -shared -Wl,-soname,$(DYNLIBFILE) -o $(DYNLIBFILEVERS) $(LIBFILE)

$(LIBFILE): $(SRC) $(HDR)
	$(CC) $(CFLAGS) -c $(SRC) -o $(LIBFILE)

.PHONEY: clean
clean:
	rm -f *.o *.so.*

.PHONEY: install
install:
	install -m 0755 -d /usr/local/include
	install -m 0644 $(HDR) /usr/local/include
	install -m 0755 -d /usr/local/lib
	install -m 0755 $(DYNLIBFILEVERS) /usr/local/lib/$(DYNLIBFILEVERS)
	ln -sf /usr/local/lib/$(DYNLIBFILEVERS) /usr/local/lib/$(DYNLIBFILE)
	ldconfig

.PHONEY: uninstall
uninstall:
	rm -f /usr/local/include/$(HDR)
	rm -f /usr/local/lib/$(DYNLIBFILE)
	rm -f /usr/local/lib/$(DYNLIBFILE).*
	ldconfig

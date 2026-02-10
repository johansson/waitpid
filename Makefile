ALL=waitpid
CC ?= cc
CFLAGS ?= -g -Wall -O2
PREFIX ?= /usr/local
# for sudo not retaining PREFIX
ifeq ($(PREFIX),)
PREFIX=/usr/local
endif
BINDIR=$(PREFIX)/bin
INSTALL=install
UNAME_S := $(shell uname -s)
INSTALL_GROUP=
ifeq ($(UNAME_S),Darwin)
INSTALL_GROUP=-g wheel
endif
FORMATTER ?= /Library/Developer/CommandLineTools/usr/bin/clang-format

all: $(ALL)

install: all
	$(INSTALL) $(INSTALL_GROUP) -m0755 $(ALL) $(BINDIR)

format:
	$(FORMATTER) -i src/main.c

waitpid:
	$(CC) $(CFLAGS) src/main.c -o waitpid

clean:
	rm -rf $(ALL) waitpid.dSYM

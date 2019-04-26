.PHONY: all clean

sudo?=
prefix?=/usr/local

MODULES=\
	bignum\
	curses\
	env\
	example\
	small\
	sqlite\
	util

all clean install:
	@for d in $(MODULES); do $(MAKE) -$(MAKEFLAGS) -C $$d sudo=$(sudo) prefix=$(prefix) $@; done

.PHONY: all clean

prefix?=/usr/local
sudo?=

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

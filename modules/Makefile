.PHONY: all clean

prefix?=/usr/local
sudo?=

_dirs=\
	bignum\
	curses\
	env\
	example\
	small\
	sndfile\
	sqlite\
	util

all clean install:
	@for _d in $(_dirs); do $(MAKE) -$(MAKEFLAGS) -C $$_d sudo=$(sudo) prefix=$(prefix) $@; done

.PHONY: all clean

sudo?=
prefix?=/usr/local

MODULES=\
	bignum\
	env\
	example\
	small

all clean install:
	@for d in $(MODULES); do $(MAKE) -$(MAKEFLAGS) -C $$d sudo=$(sudo) prefix=$(prefix) $@; done

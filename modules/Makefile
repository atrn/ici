.PHONY: all clean

sudo?=
prefix?=/usr/local

MODULES=\
	bignum\
	example\
	small

all:
	@for d in $(MODULES); do make -C $$d sudo=$(sudo) prefix=$(prefix) $@; done

clean:
	@for d in $(MODULES); do make -C $$d sudo=$(sudo) prefix=$(prefix) $@; done

install:
	@for d in $(MODULES); do make -C $$d sudo=$(sudo) prefix=$(prefix) $@; done

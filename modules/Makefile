.PHONY: all clean

prefix?=/usr/local
sudo?=

_dirs=\
	bignum\
	curses\
	env\
	example\
	ipp\
	small\
	sndfile\
	sqlite\
	util\
	vec

_make=	$(MAKE) --no-print-directory

all clean install:
	@for _d in $(_dirs); do (cd $$_d; echo $$_d; $(_make) -$(MAKEFLAGS) sudo=$(sudo) prefix=$(prefix) $@); done

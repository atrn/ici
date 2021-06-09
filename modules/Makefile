#
# GNU make specific Makefile for building all modules.
#

.PHONY: all clean

prefix?=/usr/local
sudo?=

ICI_DOT_H_DIR?=$(PWD)/..
ICI_LIB_DIR?=$(PWD)/..

ifeq ($(shell uname),Darwin)
ifeq($(ICI_BUILD_TYPE_DLL),)
ICI_MACOS_BUNDLE_HOST=ici
else
ICI_MACOS_BUNDLE_HOST=libici.dylib
endif
endif

MODULES=\
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
	@for _d in $(MODULES);\
	do (\
	    echo $$_d; \
	    cd $$_d; \
	    $(_make) -$(MAKEFLAGS) \
		sudo=$(sudo) prefix=$(prefix) \
		ICI_DOT_H_DIR=$(ICI_DOT_H_DIR) \
		ICI_LIB_DIR=$(ICI_LIB_DIR) \
		ICI_MACOS_BUNDLE_HOST=$(ICI_MACOS_BUNDLE_HOST) \
		$@ \
        ); done

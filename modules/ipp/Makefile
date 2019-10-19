.PHONY: all dll clean test-loading install

sudo?=
prefix?=/usr/local

# ippdir must exist for this makefile to do anything

ippdir?=/opt/intel/ipp/

_os=$(shell uname)
ifeq ($(_os),Darwin)
_module=ici-ipp.dylib
else
_module=ici-ipp.so
endif

all: dll

dll:; @if [ -d $(ippdir) ]; then dmake -o $(_module) dll; else exit 0; fi

clean:; @if [ -d $(ippdir) ]; then dmake -o $(_module) dll clean; else exit 0; fi

install: dll
	@if [ -d $(ippdir) ]; then \
	    $(sudo) install -c -m 444 $(_module) $(prefix)/lib/ici/$(_module); \
	    $(sudo) install -c -m 444 ici-ipp.ici $(prefix)/lib/ici/ici-ipp.ici; \
	fi

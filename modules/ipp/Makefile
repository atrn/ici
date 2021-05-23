# IPPROOT must exist for this makefile to do anything
#
# IPPROOT is usually set the intel-provided setvars.sh

.PHONY: all dll clean test-loading install

sudo?=
prefix?=/usr/local

_os=$(shell uname)
ifeq ($(_os),Darwin)
_module=ici-ipp.dylib
else
_module=ici-ipp.so
endif

all: dll

dll:; @if [ -n "$(IPPROOT)" ]; then dmake -o $(_module) dll; else exit 0; fi

clean:; @if [ -n "$(IPPROOT)" ]; then dmake -o $(_module) dll clean; else exit 0; fi

install: dll
	@if [ -n "$(IPPROOT)" ]; then \
	    $(sudo) install -c -m 444 $(_module) $(prefix)/lib/ici/$(_module); \
	    $(sudo) install -c -m 444 ici-ipp.ici $(prefix)/lib/ici/ici-ipp.ici; \
	fi

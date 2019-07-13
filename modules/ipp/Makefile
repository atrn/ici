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

dll:; @dmake -o $(_module) dll

clean:; @dmake -o $(_module) dll clean

install: dll
	$(sudo) install -c -m 444 $(_module) $(prefix)/lib/ici/$(_module)
	$(sudo) install -c -m 444 ici-ipp.ici $(prefix)/lib/ici/ici-ipp.ici

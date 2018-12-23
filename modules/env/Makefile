.PHONY: all dll clean test-loading install

sudo?=
prefix=/usr/local

#  Our output filename is OS specific
_OS=$(shell uname)
ifeq ($(_OS),Darwin)
MODULE=ici-env.dylib
else
MODULE=ici-env.so
endif

all : dll

dll:; @dmake -o $(MODULE) dll

clean:; @dmake -o $(MODULE) dll clean

test-loading: dll

test: dll
	ici test-env.ici

install: dll
	$(sudo) install -c -m 444 $(MODULE) $(prefix)/lib/ici

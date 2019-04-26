.PHONY: all dll clean test-loading install

module=ici-util

sudo?=
prefix?=/usr/local
os=$(shell uname)
ifeq ($(os),Darwin)
dll=dylib
else
dll=so
endif

all : dll

dll:; @dmake -o $(module).$(dll) dll

clean:; @dmake -o $(module).$(dll) dll clean

test-loading: dll

test: dll
	ici test-util.ici

install: dll
	$(sudo) install -c -m 444 $(module).$(dll) $(prefix)/lib/ici
	$(sudo) install -c -m 444 $(module).ici $(prefix)/lib/ici

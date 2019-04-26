.PHONY: all dll clean install test

sudo?=
prefix?=/usr/local
module?=ici-sqlite
os=$(shell uname)
ifeq ($(os),Darwin)
dll=dylib
else
dll=so
endif

all : dll

dll:; @dmake -o $(module).$(dll) dll

clean:; @dmake -o $(module).$(dll) dll clean; rm -f tst-*.db

install: dll
	$(sudo) install -c -m 444 $(module).ici $(prefix)/lib/ici
	$(sudo) install -c -m 444 $(module).$(dll) $(prefix)/lib/ici

test: all
	ici tst-all.ici

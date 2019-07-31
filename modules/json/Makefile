.PHONY: all clean install

sudo?=
prefix=/usr/local

all:
	@:

clean:
	@:

install:
	@for m in ici-*.ici; do $(sudo) install -c -m 444 $$m $(prefix)/lib/ici; done

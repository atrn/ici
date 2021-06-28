.PHONY: all clean distclean install

sudo?=
prefix=/usr/local

all:; @:

clean:; @:

distclean:; @:

install:
	@for m in ici-*.ici; do $(sudo) install -c -m 444 $$m $(prefix)/lib/ici; done

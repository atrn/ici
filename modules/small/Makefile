.PHONY: build clean distclean install

sudo?=
prefix=/usr/local

build:; @true

clean:; @true

distclean:; @true

install:
	@for m in ici-*.ici; do $(sudo) install -c -m 444 $$m $(prefix)/lib/ici; done

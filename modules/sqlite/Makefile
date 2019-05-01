#
#  sqlite module.
#  Copyright (C) 2019 A.Newman.
#

.PHONY: all dll clean install test

module	= ici-sqlite
prefix	?= /usr/local
sudo	?=
os	= $(shell uname)
ifeq ($(os),Darwin)
dll	= dylib
else
dll	= so
endif

_dest	= $(prefix)/lib/ici
_dll	= $(module).$(dll)
_ici	= $(module).ici ici-data.ici

all : dll

dll:
	@dmake -o $(_dll) dll

clean:
	@dmake -o $(_dll) dll clean
	@rm -f tests/*.db

install: dll
	@$(sudo) install -c -m 444 $(_dll) $(_dest)
	@$(sudo) install -c -m 444 $(_ici) $(_dest)

test: all
	cd tests && ICIPATH=$PWD:/usr/local/lib/ici:. ici tst-all.ici

.PHONY: all dll clean install

module	= ici-curses
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
_ici	= $(module).ici

all : dll

dll:
	@dmake -o $(_dll) dll

clean:
	@dmake -o $(_dll) dll clean

install: dll
	@$(sudo) install -c -m 444 $(_dll) $(_dest)
#	@$(sudo) install -c -m 444 $(_ici) $(_dest)

.PHONY: all dll clean install

sudo?=
prefix=/usr/local

#  Our output filename is OS specific
_OS=$(shell uname)
ifeq ($(_OS),Darwin)
MODULE=ici-bignum.dylib
else
MODULE=ici-bignum.so
endif

all : dll

dll:
	@$(MAKE) -s -C mit BigNum.a && \
	dmake -o $(MODULE) dll

clean:
	@dmake -o $(MODULE) dll clean
	@$(MAKE) -s -C mit clean

install: dll
	$(sudo) install -c -m 444 ici-bignum.ici $(prefix)/lib/ici
	$(sudo) install -c -m 444 $(MODULE) $(prefix)/lib/ici

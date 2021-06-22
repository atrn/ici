.PHONY: all plugin clean install

ICI_MODULE_NAME=bignum

sudo?=
prefix?=/usr/local

_OS=$(shell uname)
ifeq ($(_OS),Darwin)
MODULE=ici-$(ICI_MODULE_NAME).bundle
else
MODULE=ici-$(ICI_MODULE_NAME).so
endif

ifndef ICI_LIB_DIR
ICI_LIB_DIR = $(prefix)/lib
endif

all : plugin

plugin:
	@$(MAKE) -s -C mit BigNum.a && \
	ICI_MODULE_NAME=$(ICI_MODULE_NAME) \
	ICI_LIB_DIR=$(ICI_LIB_DIR) \
	dmake -o $(MODULE) plugin

clean:
	@ICI_MODULE_NAME=$(ICI_MODULE_NAME) dmake -o $(MODULE) plugin clean
	@$(MAKE) -s -C mit clean

distclean: clean

install: plugin
	$(sudo) install -c -m 444 ici-bignum.ici $(prefix)/lib/ici
	$(sudo) install -c -m 444 $(MODULE) $(prefix)/lib/ici

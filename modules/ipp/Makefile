# IPPROOT must be set for this makefile to do anything
#
# IPPROOT is usually set the intel-provided setvars.sh

.PHONY: all dll clean install

ICI_MODULE_NAME=ipp

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
	ICI_MODULE_NAME=$(ICI_MODULE_NAME) \
	ICI_LIB_DIR=$(ICI_LIB_DIR) \
	dmake --dcc-debug -o $(MODULE) plugin

clean:
	@ICI_MODULE_NAME=$(ICI_MODULE_NAME) dmake plugin clean

distclean: clean

install: plugin
	$(sudo) install -c -m 444 ici-$(ICI_MODULE_NAME).ici $(prefix)/lib/ici
	$(sudo) install -c -m 444 $(MODULE) $(prefix)/lib/ici

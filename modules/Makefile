#
# (GNU make) Makefile for building all modules.
#

.PHONY: build clean distclean install

ALL_MODULES=\
	bignum\
	curses\
	env\
	example\
	small\
	sndfile\
	sqlite\
	str\
	util\
	vec\
	vm

silent?=	@

# If IPPROOT is defined (via the environment) assume it
# points to Intel's IPP and we can build the IPP module.

ifdef IPPROOT
ALL_MODULES += ipp
endif

ifndef ICI_MODULES_DIR
ICI_MODULES_DIR=$(shell pwd)
endif

# Include Makefile.inc to define common macros.

include $(ICI_MODULES_DIR)/Makefile.inc

_make=	$(MAKE) --no-print-directory

build clean distclean install:
	$(silent) for _d in $(ALL_MODULES);\
	do (\
	    $(_make) -C "$$_d" \
		sudo=$(sudo) \
		prefix=$(prefix) \
		ICI_DOT_H_DIR=$(ICI_DOT_H_DIR) \
		ICI_LIB_DIR=$(ICI_LIB_DIR) \
		ICI_MACOS_BUNDLE_HOST=$(ICI_MACOS_BUNDLE_HOST) \
		$@ \
        ); done

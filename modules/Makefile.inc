# -*- mode:makefile -*-
#
# Define commonly used macros
#
# prefix
# sudo
#
# ICI_DOT_H_DIR
# ICI_LIB_DIR
# ICI_MACOS_BUNDLE_HOST

prefix?=/usr/local
sudo?=

ICI_DOT_H_DIR?=$(prefix)/include
ICI_LIB_DIR?=$(prefix)/lib
ICI_PLUGIN_DIR?=$(prefix)/lib/ici

_UNAME=$(shell uname)

# Define ICI_MACOS_BUNDLE_HOST if required.
#
# If ICI was built as a dynamic library, which is the usual case the
# bundle host is that dynamic library otherwise we use the ici
# executable itself.

ifeq ($(_UNAME),Darwin)
ifdef ICI_BUILD_TYPE_DLL
ICI_MACOS_BUNDLE_HOST?=$(ICI_LIB_DIR)/libici.dylib
else
ICI_MACOS_BUNDLE_HOST=$(prefix)/bin/ici
endif
endif

# Derive ICI_PLUGIN_FILE from ICI_MODULE_NAME if it is defined (the
# modules/Makefile does NOT define it but includes this file).

ifdef ICI_MODULE_NAME

ifeq ($(_UNAME),Darwin)
ICI_PLUGIN_FILE=ici-$(ICI_MODULE_NAME).bundle
endif # Darwin

ifeq ($(_UNAME),FreeBSD)
ICI_PLUGIN_FILE=ici-$(ICI_MODULE_NAME).so
endif # FreeBSD

ifeq ($(_UNAME),Linux)
ICI_PLUGIN_FILE=ici-$(ICI_MODULE_NAME).so
endif # Linux

ICI_PLUGIN_DOT_ICI=ici-$(ICI_MODULE_NAME).ici

endif # ICI_MODULE_NAME

# -*- mode:makefile -*-
#
# This (GNU) Makefile uses the dcc compiler driver
# to build ici on MacOS or FreeBSD.
#

.PHONY: all default lib clean ici test install

os=    $(shell uname|tr A-Z a-z)
sudo?=
prog=  ici
lib=   libici.a
ifeq ($(os),darwin)
dll=   libici.dylib
else
dll=	libici.so
endif
conf?= conf/$(os).h
dest?= /opt/ici

# The 'build' macro controls the type of build.  Uncomment one of the
# following lines to select the desired type of build.
#
# build=dll
#
#   ICI is built as a dynamic library, libici.dylib, and the ici
#   executable is linked against that library.
#
# build=exe
#
#   ICI is built as single, statically linked, executable with no
#   library component.
#
# build=lib
#
#   ICI is built as a static library, libici.a, and the ici executable
#   linked against that library. Similar to build=exe but the library
#   is installed and made available to users.
#

build?=dll
#build?=exe
#build?=lib

ifndef build
build=exe
endif

srcs= $(shell ls *.cc | fgrep -v win32)
hdrs= $(shell ls *.h|fgrep -v ici.h)

ldflags=
ifeq ($(os),darwin)
ldflags=-macosx_version_min 10.12 -framework System
endif

# The 'default' make target tests the interpreter which
# is built if required.
#
default: all

# The 'test' target tests the interpreter by running the standard
# 'core' test.
#
test: all
	./$(prog) test-core.ici

# The 'all' target builds an ici interpreter executable and library,
# if that is enabled.
#
all: $(prog) ici.h

# The ici.h file is built using the current interpreter executable and
# depends on all files that may contribute to the output.
#
ici.h: $(prog) mk-ici-h.ici $(hdrs)
	LD_LIBRARY_PATH=`pwd` DYLD_LIBRARY_PATH=`pwd` ./$(prog) mk-ici-h.ici $(conf)


ifeq ($(build),dll)
# This build variant has the interpreter code in a dynamic library.
#
$(prog): lib
	@dcc etc/main.cc -o $@ -L. -lici

lib:
	@dcc --dll $(dll) -fPIC $(srcs) -lc++ $(libs) $(ldflags)


else ifeq ($(build),exe)
# The 'exe' build builds an executable containing the complete
# interpreter and does not create any library.
#
$(prog):
	@dcc etc/main.cc $(srcs) -o $@

else ifeq ($(build),lib)

# The 'lib' build creates a static library and an executable that is
# linked against that library.
#
$(prog): lib
	@dcc etc/main.cc -o $@ -L. -lici  $(libs)

lib:
	@dcc --lib $(lib) $(srcs)

else
$(error "Bad build - nothing matched!")
endif

clean:;	rm -rf etc/main.o etc/.dcc.d *.o .dcc.d $(prog) ici.h $(dll) $(lib)


# Installation

.PHONY:  install-ici-dot-h install-libici install-ici-exe install-ici-corefiles
install: install-ici-dot-h install-libici install-ici-exe install-ici-corefiles

install-ici-dot-h:
	mkdir -p $(dest)/include
	install -c -m 444 ici.h $(dest)/include
	install -c -m 444 icistr-setup.h $(dest)/include

install-libici:
	mkdir -p $(dest)/lib
ifeq ($(build),lib)
	install -c -m 444 $(lib) $(dest)/lib
else ifeq ($(build),dll)
	install -c -m 444 $(dll) $(dest)/lib
endif

install-ici-exe:
	mkdir -p $(dest)/bin
	install -c -m 555 $(prog) $(dest)/bin

install-ici-corefiles:
	mkdir -p $(dest)/lib/ici
	install -c -m 444 ici-core*.ici $(dest)/lib/ici

.PHONY: full-install
full-install:
	$(MAKE) clean
	$(MAKE) build=dll conf=$(conf)
	$(sudo) $(MAKE) build=dll install dest=$(dest)
	$(MAKE) clean
	$(MAKE) build=lib conf=$(conf)
	$(sudo) $(MAKE) build=lib install-libici install-ici-exe dest=$(dest)
	$(MAKE) clean

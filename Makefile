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
dest?= /usr/local
dccflags?=
cxxflags?=CXXFLAGS

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

#build?=dll
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
	@echo; echo '* CORE ================'; echo;\
	LD_LIBRARY_PATH=`pwd` DYLD_LIBRARY_PATH=`pwd` ./$(prog) test-core.ici
	-@echo; echo ' * TESTS ================'; echo;\
	LD_LIBRARY_PATH=`pwd` DYLD_LIBRARY_PATH=`pwd` $(MAKE) -C test ici=`pwd`/$(prog) test
	-@echo;echo '* SERIALIZATION ================'; echo;\
	LD_LIBRARY_PATH=`pwd` DYLD_LIBRARY_PATH=`pwd` $(MAKE) -C test/serialization ici=`pwd`/$(prog) test

# The 'all' target builds an ici interpreter executable and library,
# if that is enabled.
#
all: $(prog) ici.h

# The ici.h file is built using the current interpreter executable and
# depends on all files that may contribute to the output.
#
ici.h: $(prog) mk-ici-h.ici $(hdrs)
	@LD_LIBRARY_PATH=`pwd` DYLD_LIBRARY_PATH=`pwd` ./$(prog) mk-ici-h.ici $(conf)


ifeq ($(build),dll)
# This build variant has the interpreter code in a dynamic library.
#
$(prog): lib
	@CXXFLAGSFILE=$(cxxflags) dcc $(dccflags) etc/main.cc -o $@ -L. -lici

lib:
	@CXXFLAGSFILE=$(cxxflags) dcc $(dccflags) --dll $(dll) -fPIC $(srcs) -lc++ $(libs) $(ldflags)


else ifeq ($(build),exe)
# The 'exe' build builds an executable containing the complete
# interpreter and does not create any library.
#
$(prog):
	@CXXFLAGSFILE=$(cxxflags) dcc $(dccflags) etc/main.cc $(srcs) -o $@


else ifeq ($(build),lib)
# The 'lib' build creates a static library and an executable that is
# linked against that library.
#
$(prog): lib
	@CXXFLAGSFILE=$(cxxflags) dcc $(dccflags) etc/main.cc -o $@ -L. -lici  $(libs)

lib:
	@CXXFLAGSFILE=$(cxxflags) dcc $(dccflags) --lib $(lib) $(srcs)

else
$(error "$(build) is not a supported build")
endif


# Cleaning

clean:
	rm -rf etc/main.o etc/.dcc.d *.o .dcc.d $(prog) ici.h $(dll) $(lib)
	@$(MAKE) -Ctest clean
	@$(MAKE) -Ctest/serialization clean

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
	@echo '1. make clean'; $(MAKE) -s clean
	@echo '2. build dll'; $(MAKE) -s lib build=dll conf=$(conf) dccflags=--quiet
	@echo '3. install dll'; $(sudo) $(MAKE) -s build=dll install-libici dest=$(dest)
	@echo '4. make clean'; $(MAKE) -s clean
	@echo '5. make build=lib'; $(MAKE) -s build=lib conf=$(conf) dccflags=--quiet
	@echo '6. make install'; $(sudo) $(MAKE) -s build=lib install-libici install-ici-exe dest=$(dest)
	@echo '7. make clean'; $(MAKE) -s clean

.PHONY: debug lto
debug:
	@$(MAKE) all cxxflags=CXXFLAGS.debug dccflags=$(dccflags) build=$(build)

lto:
	@$(MAKE) all cxxflags=CXXFLAGS.lto dccflags=$(dccflags) build=exe

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
	LD_LIBRARY_PATH=`pwd` ICIPATH=`pwd` DYLD_LIBRARY_PATH=`pwd` ./$(prog) test-core.ici
	-@echo; echo ' * TESTS ================'; echo;\
	LD_LIBRARY_PATH=`pwd` ICIPATH=`pwd` DYLD_LIBRARY_PATH=`pwd` $(MAKE) -C test ici=`pwd`/$(prog) test
	-@echo;echo '* SERIALIZATION ================'; echo;\
	LD_LIBRARY_PATH=`pwd` ICIPATH=`pwd` DYLD_LIBRARY_PATH=`pwd` $(MAKE) -C test/serialization ici=`pwd`/$(prog) test

# The 'all' target builds an ici interpreter executable and library,
# if that is enabled.
#
all: $(prog)

# The ici.h file is built using the current interpreter executable and
# depends on all files that may contribute to the output.
#
ici.h: $(prog) mk-ici-h.ici $(hdrs)
	@LD_LIBRARY_PATH=`pwd` ICIPATH=`pwd` DYLD_LIBRARY_PATH=`pwd` ./$(prog) mk-ici-h.ici $(conf)


ifeq ($(build),dll)
# This build variant has the interpreter code in a dynamic library.
#
$(prog): lib
	@CXXFLAGSFILE=$(cxxflags) dcc $(dccflags) etc/main.cc -o $@ -L. -lici

lib:
	@CXXFLAGSFILE=$(cxxflags) dcc $(dccflags) --dll $(dll) -fPIC $(srcs) $(libs) $(ldflags)


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


# Other targets for developer types.
#
.PHONY: debug lto
debug:
	@$(MAKE) all cxxflags=CXXFLAGS.debug dccflags=$(dccflags) build=$(build)

lto:
	@$(MAKE) all cxxflags=CXXFLAGS.lto dccflags="$(dccflags) --quiet" build=exe


# Cleaning

clean:
	@case "$(MAKEFLAGS)" in \
	*s*);; \
	*) echo rm $(prog) $(lib) $(dll) '*.o';\
	esac;\
	rm -rf etc/main.o etc/.dcc.d *.o .dcc.d $(prog) ici.h $(dll) $(lib)
	@$(MAKE) -Ctest clean
	@$(MAKE) -Ctest/serialization clean


# Installation

.PHONY: install-ici-exe install-libici install-ici-dot-h

ifeq ($(build),exe)
install: install-ici-exe
else
install: install-ici-exe install-libici
endif

install-libici: install-ici-dot-h
	$(sudo) mkdir -p $(dest)/lib
ifeq ($(build),lib)
	$(sudo) install -c -m 444 $(lib) $(dest)/lib
else ifeq ($(build),dll)
	$(sudo) install -c -m 444 $(dll) $(dest)/lib
endif

install-ici-dot-h: ici.h
	$(sudo) mkdir -p $(dest)/include
	$(sudo) install -c -m 444 ici.h $(dest)/include
	$(sudo) install -c -m 444 icistr-setup.h $(dest)/include

install-ici-exe:
	$(sudo) mkdir -p $(dest)/bin
	$(sudo) install -c -m 555 $(prog) $(dest)/bin
	$(sudo) mkdir -p $(dest)/lib/ici
	$(sudo) install -c -m 444 ici-core*.ici $(dest)/lib/ici

# Install everything - static and dynamic libs, exe. And on drawin
# build an LTO exe for actual use.
#
.PHONY: full-install
full-install:
	@echo '1  - make clean'; $(MAKE) -s clean
	@echo '2  - build dll'; $(MAKE) -s lib build=dll conf=$(conf) dccflags=--quiet
	@echo '3  - install dll'; $(MAKE) -s build=dll install-libici dest=$(dest)
	@echo '4  - make clean'; $(MAKE) -s clean
	@echo '5  - make build=lib'; $(MAKE) -s build=lib conf=$(conf) dccflags=--quiet
	@echo '6  - make install'; $(MAKE) -s build=lib install-libici install-ici-exe dest=$(dest)
	@echo '7  - make clean'; $(MAKE) -s clean
ifeq ($(os),darwin)
	@echo '8  - make lto'; $(MAKE) -s lto
	@echo '9  - install lto'; $(MAKE) -s install-ici-exe
	@echo '10 - make clean'; $(MAKE) -s clean
endif

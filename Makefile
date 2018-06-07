# -*- mode:makefile -*-
#
# This (GNU) Makefile uses the dcc compiler driver
# to build ici on MacOS, FreeBSD and Linux.
#
# Targets
#
#	all
#
#	ici
#	ici.h
#	lib
#
#	clean
#
#	install
#	install-ici-exe
#	install-libici
#	install-ici-dot-h
#
#	full-install
#
#	debug
#	lto
#	test

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
prefix?= /usr/local
dccflags?=
cxxflags?=CXXFLAGS

.PHONY: all lib clean $(prog) install

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

# Default to building the exe if otherwise unset.
#
ifndef build
build=exe
endif

# On darwin (MacOS) we can build an "lto" executable
# (whole program optimization) that may be faster.
# It isn't really any faster so we don't build it
# by default.
#
install_lto_exe=no

srcs= $(shell ls *.cc | fgrep -v win32)
hdrs= $(shell ls *.h|fgrep -v ici.h)

ldflags=
ifeq ($(os),darwin)
ldflags=-macosx_version_min 10.12 -framework System
endif

# The 'all' target builds an ici interpreter executable and library,
# if that is enabled.
#
all: $(prog)

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
$(error "$(build) is not a supported build type")
endif

# The ici.h file is built using the current interpreter executable and
# depends on all files that may contribute to the output.
#
ici.h: $(prog) mk-ici-h.ici $(hdrs)
	@LD_LIBRARY_PATH=`pwd` ICIPATH=`pwd` DYLD_LIBRARY_PATH=`pwd` ./$(prog) mk-ici-h.ici $(conf)


# Other targets for developer types.
#
.PHONY: debug lto test

# The 'debug' target builds using the .debug CXXFLAGS options file.
#
debug:
	@$(MAKE) all cxxflags=CXXFLAGS.debug dccflags=$(dccflags) build=$(build)

# The 'lto' target builds using the .lto CXXFLAGS options file.
#
lto:
	@$(MAKE) all cxxflags=CXXFLAGS.lto dccflags="$(dccflags) --quiet" build=exe


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
	$(sudo) mkdir -p $(prefix)/lib
ifeq ($(build),lib)
	$(sudo) install -c -m 444 $(lib) $(prefix)/lib
else ifeq ($(build),dll)
	$(sudo) install -c -m 444 $(dll) $(prefix)/lib
endif

install-ici-dot-h: ici.h
	$(sudo) mkdir -p $(prefix)/include
	$(sudo) install -c -m 444 ici.h $(prefix)/include
	$(sudo) install -c -m 444 icistr-setup.h $(prefix)/include

install-ici-exe:
	$(sudo) mkdir -p $(prefix)/bin
	$(sudo) install -c -m 555 $(prog) $(prefix)/bin
	$(sudo) mkdir -p $(prefix)/lib/ici
	$(sudo) install -c -m 444 ici-core*.ici $(prefix)/lib/ici

# Install everything - static and dynamic libs, exe. And on drawin
# build an LTO exe for actual use.
#
.PHONY: full-install
full-install:
	@echo '1  - make clean'; $(MAKE) -s clean
	@echo '2  - build dll'; $(MAKE) -s lib build=dll conf=$(conf) dccflags=--quiet
	@echo '3  - install dll'; $(MAKE) -s build=dll install-libici prefix=$(prefix) dccflags=--quiet
	@echo '4  - make clean'; $(MAKE) -s clean
	@echo '5  - build lib/exe'; $(MAKE) -s build=lib conf=$(conf) dccflags=--quiet
	@echo '6  - install lib/exe'; $(MAKE) -s build=lib install-libici install-ici-exe prefix=$(prefix) dccflags=--quiet
	@echo '7  - make clean'; $(MAKE) -s clean
ifeq ($(install_lto_exe),yes)
	@echo '8  - make lto'; $(MAKE) -s lto
	@echo '9  - install lto'; $(MAKE) -s install-ici-exe
	@echo '10 - make clean'; $(MAKE) -s clean
endif

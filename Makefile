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
#	distclean
#
#	install
#	install-ici-exe
#	install-libici
#	install-ici-dot-h
#	install-core-ici
#
#	full-install
#
#	debug
#	test
#
#	modules
#	clean-modules
#	install-modules
#
# cmake
#
#	with-cmake
#	configure-cmake
#	clean-cmake
#
# xcode
#
#	with-xcode
#	configure-xcode
#	clean-xcode
#

os=		$(shell uname|tr A-Z a-z)
sudo?=
prog=		ici
lib=		libici.a
ifeq ($(os),darwin)
dll=		libici.dylib
else
dll=		libici.so
endif
conf?= 		conf/$(os).h
prefix?= 	/usr/local
dccflags?=
cxxflags?=	CXXFLAGS
libs=
ifeq ($(os),linux)
libs=		-lpthread -ldl
endif
ifeq ($(os),freebsd)
libs=		-lpthread
endif
objdir?=	.dcc.o
cmakebuild?=	Release

.PHONY:		all lib clean $(prog) install distclean modules clean-modules install-modules

# The 'build' macro controls how ICI it built. It can be one of:
#
#   - exe	Build an ici executable, no ICI library is created.
#   - lib	Build a static library, libici.a, and build the ici
#		executable using that library.
#   - dll	Build a dynamic library, libici.so (on ELF platforms),
#		build the ici executable using that library.
#
# The default is 'exe'
#

build?=		exe
#build?=	lib
#build?=	dll

srcs=		$(shell ls *.cc | fgrep -v win32)
hdrs=		$(shell ls *.h | fgrep -v ici.h)

ldflags=
ifeq ($(os),darwin)
ldflags=	-macosx_version_min 10.12 -framework System
endif

# The common prefix for the dcc commands we run
#
dcc=		CXXFLAGSFILE=$(cxxflags) dcc $(dccflags) --objdir $(objdir)

# The 'all' target builds an ici interpreter executable and library,
# if that is enabled.
#
all:		$(prog)


ifeq ($(build),dll)
# This build variant has the interpreter code in a dynamic library.
#
$(prog):	lib
	@[ -d $(objdir) ] || mkdir $(objdir)
	@$(dcc) etc/main.cc -fPIC -o $@ -L. -lici

lib:
	@[ -d $(objdir) ] || mkdir $(objdir)
	@$(dcc) --dll $(dll) -fPIC $(srcs) $(libs) $(ldflags)


else ifeq ($(build),exe)
# The ici target builds an executable containing the complete
# interpreter and does not create any library.
#
$(prog):
	@[ -d $(objdir) ] || mkdir $(objdir)
	@$(dcc) etc/main.cc $(srcs) -o $@


else ifeq ($(build),lib)
# The 'lib' build creates a static library and an executable that is
# linked against that library.
#
$(prog):	lib
	@[ -d $(objdir) ] || mkdir $(objdir)
	@$(dcc) etc/main.cc -o $@ -L. -lici  $(libs)

lib:
	@[ -d $(objdir) ] || mkdir $(objdir)
	@$(dcc) --lib $(lib) $(srcs)

else
$(error "$(build) is not a supported build type")
endif

# The ici.h file is built using the current interpreter executable and
# depends on all files that may contribute to the output.
#
ici.h:		$(prog) mk-ici-h.ici $(hdrs)
	@ICIPATH=`pwd` LD_LIBRARY_PATH=`pwd` DYLD_LIBRARY_PATH=`pwd` ./$(prog) mk-ici-h.ici $(conf)


# Other targets for developer types.
#
.PHONY: debug test

# The 'debug' target builds using the .debug CXXFLAGS options file.
#
debug:
	@$(MAKE) all cxxflags=CXXFLAGS.debug dccflags=$(dccflags) build=$(build)


# The 'test' target tests the interpreter by running the standard
# 'core' test.
#
test:		all
	@echo; echo '* CORE ================'; echo;\
	ICIPATH=`pwd` LD_LIBRARY_PATH=`pwd` DYLD_LIBRARY_PATH=`pwd` ./$(prog) test-core.ici
	-@echo; echo ' * TESTS ================'; echo;\
	ICIPATH=`pwd` LD_LIBRARY_PATH=`pwd` DYLD_LIBRARY_PATH=`pwd` $(MAKE) -C test ici=`pwd`/$(prog) test
	-@echo;echo '* SERIALIZATION ================'; echo;\
	ICIPATH=`pwd` LD_LIBRARY_PATH=`pwd` DYLD_LIBRARY_PATH=`pwd` $(MAKE) -C test/serialization ici=`pwd`/$(prog) test

# Cleaning

# The library file we rm depends on the build type...
#
ifeq ($(build),dll)
rmlib=$(dll)
endif
ifeq ($(build),lib)
rmlib=$(lib)
endif
ifeq ($(build),exe)
rmlib=
endif

clean:	; @case "$(MAKEFLAGS)" in \
	*s*);; *) echo rm -rf $(objdir) $(prog) $(rmlib) ici.h;\
	esac;\
	rm -rf $(objdir) $(prog) $(rmlib) ici.h
	@$(MAKE) -Ctest clean
	@$(MAKE) -Ctest/serialization clean

distclean:	clean
	rm -f *.a *.so *.dylib
	rm -rf .build
	rm -rf .build.xcode

# Installation

.PHONY:		install-ici-exe install-libici install-ici-dot-h install-core-ici

ifeq ($(build),exe)
install:	install-ici-exe
else
install:	install-ici-exe install-libici
endif

install-libici: install-ici-dot-h
ifeq ($(build),lib)
	$(sudo) mkdir -p $(prefix)/lib
	$(sudo) install -c -m 444 $(lib) $(prefix)/lib
	$(MAKE) sudo=$(sudo) install-core-ici
else ifeq ($(build),dll)
	$(sudo) mkdir -p $(prefix)/lib
	$(sudo) install -c -m 444 $(dll) $(prefix)/lib
	$(MAKE) sudo=$(sudo) install-core-ici
endif

install-ici-dot-h: ici.h
	$(sudo) mkdir -p $(prefix)/include
	$(sudo) install -c -m 444 ici.h $(prefix)/include
	$(sudo) install -c -m 444 icistr-setup.h $(prefix)/include

install-ici-exe:
	$(sudo) mkdir -p $(prefix)/bin
	$(sudo) install -c -m 555 $(prog) $(prefix)/bin
ifeq ($(build),exe)
	$(MAKE) sudo=$(sudo) install-ici-dot-h
	$(MAKE) sudo=$(sudo) install-core-ici
endif

install-core-ici:
	$(sudo) mkdir -p $(prefix)/lib/ici
	$(sudo) install -c -m 444 ici-core*.ici $(prefix)/lib/ici

# Install everything - static and dynamic libs, exe.
#
.PHONY: full-install
full-install:
	@echo '1  - make clean'; $(MAKE) -s clean
	@echo '2  - make (lib)'; $(MAKE) -s lib ici.h build=lib conf=$(conf) dccflags="$(dccflags) --quiet"
	@echo '3  - make install (lib)'; $(MAKE) -s build=lib install-libici prefix=$(prefix) dccflags="$(dccflags) --quiet"
	@echo '4  - make clean'; $(MAKE) -s clean
	@echo '5  - make (dll)'; $(MAKE) -s build=dll conf=$(conf) dccflags="$(dccflags) --quiet"
	@echo '6  - make install (dll)'; $(MAKE) -s build=dll install-libici install-ici-exe prefix=$(prefix) dccflags="$(dccflag) --quiet"
	@echo '7  - make clean'; $(MAKE) -s clean

# cmake
#
.PHONY:		with-cmake configure-cmake clean-cmake

with-cmake:	configure-cmake
	@cmake --build .build

configure-cmake:
	@cmake -B.build -H. -GNinja -DCMAKE_BUILD_TYPE=$(cmakebuild)

clean-cmake:
	@[ -d .build ] && ninja -C.build -t clean

# xcode (via cmake)
#
ifeq ($(os),darwin)
.PHONY:		with-xcode configure-xcode clean-xcode

configure-xcode:
	@cmake -B.build.xcode -H. -GXcode

with-xcode:	configure-xcode
	@cmake --build .build.xcode

clean-xcode:
	@rm -rf .build.xcode
endif

# modules
#
modules:
	@$(MAKE) -C modules -$(MAKEFLAGS)

clean-modules:
	@$(MAKE) -C modules -$(MAKEFLAGS) clean

install-modules:
	@$(MAKE) -C modules -$(MAKEFLAGS) install

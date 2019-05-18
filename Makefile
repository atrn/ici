# -*- mode:makefile -*-
#
# This (GNU) Makefile is used to build and install ICI in a variety of
# ways and using a variety of tools.
#
# By default the dcc compiler driver (http://github.com/atrn/dcc) is
# used to take care of compilation and linking.  Alternatively cmake
# or xcode, via cmake, may be used to build.
#
# The following targets are defined:
#
#	all			Makes the ici program, library and
#				associated ici.h file.
#
#	ici			Build the ici executable.
#	lib			Build the ici library.
#	ici.h			Generate the ici.h file.
#
#	clean			Clean build output.
#
#	distclean		Remove all generated files - any dcc,
#				cmake or xcode output, i.e. anything
#				not under version control.
#
#	install			Install everything.
#	install-ici-exe 	Install the executable.
#	install-libici		Install the library.
#	install-ici-dot-h 	Install the ici.h file.
#	install-core-ici 	Install the core ici-core*.ici files.
#
#	full-install		Builds and installs both static and dynamic
#				library variants, ici.h and a dynamically
#				linked executable.
#
#	debug			Set debug options and then build via a
#				recursive make. E.g. $ make debug lib
#
#	test			Run tests.
#
# Modules
#
#	modules			Build the modules.
#	clean-modules		Clean them modules.
#	install-modules		Install the modules.
#
# Cmake
#
#	with-cmake		Run cmake to build ici.
#	configure-cmake		Run cmake to generate build files.
#	clean-cmake		Clean up th cmake build directory.
#
# Xcode
#
#	with-xcode		Run xcodebuild to build ici.
#	configure-xcode		Run cmake to generate an xcode project.
#	clean-xcode		Clean up the xcode build directory.
#
#
# Important Make Macros
#
#	build			Selects the type of build to perform,
#				simple executable, executable plus
#				static library or executable with a
#				dynamic library. See below for more
#				information.
#
#	prefix			The installation prefix, defaulting to
#				/usr/local.
#
#	sudo			Used when installing to prefix the
#				install command. Defaults to the empty
#				string requiring the user doing the
#				install have permission to write to
#				the prefix directory.
#
#	conf			The ici "conf" header file to use when
#				creating ici.h.  The interpreter code
#				selects the conf in fwd.h via platform
#				detection.
#
# The above combine resulting in make invocations such as,
#
#	$ make sudo=sudo prefix=/local build=lib install
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

#
# The 'build' macro controls how ICI it built. It can be one of:
#
#   - dll	Builds ICI as a dynamic library, libici.so (ELF, other
#		names as required by the platform, e.g. ici.dll) and
#		builds an ici executable dynamically linked against
#		that library. Installs an executable, dynamic library
#		and assocated ici.h file.
#
#   - lib	Builds ICI as a static library, libici.a, and statically
#		links main against that library. Installs an executable,
#		static library and associated ici.h file.
#
#   - exe	Builds ICI as an executable with no library.
#
# The default build type is 'dll'.
#

build?=		dll
#build?=	exe
#build?=	lib

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
all:		$(prog) ici.h


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

# GNU make can be very noisy. We use the --no-print-directory switch for
# all sub-makes to quieten it down.
#
_MAKE=	$(MAKE) --no-print-directory

# The 'debug' target builds using the .debug CXXFLAGS options file.
#
debug:
	@$(_MAKE) all cxxflags=CXXFLAGS.debug dccflags=$(dccflags) build=$(build)


# The 'test' target tests the interpreter by running the standard
# 'core' test.
#
test:		$(prog)
	@echo; echo '* CORE ================'; echo;\
	ICIPATH=`pwd` LD_LIBRARY_PATH=`pwd` DYLD_LIBRARY_PATH=`pwd` ./$(prog) test-core.ici
	-@echo; echo ' * TESTS ================'; echo;\
	ICIPATH=`pwd` LD_LIBRARY_PATH=`pwd` DYLD_LIBRARY_PATH=`pwd` $(_MAKE) -C test ici=`pwd`/$(prog) test
	-@echo;echo '* SERIALIZATION ================'; echo;\
	ICIPATH=`pwd` LD_LIBRARY_PATH=`pwd` DYLD_LIBRARY_PATH=`pwd` $(_MAKE) -C test/serialization ici=`pwd`/$(prog) test

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
	@$(_MAKE) -Ctest clean
	@$(_MAKE) -Ctest/serialization clean

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
	$(_MAKE) sudo=$(sudo) install-core-ici
else ifeq ($(build),dll)
	$(sudo) mkdir -p $(prefix)/lib
	$(sudo) install -c -m 444 $(dll) $(prefix)/lib
	$(_MAKE) sudo=$(sudo) install-core-ici
endif

install-ici-dot-h: ici.h
	$(sudo) mkdir -p $(prefix)/include
	$(sudo) install -c -m 444 ici.h $(prefix)/include
	$(sudo) install -c -m 444 icistr-setup.h $(prefix)/include

install-ici-exe:
	$(sudo) mkdir -p $(prefix)/bin
	$(sudo) install -c -m 555 $(prog) $(prefix)/bin
ifeq ($(build),exe)
	$(_MAKE) sudo=$(sudo) install-ici-dot-h
	$(_MAKE) sudo=$(sudo) install-core-ici
endif

install-core-ici:
	$(sudo) mkdir -p $(prefix)/lib/ici
	$(sudo) install -c -m 444 ici-core*.ici $(prefix)/lib/ici

# Install everything - static and dynamic libs, exe.
#
.PHONY: full-install
full-install:
	@echo '1  - make clean'; $(_MAKE) -s clean
	@echo '2  - make (lib)'; $(_MAKE) -s lib ici.h build=lib conf=$(conf) dccflags="$(dccflags) --quiet"
	@echo '3  - make install (lib)'; $(_MAKE) -s build=lib install-libici prefix=$(prefix) dccflags="$(dccflags) --quiet"
	@echo '4  - make clean'; $(_MAKE) -s clean
	@echo '5  - make (dll)'; $(_MAKE) -s build=dll conf=$(conf) dccflags="$(dccflags) --quiet"
	@echo '6  - make install (dll)'; $(_MAKE) -s build=dll install-libici install-ici-exe prefix=$(prefix) dccflags="$(dccflag) --quiet"
	@echo '7  - make clean'; $(_MAKE) -s clean

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
	@$(_MAKE) -C modules -$(MAKEFLAGS)

clean-modules:
	@$(_MAKE) -C modules -$(MAKEFLAGS) clean

install-modules:
	@$(_MAKE) -C modules -$(MAKEFLAGS) install

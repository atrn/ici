# -*- mode:makefile -*-
#
#
# This (GNU) Makefile is used to build and install ICI.
#
# The makefile supports building ICI in a variety of ways and even
# allows for other tools to be used to build ICI.
#
#
# ** Prequisites
#
# The Makefile uses the dcc compiler driver for compilation, linking
# and library creation. See http://github.com/atrn/dcc for details.
#
# Alternatively cmake may be used to generate build files (using the
# ninja build tool by default) and, on MacOS, for Xcode.
#
#
# ** Targets
#
# The following targets are defined, the default is "all":
#
#	all			Makes the ici program and library
#				(if required) and uses that ici to
#				create the ici.h file.
#
#	ici			Builds the ici executable.
#	lib			Builds the ici library.
#	ici.h			Generates the ici.h file.
#
#	clean			Deletes build output.
#
#	distclean		Removes ALL generated files, all
#				dcc, cmake and xcode output is
#				deleted.
#
#	install			Installs everything.
#	install-ici-exe 	Installs the executable.
#	install-libici		Installs the library.
#	install-ici-dot-h 	Installs the ici.h file.
#	install-core-ici 	Installs the core ici-core*.ici files.
#
#	full-install		Builds and installs both static and dynamic
#				library variants, ici.h, a dynamically
#				linked executable and the modules.
#
#	debug			Set debug options and then build via a
#				recursive make. E.g. $ make debug lib
#
#	test			Run tests.
#
#	modules			Build the modules.
#	clean-modules		Clean them modules.
#	install-modules		Install the modules.
#
#	with-cmake		Run cmake to build ici.
#	configure-cmake		Run cmake to generate build files.
#	clean-cmake		Clean up th cmake build directory.
#
#	with-xcode		Run xcodebuild to build ici.
#	configure-xcode		Run cmake to generate an xcode project.
#	clean-xcode		Clean up the xcode build directory.
#
#
# ** Build Type
#
# The 'build' macro controls how ICI it built and may be one of
# the following:
#
#	dll			Builds ICI as a dynamic library, libici.so
#				(libici.dylib on MacOS, ici.dll on Windows)
#				and links an ici executable against that
#				library. Installs the executable, dynamic
#				library and associated ici.h file.
#
#   	lib			Builds ICI as a static library, libici.a,
#				using itto create the ici executable.
#				Installs the executable, static library
#				and its associated ici.h file.
#
#   	exe			Builds ICI as an executable directly from
#				object files with no library. Installs the
#				executable and an ici.h file but no library.
#
# The default build type is 'dll'.
#
#
# ** Other Macros
#
#	prefix			Where ICI installs, defaults to "/usr/local".
#
#	sudo			Used when installing to prefix the install
#				command. Defaults to the empty string which
#				means the user doing the install must have
#				sufficient permissions to write to the prefix
#				directory.
#
#	conf			The ici "conf" header file to use when creating
#				the ici.h file. Note, the interpreter code does
#				NOT use this! It selects the conf file using
#				platform detection at the top of fwd.h.
#
#	cmakebuild		Sets cmake's CMAKE_BUILD_TYPE. The default
#				is "Release".
#
#	cmakegenerator		The so-called "generator" used by cmake to
#				generate build files. The default is "Ninja"
#				which generates files for the ninja build tool.
#
#	cmakedir		Directory where cmake write its files.
#
#	xcodedir		Directory where cmake writes the Xcode project.
#
#
# The above combine allowing for commands such as,
#
#	$ make sudo=sudo prefix=/local build=dll install
#
#


os=			$(shell uname|tr A-Z a-z)
prog=			ici
lib=			libici.a
dll=			libici.so
ifeq ($(os),darwin)
dll=			libici.dylib
endif
conf?= 			conf/$(os).h
prefix?= 		/usr/local
sudo?=
dccflags?=
cxxflags?=		CXXFLAGS
objdir?=		.dcc.o

cmakebuild?=		Release
cmakegenerator?=	Ninja
cmakedir?=		.build
xcodedir?=		.xcode

.PHONY:			all $(prog) lib clean distclean install
.PHONY:			modules clean-modules install-modules

build?=			dll
#build?=		exe
#build?=		lib

srcs=			$(shell ls *.cc | fgrep -v win32)
hdrs=			$(shell ls *.h | fgrep -v ici.h)

# The common prefix for the dcc commands we run
#
dcc=			CXXFLAGSFILE=$(cxxflags) dcc $(dccflags) --objdir $(objdir)

# The 'all' target builds an ici interpreter executable and library,
# if that is enabled.
#
all:			$(prog) ici.h


ifeq ($(build),dll)
# This build variant has the interpreter code in a dynamic library.
#
$(prog):		lib
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
$(prog):		lib
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
.PHONY:		debug test

# GNU make can be very noisy. We use the --no-print-directory switch for
# all sub-makes to quieten it down.
#
_MAKE=		$(MAKE) --no-print-directory

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
	rm -rf $(cmakedir) $(xcodedir)

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
	@echo '7  - make modules'; $(_MAKE) -s clean-modules; $(_MAKE) -s modules
	@echo '8  - make install modules'; $(_MAKE) -s install-modules
	@echo '9  - make clean'; $(_MAKE) -s clean; $(_MAKE) -s clean-modules

# cmake
#
.PHONY:		with-cmake configure-cmake clean-cmake

with-cmake:	configure-cmake
	@cmake --build $(cmakedir)

configure-cmake:
ifneq ($(cmakebuild),)
	@cmake -B$(cmakedir) -H. -G'$(cmakegenerator)' -DCMAKE_BUILD_TYPE='$(cmakebuild)'
else
	@cmake -B$(cmakedir) -H. -G'$(cmakegenerator)'
endif

clean-cmake:
	@[ -d $(cmakedir) ] && cmake --build $(cmakedir) --target clean

# xcode (via cmake)
#
ifeq ($(os),darwin)
.PHONY:		with-xcode configure-xcode clean-xcode

configure-xcode:
	@$(MAKE) configure-cmake cmakedir=$(xcodedir) cmakegenerator=Xcode cmakebuild=

with-xcode:
	@$(MAKE) with-cmake cmakedir=$(xcodedir) cmakegenerator=Xcode cmakebuild=

clean-xcode:
	@$(MAKE) clean-cmake cmakedir=$(xcodedir)
endif

# modules
#
modules:
	@$(_MAKE) -C modules -$(MAKEFLAGS)

clean-modules:
	@$(_MAKE) -C modules -$(MAKEFLAGS) clean

install-modules:
	@$(_MAKE) -C modules -$(MAKEFLAGS) install

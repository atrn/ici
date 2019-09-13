# -*- mode:makefile -*-
#
# This (GNU) Makefile is used to build and install ICI on
# MacOS, FreeBSD and Linux.
#
# The makefile supports building ICI in number of different ways
# and even allows you to use other tools to build ICI.
#
# But first, you need a few tools... (it wouldn't be a _real_
# project unless building it was simple!)
#
# ** Prequisites
#
# You need one of two basic tools to proceed. You can either use
# my make-like compiler _driver_, `dcc` or you can use `cmake` to
# generate build files for some actual build tool or environment.
#
#
# *** dcc
#
# This Makefile assumes dcc is being used. Dcc takes care of
# compilation, linking and library creation (and all dependency
# checking).
#
# Dcc is on github, http://github.com/atrn/dcc, and is written in the
# Go programming language. If you have Go, dcc is installed via,
#
#	$ go get github.com/atrn/dcc
#
#
# *** cmake
#
# Alternatively cmake may be used to generate build files (using the
# ninja build tool by default) and, on MacOS, for Xcode.
#
#
# ** Make Targets
#
# The following targets are defined, the default being "all":
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
#				dcc, cmake and xcode output is,
#				forcibly, deleted.
#
#	install			Installs everything.
#	install-ici-exe 	Installs the executable.
#	install-libici		Installs the library.
#	install-ici-dot-h 	Installs the ici.h file.
#	install-core-ici 	Installs the core ici-core*.ici files.
#
#	full-install		Builds and installs static and dynamic
#				libraries, the ici.h header file and a,
#				dynamically linked, executable.
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
# ** Build Types
#
# The 'build' macro controls how ICI it built. It can be one of,
#
#   	lib			Builds ICI as a static library, libici.a,
#				and uses it to create an ici executable.
#				Installs the executable, static library
#				and its associated ici.h file.
#
#	dll			Builds ICI as a dynamic library and uses
#				it to create the ici executable. Installs
#				the executable, dynamic library and its
#				associated ici.h file.
#
#   	exe			Builds ICI as an executable directly from
#				object files with no library. Installs the
#				executable and an ici.h file.
#
#
# The default build type is 'lib'.
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
#	cmakeargs		Extra arguments passed to cmake when it is
#				is run to configure thing for the specific
#				generator (ninja by default).
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
# All the above combine allowing for commands such as,
#
#	$ make sudo=sudo prefix=/opt/ici build=dll install
#


os=			$(shell uname|tr A-Z a-z)

prog=			ici
conf?= 			conf/$(os).h
lib=			libici.a
ifeq ($(os),darwin)
dll=			libici.dylib
else
dll=			libici.so
endif

build?=			dll
#build?=		lib
#build?=		exe

prefix?= 		/usr/local	# Duplicated in LDFLAGS.darwin
sudo?=

dccflags?=
cxxflags?=		CXXFLAGS
objdir?=		.objs

cmakeargs?=
cmakebuild?=		Release
cmakegenerator?=	Ninja
cmakedir?=		build
xcodedir?=		.xcode


# ###############################################################
#
# Helper macros and setup
#

_srcs=			$(shell ls *.cc | fgrep -v win32)
_hdrs=			$(shell ls *.h | fgrep -v ici.h)
_dcc=			CXXFLAGSFILE=$(cxxflags) dcc $(dccflags) --objdir $(objdir)
_make=			$(MAKE) --no-print-directory

# ###############################################################
#
# Targets
#

.PHONY:			all $(prog) lib clean distclean debug test
.PHONY:			install full-install install-ici-exe
.PHONY:			install-libici install-ici-dot-h install-core-ici
.PHONY:			modules clean-modules install-modules
.PHONY:			with-cmake configure-cmake clean-cmake


all:			$(prog) ici.h

ifeq ($(build),dll)
#
# Build ICI dynamic library
#
$(prog):		lib
	@[ -d $(objdir) ] || mkdir $(objdir)
	@$(_dcc) etc/main.cc -fPIC -o $@ -L. -lici

lib:
	@[ -d $(objdir) ] || mkdir $(objdir)
	@$(_dcc) --dll $(dll) -fPIC $(_srcs)


else ifeq ($(build),exe)

#
# The ici target builds an executable containing the complete
# interpreter and does not create any library.
#
$(prog):
	@[ -d $(objdir) ] || mkdir $(objdir)
	@$(_dcc) etc/main.cc $(_srcs) -o $@


else ifeq ($(build),lib)


#
# The 'lib' build creates a static library and an executable that is
# linked against that library.
#
$(prog):		lib
	@[ -d $(objdir) ] || mkdir $(objdir)
	@$(_dcc) etc/main.cc -o $@ -L. -lici

lib:
	@[ -d $(objdir) ] || mkdir $(objdir)
	@$(_dcc) --lib $(lib) $(_srcs)

else
# Unknown $(build)
#
$(error "$(build) is not a supported build type")
endif


#
# The ici.h file is built using the current interpreter executable to
# run a program that reads the ICI header files to extract the public
# parts. So ici.h depends on all these.
#
ici.h:		$(prog) mk-ici-h.ici $(_hdrs)
	@ICIPATH=`pwd` LD_LIBRARY_PATH=`pwd` DYLD_LIBRARY_PATH=`pwd` ./$(prog) mk-ici-h.ici $(conf)


#
# The 'debug' target builds the ici executable using the .debug CXXFLAGS options file.
#
debug:
	@$(_make) $(prog) cxxflags=CXXFLAGS.debug dccflags=$(dccflags) build=$(build)


#
# The 'test' target tests the interpreter by running the standard
# 'core' test.
#
test:		$(prog)
	@echo; echo '* CORE ----------------------------------------------------------------'; echo;\
	ICIPATH=`pwd` LD_LIBRARY_PATH=`pwd` DYLD_LIBRARY_PATH=`pwd` ./$(prog) test-core.ici
	-@echo; echo ' * TESTS ----------------------------------------------------------------'; echo;\
	ICIPATH=`pwd` LD_LIBRARY_PATH=`pwd` DYLD_LIBRARY_PATH=`pwd` $(_make) -C test ici=`pwd`/$(prog) test
	-@echo;echo '* SERIALIZATION ----------------------------------------------------------------'; echo;\
	ICIPATH=`pwd` LD_LIBRARY_PATH=`pwd` DYLD_LIBRARY_PATH=`pwd` $(_make) -C test/serialization ici=`pwd`/$(prog) test



# ###############################################################
#
# Cleaning
#

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
	@$(_make) -Ctest clean
	@$(_make) -Ctest/serialization clean

distclean:	clean
	rm -f *.a *.so *.dylib
	rm -rf $(cmakedir) $(xcodedir) build



# ###############################################################
#
# Installing
#

ifeq ($(build),exe)
install:	install-ici-exe
else
install:	install-ici-exe install-libici
endif

install-libici: install-ici-dot-h
ifeq ($(build),lib)
	$(sudo) mkdir -p $(prefix)/lib
	$(sudo) install -c -m 444 $(lib) $(prefix)/lib
	$(_make) sudo=$(sudo) install-core-ici
else ifeq ($(build),dll)
	$(sudo) mkdir -p $(prefix)/lib
	$(sudo) install -c -m 444 $(dll) $(prefix)/lib
	$(_make) sudo=$(sudo) install-core-ici
endif

install-ici-dot-h: ici.h
	$(sudo) mkdir -p $(prefix)/include
	$(sudo) install -c -m 444 ici.h $(prefix)/include
	$(sudo) install -c -m 444 icistr-setup.h $(prefix)/include

install-ici-exe:
	$(sudo) mkdir -p $(prefix)/bin
	$(sudo) install -c -m 555 $(prog) $(prefix)/bin
ifeq ($(build),exe)
	$(_make) sudo=$(sudo) install-ici-dot-h
	$(_make) sudo=$(sudo) install-core-ici
endif

install-core-ici:
	$(sudo) mkdir -p $(prefix)/lib/ici
	$(sudo) install -c -m 444 ici-core*.ici $(prefix)/lib/ici



#
# Install everything - static and dynamic libs, exe.
#
full-install:
	@echo '1  - make clean'; $(_make) -s clean
	@echo '2  - make (lib)'; $(_make) -s lib ici.h build=lib conf=$(conf) dccflags="$(dccflags) --quiet"
	@echo '3  - make install (lib)'; $(_make) -s build=lib install-libici prefix=$(prefix) dccflags="$(dccflags) --quiet"
	@echo '4  - make clean'; $(_make) -s clean
	@echo '5  - make (dll)'; $(_make) -s build=dll conf=$(conf) dccflags="$(dccflags) --quiet"
	@echo '6  - make install (dll)'; $(_make) -s build=dll install-libici install-ici-exe prefix=$(prefix) dccflags="$(dccflag) --quiet"
	@echo '7  - make clean'; $(_make) -s clean

# And when the modules are more stable, re-enable the following...
#
#	@echo '8  - make modules'; $(_make) -s clean-modules; $(_make) -s modules
#	@echo '9  - make install modules'; $(_make) -s install-modules
#	@echo '10 - make clean modules'; $(_make) -s clean-modules
#


# ###############################################################
#
# Building with cmake
#

with-cmake:	configure-cmake
	@cmake --build $(cmakedir)

configure-cmake:
ifneq ($(cmakebuild),)
	@cmake -B$(cmakedir) -H. -G'$(cmakegenerator)' -DCMAKE_BUILD_TYPE='$(cmakebuild)' $(cmakeargs)
else
	@cmake -B$(cmakedir) -H. -G'$(cmakegenerator)' $(cmakeargs)
endif

clean-cmake:
	@[ -d $(cmakedir) ] && cmake --build $(cmakedir) --target clean


ifeq ($(os),darwin)


# ###############################################################
#
# Building with xcode (via cmake)
#

.PHONY:		with-xcode configure-xcode clean-xcode

configure-xcode:
	@$(MAKE) configure-cmake cmakedir=$(xcodedir) cmakegenerator=Xcode cmakebuild=

with-xcode:
	@$(MAKE) with-cmake cmakedir=$(xcodedir) cmakegenerator=Xcode cmakebuild=

clean-xcode:
	@[ -d .build.xcode ] && cmake --build .build.xcode --target clean
endif


# ###############################################################
#
# Modules
#

modules:
	@$(_make) -C modules -$(MAKEFLAGS)

clean-modules:
	@$(_make) -C modules -$(MAKEFLAGS) clean

install-modules:
	@$(_make) -C modules -$(MAKEFLAGS) sudo=$(sudo) prefix=$(prefix) install


# EOF

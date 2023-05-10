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
#	$ go install github.com/atrn/dcc@latest
#
# **** environment variables for dcc builds
#
# ICI_DEBUG_BUILD		Enable the debug build by
#				disabling optimization and
#				not defining NDEBUG.
#
# ICI_WITH_IPP			Use Intel's IPP for vecXX types.
#				Assumes IPPROOT is defined (via
#				the Intel set up scripts).
#
#				ICI_WITH_IPP is automatically set
#				if IPPROOT is defined.
#
# ICI_NO_IPP			Do NOT use IPP even if IPPROOT
#				is defined.
#
# ICI_BUILD_TYPE_DLL		Set when compiling the shared
#				object/DLL - used to enable
#				position independent code.
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
#	distclean-modules	Distclean them modules.
#	install-modules		Install the modules.
#
#	modules-clean		Same as clean-modules
#	modules-distclean	Same as distclean-modules
#	modules-install		Same as install-modules
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
# The 'buildtype' macro controls how ICI is built. It can be one of,
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
#   	lib			Builds ICI as a static library, libici.a,
#				and uses it to create an ici executable.
#				Installs the executable, static library
#				and its associated ici.h file.
#
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
#	inplace			If defined, build modules against this directory's
#				ici.h, executable and dynamic library.  Otherwise
#				build against the installed versions of these
#				files.
#
#	silent			Normally defined as @ and used as a prefix to
#				commands to stop make not echoing the command.
#				Define it to be empty to observe the commands.
#				Other values will have undefined, and probably
#				unwanted, results.
#
# All the above combine allowing for commands such as,
#
#	$ make sudo=sudo prefix=/opt/ici buildtype=dll install
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

buildtype?=		dll
#buildtype?=		lib
#buildtype?=		exe

prefix?= 		/usr/local
sudo?=

njobs?=
dccflags?=		--write-compile-commands $(njobs)
objdir?=		.objs

cmakeargs?=
cmakebuild?=		Release
cmakegenerator?=	Ninja
cmakedir?=		build
xcodedir?=		.xcode
inplace?=		YES
silent?=		@

# ###############################################################
#
# IPP
#
ifdef IPPROOT
ifndef ICI_NO_IPP
export ICI_WITH_IPP=YES
endif
endif

# ###############################################################
#
# Helper macros and setup
#

_srcs=			$(shell ls *.cc | fgrep -v win32)
_hdrs=			$(shell ls *.h | fgrep -v ici.h)
_dcc=			PREFIX=$(prefix) dcc $(dccflags) --objdir $(objdir)
_make=			$(MAKE) --no-print-directory

# ###############################################################
#
# Targets
#

.PHONY:			all $(prog) lib clean distclean debug test
.PHONY:			install full-install install-ici-exe
.PHONY:			install-libici install-ici-dot-h install-core-ici
.PHONY:			modules clean-modules distclean-modules install-modules
.PHONY:			modules-clean modules-distclean modules-install
.PHONY:			with-cmake configure-cmake clean-cmake


# ###############################################################
#
# Default target builds the executable, ici.h header file and
# the modules.
#

all:			$(prog) ici.h modules


ifeq ($(buildtype),dll)
#
# The ici target builds an executable linked against the ICI dynamic library.
#
# The lib target builds the dynamic library.
#
$(prog): lib
	$(silent) [ -d $(objdir) ] || mkdir $(objdir)
	$(silent) $(_dcc) --append-compile-commands etc/main.cc -fPIC -o $@ -L. -lici

lib:
	$(silent) [ -d $(objdir) ] || mkdir $(objdir)
	$(silent) $(_dcc) --dll $(dll) -fPIC $(_srcs)


else ifeq ($(buildtype),exe)
#
# The ici target builds an executable not linked against a library.
#
# There is no lib target.
#
$(prog):
	$(silent) [ -d $(objdir) ] || mkdir $(objdir)
	$(silent) $(_dcc) etc/main.cc $(_srcs) -o $@


else ifeq ($(buildtype),lib)
#
# The ici target builds an executable linked against the ICI static library.
#
# The lib target builds the static library.
#
$(prog):		lib
	$(silent) [ -d $(objdir) ] || mkdir $(objdir)
	$(silent) $(_dcc) --append-compile-commands etc/main.cc -o $@ -L. -lici

lib:
	$(silent) [ -d $(objdir) ] || mkdir $(objdir)
	$(silent) $(_dcc) --lib $(lib) $(_srcs)

else
# Unknown $(buildtype)
#
$(error "'$(buildtype)' is not a supported build type")
endif


#
# The ici.h file is created using the current interpreter executable
# running a program that reads the ICI header files and extracts the
# public definitions. So ici.h depends on the executable, program
# and header files.
#
ici.h:		$(prog) mk-ici-h.ici $(_hdrs)
	$(silent) ICIPATH=`pwd` LD_LIBRARY_PATH=`pwd` DYLD_LIBRARY_PATH=`pwd` ./$(prog) mk-ici-h.ici $(conf)


#
# The 'debug' target builds in debug mode by settng the ICI_DEBUG_BUILD
# environment variable used by the dcc configuration files to select
# compilation options.
#
debug:
	$(silent) $(_make) $(prog) ICI_DEBUG_BUILD=1 dccflags=$(dccflags) buildtype=$(buildtype)


#
# The 'test' target tests the interpreter by running the standard
# 'core' test.
#
test:		$(prog)
	$(silent) echo; echo '* CORE ----------------------------------------------------------------'; echo;\
	ICIPATH=`pwd` LD_LIBRARY_PATH=`pwd` DYLD_LIBRARY_PATH=`pwd` ./$(prog) test-core.ici
	-$(silent) echo; echo ' * TESTS ----------------------------------------------------------------'; echo;\
	ICIPATH=`pwd` LD_LIBRARY_PATH=`pwd` DYLD_LIBRARY_PATH=`pwd` $(_make) -C test ici=`pwd`/$(prog) test
	-$(silent) echo;echo '* SERIALIZATION ----------------------------------------------------------------'; echo;\
	ICIPATH=`pwd` LD_LIBRARY_PATH=`pwd` DYLD_LIBRARY_PATH=`pwd` $(_make) -C test/serialization ici=`pwd`/$(prog) test



# ###############################################################
#
# Cleaning
#

# The library file we rm depends on the build type...
#
ifeq ($(buildtype),dll)
rmlib=$(dll)
endif
ifeq ($(buildtype),lib)
rmlib=$(lib)
endif
ifeq ($(buildtype),exe)
rmlib=
endif

clean:	clean-modules
	$(silent) rm -rf $(objdir) $(prog) $(rmlib) ici.h
	$(silent) $(_make) -Ctest clean
	$(silent) $(_make) -Ctest/serialization clean

distclean: clean distclean-modules
	$(silent) rm -f *.a *.so *.dylib
	$(silent) rm -rf $(cmakedir) $(xcodedir) build



# ###############################################################
#
# Installing
#

ifeq ($(buildtype),exe)
install:	install-ici-exe
else
install:	install-ici-exe install-libici
endif

install-libici: install-ici-dot-h
ifeq ($(buildtype),lib)
	$(sudo) mkdir -p $(prefix)/lib
	$(sudo) install -c -m 444 $(lib) $(prefix)/lib
	$(_make) sudo=$(sudo) install-core-ici
else ifeq ($(buildtype),dll)
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
ifeq ($(buildtype),exe)
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
	$(silent) echo '1  - make clean'; $(_make) -s clean
	$(silent) echo '2  - make (lib)'; $(_make) -s lib ici.h builtype=lib conf=$(conf) dccflags="$(dccflags) --quiet"
	$(silent) echo '3  - make install (lib)'; $(_make) -s builtype=lib install-libici prefix=$(prefix) dccflags="$(dccflags) --quiet"
	$(silent) echo '4  - make clean'; $(_make) -s clean
	$(silent) echo '5  - make (dll)'; $(_make) -s builtype=dll conf=$(conf) dccflags="$(dccflags) --quiet"
	$(silent) echo '6  - make install (dll)'; $(_make) -s builtype=dll install-libici install-ici-exe prefix=$(prefix) dccflags="$(dccflag) --quiet"
	$(silent) echo '7  - make clean'; $(_make) -s clean

# And when the modules are more stable, re-enable the following...
#
#	$(silent) echo '8  - make modules'; $(_make) -s clean-modules; $(_make) -s modules
#	$(silent) echo '9  - make install modules'; $(_make) -s install-modules
#	$(silent) echo '10 - make clean modules'; $(_make) -s clean-modules
#


# ###############################################################
#
# Building with cmake
#

with-cmake:	configure-cmake
	$(silent) cmake --build $(cmakedir)

configure-cmake:
ifneq ($(cmakebuild),)
	$(silent) cmake -B$(cmakedir) -H. -G'$(cmakegenerator)' -DCMAKE_BUILD_TYPE='$(cmakebuild)' -DCMAKE_INSTALL_PREFIX=$(prefix) $(cmakeargs)
else
	$(silent) cmake -B$(cmakedir) -H. -G'$(cmakegenerator)' -DCMAKE_INSTALL_PREFIX=$(prefix) $(cmakeargs)
endif

clean-cmake:
	$(silent) [ -d $(cmakedir) ] && cmake --build $(cmakedir) --target clean


ifeq ($(os),darwin)


# ###############################################################
#
# Building with xcode (via cmake)
#

.PHONY:		with-xcode configure-xcode clean-xcode

configure-xcode:
	$(silent) $(MAKE) configure-cmake cmakedir=$(xcodedir) cmakegenerator=Xcode cmakebuild=

with-xcode:
	$(silent) $(MAKE) with-cmake cmakedir=$(xcodedir) cmakegenerator=Xcode cmakebuild=

clean-xcode:
	$(silent) [ -d .build.xcode ] && cmake --build .build.xcode --target clean
endif


# ###############################################################
#
# Modules
#

ifeq ($(inplace),)
  ifeq ($(buildtype),dll)
_modules_depends = $(prefix)/lib/$(dll)
_make_modules = $(_make) -C modules \
	ICI_DOT_H_DIR=$(prefix)/include \
	ICI_LIB_DIR=$(prefix)/lib \
	ICI_MACOS_BUNDLE_HOST=$(prefix)/lib/$(dll) \
	ICI_BUILD_TYPE_DLL=1
  else
_modules_depends = $(prefix)/bin/$(prog)
_make_modules = $(_make) -C modules \
	ICI_DOT_H_DIR=$(prefix)/include \
	ICI_LIB_DIR=$(prefix)/lib \
	ICI_MACOS_BUNDLE_HOST=$(prefix)/bin/$(prog)
  endif
else
  ifeq ($(buildtype),dll)
  _modules_depends = $(dll)
_make_modules = $(_make) -C modules \
	ICI_DOT_H_DIR=$(PWD) \
	ICI_LIB_DIR=$(PWD) \
	ICI_MACOS_BUNDLE_HOST=$(PWD)/$(dll) \
	ICI_BUILD_TYPE_DLL=1
  else
_modules_depends = $(prog)
_make_modules = $(_make) -C modules \
	ICI_DOT_H_DIR=$(PWD) \
	ICI_LIB_DIR=$(PWD) \
	ICI_MACOS_BUNDLE_HOST=$(PWD)/$(prog)
  endif
endif

modules: $(_modules_depends)
	$(silent) $(_make_modules)

clean-modules modules-clean:
	$(silent) $(_make_modules) clean

distclean-modules modules-distclean:
	$(silent) $(_make_modules) distclean

install-modules modules-install:
	$(silent) $(_make_modules) sudo=$(sudo) prefix=$(prefix) install

# EOF

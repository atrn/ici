# -*- mode:makefile -*-
#
# This (GNU) Makefile uses the dcc compiler driver
# to build ici on MacOS.
#

.PHONY: all default lib clean ici test install

prog=  ici
lib=   libici.a
dll=   libici.dylib
conf?= conf/darwin.h

dest?= /opt/ici

# The 'static' macro controls the type of build.  Uncomment one of the
# following lines to select the desired build.
#
# static=no
#
# ICI is built as a dynamic library and an executable linked against
# that library.
#
# static=exe
#
# ICI is built as single, statically linked, executable with no
# library.
#
# static=lib
#
# ICI is built as a static library and a executable linked against
# that library.
#

#static=no
#static=exe
static=lib


# default to dynamic lib
ifndef static
static=no
endif


srcs= $(shell ls *.cc | fgrep -v win32)
hdrs= $(shell ls *.h|fgrep -v ici.h)
libs= -framework System

# The 'default' make target builds the ici interpreter, tests it
# then creates the ici.h header file.
#
default: test ici.h

# The 'test' target tests the interpreter by running the standard
# 'core' test.
#
test: all
	./$(prog) test-core.ici

# The 'all' target builds an ici interpreter executable and library,
# if that is enabled.
#
all: $(prog)

# The ici.h file is built using the current interpreter executable and
# depends on all files that may contribute to the output.
#
ici.h: $(prog) mk-ici-h.ici $(hdrs)
	./$(prog) mk-ici-h.ici $(conf)


ifeq ($(static),no)
# This build variant has the interpreter code in a dynamic library.
#
$(prog): lib
	@dcc etc/main.cc -o $@ -L. -lici

lib:
	@dcc --dll $(dll) -fPIC $(srcs) $(libs) -macosx_version_min 10.12

else ifeq ($(static),exe)

# The static 'exe' build builds an executable containing the complete
# interpreter and no library.
#
$(prog):
	@dcc etc/main.cc $(srcs) -o $@

else ifeq ($(static),lib)

# The static 'lib' build builds a static library and executable linked
# against it.
#
$(prog): lib
	@dcc etc/main.cc -o $@ -L. -lici  $(libs)

lib:
	@dcc --lib $(lib) $(srcs)

else
$(error "Nothing matched!")
endif

clean:;	@rm -rf etc/main.o *.o *.o.d $(prog) ici.h $(dll) $(lib) .dcc


.PHONY: install-ici-dot-h install-libici install-ici-exe
install: install-ici-dot-h install-libici install-ici-exe

install-ici-dot-h : ici.h
	mkdir -p $(dest)/include
	install -c -m 444 ici.h $(dest)/include
	install -c -m 444 icistr-setup.h $(dest)/include

install-libici: lib
	mkdir -p $(dest)/lib
ifeq ($(static),lib)
	install -c -m 444 $(lib) $(dest)/lib
else ifeq ($(static),exe)
else
	install -c -m 444 $(dll) $(dest)/lib
endif

install-ici-exe: $(prog)
	mkdir -p $(dest)/bin
	install -c -m 555 $(prog) $(dest)/bin
	mkdir -p $(dest)/lib/ici
	install -c -m 444 ici-core*.ici $(dest)/lib/ici

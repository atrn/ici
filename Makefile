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

# Uncomment one of these to build a static executable.
#
# Normally ICI is built as a dynamic library, libici. The ici
# executbale being a trivial executable linked against that library.
#
# If the make macro 'static' is string "exe" an single, statically
# linked, executable is built.
#
# If 'static' is "lib" a static library, libici.a, is created and a
# static executable built using that library.
#

#static=exe
static=lib

#

srcs= $(shell ls *.cc | fgrep -v win32)
hdrs= $(shell ls *.h|fgrep -v ici.h)
libs= -framework System
# -lc++

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


ifndef static

# This build variant has the interpreter code in a dynamic library.
#
$(prog): lib
	@dcc etc/main.cc -o $@ -L. -lici

lib:
	@dcc --dll $(dll) -fPIC $(srcs) $(libs) -macosx_version_min 10.12

else ifeq ($(static),exe)

# The static build builds an executable containing the complete
# interpreter.
#
$(prog):
	@dcc etc/main.cc $(srcs) -o $@

else ifeq ($(static),lib)

$(prog): lib
	@dcc etc/main.cc -o $@ -L. -lici  $(libs)

lib:
	@dcc --lib $(lib) $(srcs)

else
$(error "Nothing matched!")
endif

clean:;	@rm -rf etc/main.o *.o $(prog) ici.h $(dll) $(lib) .dcc


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

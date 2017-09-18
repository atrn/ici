# -*- mode:makefile -*-
#
# This (GNU) Makefile uses the dcc compiler driver
# to build ici on MacOS.
#

.PHONY: all default lib clean ici test install

prog=  ici
dll=   libici.dylib
conf?= conf/darwin.h

dest?= /opt/ici

# uncomment to build static executable with no ICI library
# static=yes

srcs= $(shell echo *.cc)
hdrs= $(shell ls *.h|fgrep -v ici.h)

libs= -framework System -lc++ -macosx_version_min 10.12

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
	@dcc etc/main.cc -o $@ -L. -lici -Wl,-rpath -Wl,$(dest)/lib

lib:
	@dcc --dll $(dll) -fPIC $(srcs) $(libs)

else # static

# The static build builds an executable containing the complete
# interpreter.
#
$(prog):
	@dcc etc/main.cc $(srcs) -o $@

endif

clean:;	@rm -rf etc/main.o *.o $(prog) ici.h $(dll) .dcc


.PHONY: install-ici-dot-h install-libici install-ici-exe
install: install-ici-dot-h install-libici install-ici-exe

install-ici-dot-h : ici.h
	mkdir -p $(dest)/include
	install -c -m 444 ici.h $(dest)/include
	install -c -m 444 icistr-setup.h $(dest)/include

install-libici: lib
	mkdir -p $(dest)/lib
	install -c -m 444 $(dll) $(dest)/lib

install-ici-exe: $(prog)
	mkdir -p $(dest)/bin
	install -c -m 555 $(prog) $(dest)/bin
	mkdir -p $(dest)/lib/ici
	install -c -m 444 ici-core*.ici $(dest)/lib/ici

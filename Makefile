# -*- mode:makefile -*-
#
# This (GNU) Makefile uses the dcc compiler driver
# to build ici on MacOS.
#

.PHONY: all default lib clean ici test

prog=  ici
conf?= conf/darwin.h

# uncomment to build static executable with no ICI library
static=yes

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
	@dcc etc/main.cc -o $@ -L. -lici

lib:
	@dcc --dll libici.dylib -fPIC $(srcs) $(libs)

else # static

# The static build builds an executable containing the complete
# interpreter.
#
$(prog):
	@dcc etc/main.cc $(srcs) -o $@

endif

clean:;	@rm -rf etc/main.o *.o $(prog) ici.h libici.dylib .dcc

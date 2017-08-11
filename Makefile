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

# The 'default' make target builds the ici interpreter and tests it.
#
default: test

# The 'test' target tests the interpreter by running the standard
# 'core' test.
#
test: all
	./$(prog) test-core.ici

# The 'all' target builds an ici interpreter and its public
# interface header file, ici.h.
#
all: ici.h

# The ici.h file depends upon the interpreter executable, the
# script that generates it and the ICI headers that script reads.
#
ici.h: $(prog) mk-ici-h.ici $(hdrs)
	./$(prog) mk-ici-h.ici $(conf)

ifndef static

# This build variant puts the interpreter code into a dynamic library
# use uses a small executable linked against that library.
#
$(prog): lib
	@dcc etc/main.cc -o $@ -L. -lici

lib:
	@dcc --dll libici.dylib -fPIC $(srcs) $(libs)

else

# The static build produces a single executable file.
#
$(prog):
	@dcc etc/main.cc $(srcs) -o $@

endif

clean:;	@rm -rf etc/main.o *.o $(prog) ici.h libici.dylib .dcc

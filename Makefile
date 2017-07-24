# -*- mode:makefile -*-
#
# This (GNU) Makefile uses the dcc compiler driver.
#

.PHONY: all default exe lib clean ici test

prog=  ici
conf?= conf/darwin.h
# uncomment to build static exe with no shared lib
static=yes

srcs= $(shell echo *.cc)
hdrs= $(shell ls *.h|grep -v ici\\.h)
libs= -framework System -lc++ -macosx_version_min 10.12

default: test

test: all
	./$(prog) test-core.ici

all: ici.h

ici.h: exe $(hdrs) mk-ici-h.ici
	./$(prog) mk-ici-h.ici $(conf)

ifndef static
$(prog): lib; @dcc etc/main.cc -o $@ -L. -lici
lib:; @dcc --dll libici.dylib -fPIC $(srcs) $(libs)
exe: $(prog); @ls -l ici libici.dylib; size ici libici.dylib | sed 1d
else
$(prog):; @dcc etc/main.cc $(srcs) -o $@
exe: $(prog); @ls -l $(prog); size $(prog) | sed 1d
endif

clean:;	@rm -rf etc/main.o *.o $(prog) ici.h libici.dylib .dcc

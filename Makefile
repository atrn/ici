# -*- mode:makefile -*-
#
# This (GNU) Makefile uses the dcc compiler driver.
#

.PHONY: all exe lib clean anici test

prog=  anici
conf?= conf/darwin.h

# static exe with no shared lib
static=yes

srcs= $(shell echo *.cc)
hdrs= $(shell ls *.h|grep -v anici\\.h)

libs= -framework System -lc++ -macosx_version_min 10.12

test: all
	./$(prog) test-core.ici

all: anici.h

anici.h: exe $(hdrs) mk-ici-h.ici
	./$(prog) mk-ici-h.ici $(conf)

ifndef static
$(prog): lib; @dcc etc/main.cc -o $@ -L. -lanici
lib:; @dcc --dll libanici.dylib -fPIC $(srcs) $(libs)
exe: $(prog); @ls -l anici libanici.dylib; size anici libanici.dylib | sed 1d
else
$(prog):; @dcc etc/main.cc $(srcs) -o $@
exe: $(prog); @ls -l $(prog); size $(prog) | sed 1d
endif

clean:;	@rm -rf etc/main.o *.o $(prog) anici.h libanici.dylib .dcc

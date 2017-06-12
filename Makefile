# -*- mode:makefile -*-
#
# This (GNU) Makefile uses the dcc compiler driver.
#

.PHONY: all lib clean anici

conf?= conf/darwin.h

srcs=  *.cc
hdrs=  $(shell ls *.h|grep -v anici\\.h)

all:	anici.h

anici.h: anici $(hdrs) mk-ici-h.ici
	./anici mk-ici-h.ici $(conf)

static=yes

ifndef static
anici:	lib; @dcc etc/main.cc -o anici -L. -lanici
lib:;	@dcc --dll libanici.dylib -fPIC $(srcs) -framework System -lc++ -macosx_version_min 10.12
else
anici:;	@dcc $(srcs) etc/main.cc -o $@
endif

clean:;	@rm -rf *.o anici anici.h libanici.dylib .dcc

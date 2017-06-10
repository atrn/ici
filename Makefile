# -*- mode:makefile -*-

.PHONY: all lib clean

conf?= conf/macos.h

srcs=  *.cc macos/*.cc pcre/*.cc
hdrs=  $(shell ls *.h|grep -v anici\\.h)

all: anici.h

anici.h : anici $(hdrs) mk-ici-h.ici
	./anici mk-ici-h.ici $(conf)

anici: lib; @dcc etc/main.cc -o anici -L. -lanici

lib:; @dcc --dll libanici.dylib -fPIC $(srcs) -framework System -lc++ -macosx_version_min 10.12

clean:
	@rm -rf *.o */*.o anici anici.static anici.h libanici.dylib .dcc

anici.static:
	@dcc $(srcs) etc/main.cc -o $@

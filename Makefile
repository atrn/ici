# -*- mode:makefile -*-

.PHONY: all lib clean

SRCS= *.cc macos/*.cc pcre/*.cc

all: anici anici.static
	./anici mk-ici-h.ici conf/macos.h

anici: lib
	dcc etc/main.cc -o anici -L. -lanici

lib:
	@dcc --dll libanici.dylib -fPIC $(SRCS) -framework System -lc++ -macosx_version_min 10.12

clean:
	@rm -rf *.o */*.o anici anici.h libanici.dylib .dcc

anici.static:
	dcc $(SRCS) etc/main.cc -o $@

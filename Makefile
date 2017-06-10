# -*- mode:makefile -*-

.PHONY: all lib clean

SRCS=   *.cc macos/*.cc pcre/*.cc

all: lib
	@dcc etc/main.cc -o anici -L. -lanici &&\
	    ./anici mk-ici-h.ici conf/macos.h

lib:
	@dcc --cpp --dll libanici.dylib -fPIC $(SRCS) -framework System -lc++ -macosx_version_min 10.12

clean:
	@rm -rf *.o */*.o anici anici.h libanici.dylib .dcc

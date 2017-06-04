# -*- mode:makefile -*-

.PHONY: all lib clean

SRCS=   *.c macos/*.c pcre/pcre.c pcre/study.c

all: lib
	@dcc etc/main.cc -o anici -L. -lanici &&\
	    ./anici mk-ici-h.ici conf/macos.h

lib:
	@dcc --dll libanici.dylib -fPIC $(SRCS) -framework System -macosx_version_min 10.12

clean:
	@rm -rf *.o */*.o anici anici.h libanici.dylib .dcc

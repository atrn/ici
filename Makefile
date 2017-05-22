# -*- mode:makefile -*-

.PHONY: exe lib clean

SRCS=   *.c macos/*.c pcre/*.c
OBJS=	$(SRCS:.c=.o)

exe: lib
	@dcc main.o -o anici -L. -lanici &&\
	    ./anici mk-ici-h.ici conf/macos.h

lib:
	@dcc -fPIC -c $(SRCS) &&\
	    libtool -dynamic -o libanici.dylib \
                -macosx_version_min 10.12\
                -framework System \
		$(OBJS)

clean:
	rm -rf *.o */*.o anici anici.h libanici.dylib .dcc

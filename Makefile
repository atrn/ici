# -*- mode:makefile -*-
#
# This (GNU) Makefile uses the dcc compiler driver.
#

.PHONY: all lib clean anici

prog=  anici
conf?= conf/darwin.h
static=yes

srcs=  $(shell echo *.cc)
hdrs=  $(shell ls *.h|grep -v anici\\.h)

libs= -framework System -lc++ -macosx_version_min 10.12

all:	anici.h

anici.h: $(prog) $(hdrs) mk-ici-h.ici
	./$(prog) mk-ici-h.ici $(conf)


ifndef static
$(prog): lib; @dcc etc/main.cc -o $@ -L. -lanici
lib:;	@dcc --dll libanici.dylib -fPIC $(srcs) $(libs)
else
$(prog):; @dcc etc/main.cc $(srcs) -o $@
endif

clean:;	@rm -rf etc/main.o *.o $(prog) anici.h libanici.dylib .dcc

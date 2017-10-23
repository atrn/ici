PREFIX=	/usr/local/
INSTALL=install -c -m 444

CC=	egcc
CFLAGS=	-g -O2 -Wall -I../xmltok

libxmlparse.a:	xmlparse.c
	$(CC) $(CFLAGS) -c xmlparse.c
	$(AR) rv $@ xmlparse.o
	$(RANLIB) $@

libxmlparse.so:	xmlparse.c
	$(CC) $(CFLAGS) -fpic -c xmlparse.c

clean:
	rm -f xmlparse.o core

clobber: clean
	rm -f libxmlparse.a libxmlparse.so .depend

install: libxmlparse.a
	$(INSTALL) libxmlparse.a $(PREFIX)lib
	$(INSTALL) xmlparse.h $(PREFIX)include

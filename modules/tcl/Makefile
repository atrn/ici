#
# This bsd specific makefile was automatically created by
# processing Makefile.bsd to get tcl/Makefile.
#
# See the comments in configure for details.
#
PREFIX	= /usr/local
MOD	= tcl
CFLAGS	= -DBSD_4 -g -O2 -DNDEBUG -Wall -I. -I/usr/local/include -fPIC
LDFLAGS = -shared
OBJS	= tcl.o 
C_SRC	= tcl.c 
ALL_SRC	= $(C_SRC) ici4tcl.ici 
ICIMOD	= ici4tcl.so
LIBS	= 
LIBINSTALLS = $(ICIMOD) ici4tcl.ici 

default: $(ICIMOD)

TCL_PREFIX = /usr/local
X11_PREFIX = /usr/local

TCL_VERSION_DOTTED = 8.5
TCL_VERSION_UNDOTTED = 85

TCL_INCLUDE = $(TCL_PREFIX)/include/tcl$(TCL_VERSION_DOTTED)
X11_INCLUDE = $(X11_PREFIX)/include

TK_INCLUDE = $(TCL_PREFIX)/include/tk$(TCL_VERSION_DOTTED)
TCL_LIBDIR = $(TCL_PREFIX)/lib

CFLAGS += -I$(TCL_INCLUDE) -I$(TK_INCLUDE) -I$(X11_INCLUDE)

LIBS = -L$(TCL_LIBDIR) -ltk$(TCL_VERSION_UNDOTTED) -ltcl$(TCL_VERSION_UNDOTTED) -lX11

$(ICIMOD): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBS) $(LDFLAGS) 

doc: ici$(MOD).1 ici$(MOD).html $(MOD).man.frag $(MOD).synopsis

ici$(MOD).1 $(MOD).man.frag $(MOD).synopsis: $(ALL_SRC)
	ici ../tools/extract-doc-from-src.ici man $(ALL_SRC)

ici$(MOD).html: $(ALL_SRC)
	ici ../tools/extract-doc-from-src.ici html $(ALL_SRC)

clean:
	-rm -f $(OBJS) $(MOD).man.frag $(MOD).html.frag $(MOD).synopsis

clobber: clean
	-rm -f $(ICIMOD) ici$(MOD).1 ici$(MOD).html .depend

depend:
	mkdep $(CFLAGS) $(C_SRC)

test tst:
	ici tst-$(MOD).ici

install: $(LIBINSTALLS)
	install -c -m 444 $(LIBINSTALLS) $(PREFIX)/lib/ici4

install-doc: ici$(MOD).1 ici$(MOD).html
	install -c -m 444 ici$(MOD).1 $(PREFIX)/man/man1

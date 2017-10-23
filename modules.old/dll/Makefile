#
# This bsd specific makefile was automatically created by
# processing Makefile.bsd to get dll/Makefile.
#
# See the comments in configure for details.
#
PREFIX	= /usr/local
MOD	= dll
CFLAGS	= -DBSD_4 -g -O2 -DNDEBUG -Wall -I. -I/usr/local/include -fPIC
LDFLAGS = -shared
OBJS	= dll.o dllfunc.o dllmanager.o 
C_SRC	= dll.c dllfunc.c dllmanager.c 
ALL_SRC	= $(C_SRC) 
ICIMOD	= ici4dll.so
LIBS	= 
LIBINSTALLS = $(ICIMOD) 

default: $(ICIMOD)

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

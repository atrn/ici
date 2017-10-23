#
# This bsd specific makefile was automatically created by
# processing Makefile.bsd to get dbg/Makefile.
#
# See the comments in configure for details.
#
PREFIX	= /usr/local
MOD	= dbg
CFLAGS	= -DBSD_4 -g -O2 -DNDEBUG -Wall -I. -I/usr/local/include -fPIC
LDFLAGS = -shared
OBJS	= dbg.o 
C_SRC	= dbg.c 
ALL_SRC	= $(C_SRC) ici4dbg.ici 
ICIMOD	= ici4dbg.so
LIBS	= 
LIBINSTALLS = $(ICIMOD) ici4dbg.ici 

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

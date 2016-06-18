#
# This bsd specific makefile was automatically created by
# processing Makefile.bsd to get uuid/Makefile.
#
# See the comments in configure for details.
#
PREFIX	= /usr/local
MOD	= uuid
CFLAGS	= -DBSD_4 -g -O2 -DNDEBUG -Wall -I. -I/usr/local/include -fPIC
LDFLAGS = -shared
OBJS	= uuid.o 
C_SRC	= uuid.c 
ALL_SRC	= $(C_SRC) ici4uuid.ici 
ICIMOD	= ici4uuid.so
LIBS	= 
LIBINSTALLS = $(ICIMOD) ici4uuid.ici 

default: $(ICIMOD)

LIBS += -L$(PREFIX)/lib -luuid
CFLAGS += -I$(PREFIX)/include

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

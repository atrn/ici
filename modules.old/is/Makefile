#
# This bsd specific makefile was automatically created by
# processing Makefile.no-c to get is/Makefile.
#
# See the comments in configure for details.
#
PREFIX	= /usr/local
MOD	= is
ALL_SRC	= ici4is.ici 
LIBINSTALLS = ici4is.ici 

default:

doc: ici$(MOD).1 ici$(MOD).html

ici$(MOD).1: $(ALL_SRC)
	ici ../tools/extract-doc-from-src.ici man $(ALL_SRC)

ici$(MOD).html: $(ALL_SRC)
	ici ../tools/extract-doc-from-src.ici html $(ALL_SRC)

clean:

clobber: clean

depend:

test tst:
	ici tst-$(MOD).ici

install: $(LIBINSTALLS)
	install -c -m 444 $(LIBINSTALLS) $(PREFIX)/lib/ici4

install-doc: ici$(MOD).1 ici$(MOD).html
	install -c -m 444 ici$(MOD).1 $(PREFIX)/man/man1

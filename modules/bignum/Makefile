#
# This bsd specific makefile was automatically created by
# processing Makefile.bsd to get bignum/Makefile.
#
# See the comments in configure for details.
#
PREFIX	= /usr/local
MOD	= bignum
CFLAGS	= -DBSD_4 -g -O2 -DNDEBUG -Wall -I. -I/usr/local/include -fPIC
LDFLAGS = -shared
OBJS	= bignum.o 
C_SRC	= bignum.c 
ALL_SRC	= $(C_SRC) ici4bignum.ici 
ICIMOD	= ici4bignum.so
LIBS	= 
LIBINSTALLS = $(ICIMOD) ici4bignum.ici 

default: $(ICIMOD)

# This comes from bignum/Makefile.frag...
CFLAGS += -I. -Imit/h
OBJS = bignum.o  KerN.o bnInit.o \
	bnMult.o bnDivide.o bnCmp.o \
	bzf.o bz.o 

KerN.o:	mit/c/KerN.c
	$(CC) $(CFLAGS) -c mit/c/KerN.c

bnInit.o:	mit/c/bn/bnInit.c
	$(CC) $(CFLAGS) -c mit/c/bn/bnInit.c

bnMult.o:	mit/c/bn/bnMult.c
	$(CC) $(CFLAGS) -c mit/c/bn/bnMult.c

bnDivide.o:	mit/c/bn/bnDivide.c
	$(CC) $(CFLAGS) -c mit/c/bn/bnDivide.c

bnCmp.o:	mit/c/bn/bnCmp.c
	$(CC) $(CFLAGS) -c mit/c/bn/bnCmp.c

bzf.o:	mit/c/bzf.c
	$(CC) $(CFLAGS) -c mit/c/bzf.c

bz.o:	mit/c/bz.c
	$(CC) $(CFLAGS) -c mit/c/bz.c
# end of bignum/Makefile.frag

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

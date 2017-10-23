# This comes from bignum/Makefile.frag...
CFLAGS += -I. -Imit/h
OBJS = $(CF_STAR_DOT_O) KerN.$(CF_O) bnInit.$(CF_O) \
	bnMult.$(CF_O) bnDivide.$(CF_O) bnCmp.$(CF_O) \
	bzf.$(CF_O) bz.$(CF_O) 

KerN.$(CF_O):	mit/c/KerN.c
	$(CC) $(CFLAGS) -c mit/c/KerN.c

bnInit.$(CF_O):	mit/c/bn/bnInit.c
	$(CC) $(CFLAGS) -c mit/c/bn/bnInit.c

bnMult.$(CF_O):	mit/c/bn/bnMult.c
	$(CC) $(CFLAGS) -c mit/c/bn/bnMult.c

bnDivide.$(CF_O):	mit/c/bn/bnDivide.c
	$(CC) $(CFLAGS) -c mit/c/bn/bnDivide.c

bnCmp.$(CF_O):	mit/c/bn/bnCmp.c
	$(CC) $(CFLAGS) -c mit/c/bn/bnCmp.c

bzf.$(CF_O):	mit/c/bzf.c
	$(CC) $(CFLAGS) -c mit/c/bzf.c

bz.$(CF_O):	mit/c/bz.c
	$(CC) $(CFLAGS) -c mit/c/bz.c
# end of bignum/Makefile.frag

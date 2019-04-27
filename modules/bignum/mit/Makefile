#  Copyright     Digital Equipment Corporation & INRIA     1988, 1989
#  Last modified_on Mon May 21 17:12:01 GMT+2:00 1990 by herve 
#       modified_on Tue Apr 10 20:34:44 GMT+2:00 1990 by shand 
#       modified_on Thu Nov  2 14:23:14 GMT+1:00 1989 by gangnet 
#       modified_on Wed Jul  5 10:23:54 GMT+2:00 1989 by bertin 

CC = cc
LIB = BigNum.a
OBJECT = o/KerN.o o/bnInit.o o/bnMult.o o/bnDivide.o o/bnCmp.o o/bzf.o o/bz.o 
KERNH = h/BigNum.h 
CFLAGS = -c `cat .Version` -I./h -O2 -fPIC
LKFLAGS = -O2 #-g

SENDMAIL=/usr/lib/sendmail
SENDMAILFLAGS=

# extra entries:
# all     - make all the stuff
# tidy    - cleanup directories
# scratch - start from scratch

default:
	@echo "Usage: make <version>"
	@echo "see README for valid versions."
	@sh -c 'exit 1'

all: testKerN bztest 

tidy:
	rm -f ,* .,* *~ #*# .emacs_[0-9]* *.BAK *.CKP core a.out
	rm -f */,* */.,* */*~ */#*# */.emacs_[0-9]* */*.BAK */*.CKP

scratch:tidy 
	-[ -d o ] && rm -rf o/
	[ -d o ] || mkdir o

clean:scratch
	rm -f BigNum.a bztest testKerN

# build the BigNum library
lib : $(LIB)

$(LIB): $(OBJECT)
	-rm -f $(LIB)
	ar cr $(LIB) $(OBJECT)
	ranlib $(LIB)

# How to choose the machine dependante version. All produce KerN.o
#	echo "The Default is KerN written in C with digits on 32 bits"
#	echo "-DDIGITon32BITS" >.Version
o/KerN.o: c/KerN.c Makefile
	@$(MAKE) C32 CC=$(CC) CFLAGS="$(CFLAGS)"

C16: scratch
	echo "-DDIGITon16BITS" >.Version
	$(CC) $(CFLAGS) c/KerN.c
	mv KerN.o o
	$(MAKE) CC=$(CC) CFLAGS="$(CFLAGS)" all

C32: scratch
	echo "-DDIGITon32BITS" >.Version
	$(CC) $(CFLAGS) c/KerN.c
	mv KerN.o o
	$(MAKE) CC=$(CC) CFLAGS="$(CFLAGS)" all

68K: scratch
	echo "-DDIGITon32BITS" >.Version
	as s/68KerN.s -o o/KerN.o
	$(MAKE) CC=$(CC) CFLAGS="$(CFLAGS)" all

vax: scratch
	echo "-DDIGITon32BITS" >.Version
	as s/vaxKerN.s -o o/KerN.o
	$(MAKE) CC=$(CC) CFLAGS="$(CFLAGS)" all

rt: scratch
	echo "-DDIGITon32BITS" >.Version
	as s/rtKerN.s -o o/KerN.o
	$(MAKE) CC=$(CC) CFLAGS="$(CFLAGS)" all

ns: scratch
	echo "-DDIGITon32BITS" >.Version
	as s/nsKerN.s -o o/KerN.o
	$(MAKE) CC=$(CC) CFLAGS="$(CFLAGS)" all

mips: scratch
	echo "-DDIGITon32BITS" >.Version
	as -w s/mipsKerN.s -o o/KerN.o
	$(MAKE) CC=$(CC) CFLAGS="$(CFLAGS)" all

pyramid: scratch
	echo "-DDIGITon32BITS" >.Version
	as s/pyramidKerN.s -o o/KerN.o
	$(MAKE) CC=$(CC) CFLAGS="$(CFLAGS)" all

i960: scratch
	echo "-DDIGITon32BITS" >.Version
	as s/i960KerN.s -o o/KerN.o
	$(MAKE) CC=$(CC) CFLAGS="$(CFLAGS)" all

# Construct VMS assembler from UNIX version

s/vaxKerN.mar: s/vaxKerN.s
	sed -f s/unix2vms.sed < s/vaxKerN.s > $@

# Level N
o/bnInit.o: c/bn/bnInit.c $(KERNH)
	$(CC) $(CFLAGS) c/bn/bnInit.c
	mv bnInit.o o

o/bnMult.o: c/bn/bnMult.c $(KERNH)
	$(CC) $(CFLAGS) c/bn/bnMult.c
	mv bnMult.o o

o/bnDivide.o: c/bn/bnDivide.c $(KERNH)
	$(CC) $(CFLAGS) c/bn/bnDivide.c
	mv bnDivide.o o

o/bnCmp.o: c/bn/bnCmp.c $(KERNH)
	$(CC) $(CFLAGS) c/bn/bnCmp.c
	mv bnCmp.o o

# Level Z
o/bz.o: c/bz.c h/BigZ.h $(KERNH)
	$(CC) $(CFLAGS) c/bz.c
	mv bz.o o

# Some functions build with BigZ
o/bzf.o: c/bzf.c h/BigZ.h $(KERNH)
	$(CC) $(CFLAGS) c/bzf.c
	mv bzf.o o

# Tests Of KerN
testKerN: Makefile o/testKerN.o $(LIB)
	$(CC) o/testKerN.o $(LIB) $(LKFLAGS) -o testKerN

o/testKerN.o: c/testKerN.c $(KERNH) h/BntoBnn.h
	$(CC) $(CFLAGS) c/testKerN.c
	mv testKerN.o o

# Tests Of BigZ
bztest: o/bztest.o $(LIB)
	$(CC) o/bztest.o $(LIB) $(LKFLAGS) -o bztest

o/bztest.o: c/bztest.c h/BigZ.h $(KERNH)
	$(CC) $(CFLAGS) c/bztest.c
	mv bztest.o o

# documentation
doc: doc/bn.ps doc/bnf.ps 
docprl: doc/bnprl.ps

doc/bn.dvi: doc/bn.tex doc/bnbody.tex
	cd doc;\
	latex bn;\
	makeindex bn;\
	sed -e "s/\\item Bz/\\newpage \\Bz/g" < bn.ind > bn.index;\
	mv bn.index bn.ind;\
	latex bn;\
	cd ..

doc/bn.ps: doc/bn.dvi
	cd doc;\
	dvips -o bn.ps bn;\
	cd ..

doc/bnf.dvi: doc/bnf.tex
	cd doc;\
	latex bnf;\
	cd ..

doc/bnf.ps: doc/bnf.dvi
	cd doc;\
	dvips -o bnf.ps bnf;\
	cd ..

doc/bnprl.dvi: doc/bnprl.tex doc/bnbody.tex
	cd doc;\
	latex bnprl;\
	makeindex bnprl;\
	sed -e "s/\\item Bz/\\newpage \\Bz/g" < bnprl.ind > bnprl.index;\
	mv bnprl.index bnprl.ind;\
	latex bnprl;\
	cd ..

doc/bnprl.ps: doc/bnprl.dvi
	cd doc;\
	dvips -o bnprl.ps bnprl;\
	cd ..

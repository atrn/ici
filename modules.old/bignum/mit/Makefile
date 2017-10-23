#  Copyright     Digital Equipment Corporation & INRIA     1988, 1989
#  Last modified_on Mon May 21 17:12:01 GMT+2:00 1990 by herve 
#       modified_on Tue Apr 10 20:34:44 GMT+2:00 1990 by shand 
#       modified_on Thu Nov  2 14:23:14 GMT+1:00 1989 by gangnet 
#       modified_on Wed Jul  5 10:23:54 GMT+2:00 1989 by bertin 

CC = cc
LIB = BigNum.a
OBJECT = o/KerN.o o/bnInit.o o/bnMult.o o/bnDivide.o o/bnCmp.o o/bzf.o o/bz.o 
KERNH = h/BigNum.h 
CFLAGS = -c `cat .Version` -I./h -O2 # -g -pg
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
	echo All is done !

tidy:
	rm -f ,* .,* *~ #*# .emacs_[0-9]* *.BAK *.CKP core a.out
	rm -f */,* */.,* */*~ */#*# */.emacs_[0-9]* */*.BAK */*.CKP

scratch:tidy 
	-[ -d o ] && rm -f o/*.o
	[ -d o ] || mkdir o

# build the BigNum library
$(LIB): $(OBJECT)
	-rm -f $(LIB)
	ar cr $(LIB) $(OBJECT)
	ranlib $(LIB)

# How to choose the machine dependante version. All produce KerN.o
o/KerN.o: c/KerN.c Makefile
	echo "The Default is KerN written in C with digits on 32 bits"
	echo "-DDIGITon32BITS" >.Version
	$(MAKE) C32 CC=$(CC) CFLAGS="$(CFLAGS)"

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

# build shell archives
shar: bignum01

PACKET_SIZE=90

# If you modify the list of files in the package kit, don't forget 
# to update README.

bignum01: README Makefile VMSmakefile\
      doc/bn.tex doc/bnbody.tex doc/bnf.tex doc/intro\
      c/KerN.c c/bn/bnInit.c c/bn/bnMult.c c/bn/bnDivide.c\
      c/bn/bnCmp.c c/bz.c c/bzf.c \
      c/bztest.c c/testKerN.c \
      h/BigNum.h h/BigZ.h h/BntoBnn.h \
      s/vaxKerN.s s/68KerN.s s/nsKerN.s s/68KerN_mot.s \
      s/mipsKerN.s s/pyramidKerN.s s/vaxKerN.mar s/unix2vms.sed \
      s/i960KerN.s \
      o/EMPTY
	makekit -s$(PACKET_SIZE)k -nbignum -t"Now do 'make'" \
            README Makefile VMSmakefile\
            doc doc/bn.tex doc/bnbody.tex doc/bnf.tex \
                doc/intro doc/makeidx.sty \
            c c/KerN.c c/bz.c c/bzf.c c/bztest.c c/testKerN.c \
            c/bn c/bn/bnInit.c c/bn/bnMult.c c/bn/bnDivide.c c/bn/bnCmp.c \
            h h/BigNum.h h/BigZ.h h/BntoBnn.h \
            s s/vaxKerN.s s/68KerN.s s/nsKerN.s s/68KerN_mot.s \
              s/mipsKerN.s s/pyramidKerN.s s/vaxKerN.mar s/unix2vms.sed \
              s/i960KerN.s \
            o o/EMPTY

# send shell archives
USER=nil
FULLNAME=nil
COPY=librarian@decprl.dec.com
mail: bignum01
	@sh -c "if [ x$(USER) = xnil -o 'x$(FULLNAME)' = xnil ]; \
	    then echo must specify USER and FULLNAME; \
	         echo EG make USER=herve@prl FULLNAME="'\"'"J-C Herve, Digital PRL"'\"'" mail; exit 1; \
	         else :; fi"
	@touch Recipients
	@echo '' >> Recipients
	@date >> Recipients
	@echo "$(FULLNAME)" >> Recipients
	@echo '<'"$(USER)"'>' >> Recipients
	@echo "To: $(COPY)" >tosend
	@echo "Subject: BIGNUM DAEMON" >>tosend
	@echo "Michele, I have sent the package bignum to:" >>tosend
	@echo >>tosend
	@echo "    $(FULLNAME)" >>tosend
	@echo "    $(USER)" >>tosend
	@echo >>tosend
	@echo "Thanks to register this address in your distribution list." >>tosend
	@$(SENDMAIL) $(SENDMAILFLAGS) $(COPY) <tosend
	echo To: $(USER) > sendmail_header
	cp sendmail_header tosend
	echo "Subject: BigNum package from Digital PRL" >>tosend
	ls bignum[0-9][0-9] | sed -e "s/^bignum0*/    BigNum - Part /" > _flist1
	ls bignum[0-9][0-9] | sed -e "s/^/    /" > _flist2
	ls bignum[0-9][0-9] | sed -e "s/^/    \/bin\/sh /" > _flist3
	ls bignum[0-9][0-9] | sed -e "s/^/    shar -u /" > _flist4
	cc -E -Uvax -I. doc/intro >intro.tosend
	sed \
		-e "s/modified_on/modified_on/g" \
		-e "/doc\/intro/d" \
		-e "/.\/README/d" \
		-e "s/NN/"`ls bignum?? | wc -l | sed -e "s/ //g"`"/g" \
		-e "/^INCLUDE1/r _flist1" \
		-e "/^INCLUDE2/r _flist2" \
		-e "/^INCLUDE3/r _flist3" \
		-e "/^INCLUDE4/r _flist4" \
		-e "/^INCLUDE./d" \
		<intro.tosend >>tosend
	$(SENDMAIL) $(SENDMAILFLAGS) $(USER) <tosend
	for i in `ls bignum[0-9][0-9]`; \
	    do cp sendmail_header tosend; \
	    echo $$i | sed -e "s/^bignum0*/Subject: BigNum - Part /" >>tosend; \
	    echo "# Remove all text above and including this line." >>tosend; \
	    sed -e "s/modified_on/modified_on/g" <$$i >>tosend; \
	    $(SENDMAIL) $(SENDMAILFLAGS) $(USER) <tosend; \
	    done
	rm -f sendmail_header tosend _flist[1-9] bignum[0-9][0-9] intro.tosend


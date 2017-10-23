#
# This bsd specific makefile was automatically created by
# processing Makefile.bsd to get xml/Makefile.
#
# See the comments in configure for details.
#
PREFIX	= /usr/local
MOD	= xml
CFLAGS	= -DBSD_4 -g -O2 -DNDEBUG -Wall -I. -I/usr/local/include -fPIC
LDFLAGS = -shared
OBJS	= xml.o 
C_SRC	= xml.c 
ALL_SRC	= $(C_SRC) ici4xml.ici 
ICIMOD	= ici4xml.so
LIBS	= 
LIBINSTALLS = $(ICIMOD) ici4xml.ici 

default: $(ICIMOD)

# This comes from xml/Makefile.frag...
CFLAGS	= $(CF_PIC) $(CF_OPTIM) -DBSD_4 -g -O2 -DNDEBUG -Wall -I. -I/usr/local/include -Iexpat/xmlparse -Iexpat/xmltok
OBJS	= xml.o xmlparse.o xmltok.o xmlrole.o

xmlparse.o: expat/xmlparse/xmlparse.c
	$(CC) $(CFLAGS) -c expat/xmlparse/xmlparse.c

xmltok.o: expat/xmltok/xmltok.c
	$(CC) $(CFLAGS) -c expat/xmltok/xmltok.c

xmlrole.o: expat/xmltok/xmlrole.c
	$(CC) $(CFLAGS) -c expat/xmltok/xmlrole.c

# end of xml/Makefile.frag

# This comes from xml/Makefile.frag...
CFLAGS	= $(CF_PIC) $(CF_OPTIM) -DBSD_4 -g -O2 -DNDEBUG -Wall -I. -I/usr/local/include -Iexpat/xmlparse -Iexpat/xmltok -fPIC
OBJS	= xml.o xmlparse.o xmltok.o xmlrole.o

xmlparse.o: expat/xmlparse/xmlparse.c
	$(CC) $(CFLAGS) -c expat/xmlparse/xmlparse.c

xmltok.o: expat/xmltok/xmltok.c
	$(CC) $(CFLAGS) -c expat/xmltok/xmltok.c

xmlrole.o: expat/xmltok/xmlrole.c
	$(CC) $(CFLAGS) -c expat/xmltok/xmlrole.c

# end of xml/Makefile.frag

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

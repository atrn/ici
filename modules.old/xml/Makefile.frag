# This comes from xml/Makefile.frag...
CFLAGS	= $(CF_PIC) $(CF_OPTIM) $(CF_CFLAGS) -I. -I$(CF_ICI_H_DIR) -Iexpat/xmlparse -Iexpat/xmltok
OBJS	= xml.$(CF_O) xmlparse.$(CF_O) xmltok.$(CF_O) xmlrole.$(CF_O)

xmlparse.$(CF_O): expat/xmlparse/xmlparse.c
	$(CC) $(CFLAGS) -c expat/xmlparse/xmlparse.c

xmltok.$(CF_O): expat/xmltok/xmltok.c
	$(CC) $(CFLAGS) -c expat/xmltok/xmltok.c

xmlrole.$(CF_O): expat/xmltok/xmlrole.c
	$(CC) $(CFLAGS) -c expat/xmltok/xmlrole.c

# end of xml/Makefile.frag

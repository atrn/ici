#  General prefix for ICI things

PREFIX		= /usr/local

#  Where ICI modules get installed
MODDIR		= $(PREFIX)/lib/ici4

#  Tool used to install a module into $(MODDIR)
INSTALL		= install -c -m 444

#  The name of this module

NAME		= fastcgi

#  The module's loadable file
SO		= ici4$(NAME).so

#  Where FastCGI code lives
FCGI		= /usr/local
FCGILIB		= $(FCGI)/lib/libfcgi.a

#  Compilation options
CFLAGS		= -I$(PERFIX)/include -I$(FCGI)/include -I. -Wall -O2

$(SO)	: fastcgi.c
	$(CC) -shared -o $@ $(CFLAGS) fastcgi.c $(FCGILIB)

#  Test that we can at least load the module into the ICI interpreter
test		: $(SO)
	ICIPATH=. ici test1.ici

install		:
	@$(MAKE) $(MODDIR)/$(SO)

$(MODDIR)/$(SO)	: $(SO)
	$(INSTALL) $(SO) $(MODDIR)

clean		:
	rm -f $(SO)

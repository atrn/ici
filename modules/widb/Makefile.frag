# This comes from widb/Makefile.frag...
ALL_SRC	= $(C_SRC) widb.rc
OBJS	= $(CF_STAR_DOT_O) widb.res
LIBS	= $(CF_LIBS) comctl32.lib 

widb.res: widb.rc
	rc widb.rc

# end of widb/Makefile.frag

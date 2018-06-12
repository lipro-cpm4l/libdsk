## Process this file with automake to produce Makefile.in

SUBDIRS= . include lib tools man
ACLOCAL_AMFLAGS=-I m4

EXTRA_DIST= $(srcdir)/config/* \
	    $(srcdir)/m4/* \
	    $(srcdir)/dskwrite/* \
            $(srcdir)/rpcserv/*.c	     \
            $(srcdir)/rpcserv/*.h	     \
            $(srcdir)/rpcserv/ldserver.rc    \
            $(srcdir)/rpcserv/ldserver.def   \
            $(srcdir)/rpcserv/ldserver.ico   \
            $(srcdir)/rpcserv/shade.bmp	     \
	    $(srcdir)/LDBS/Makefile	     \
	    $(srcdir)/LDBS/ldbs.c	     \
	    $(srcdir)/LDBS/ldbs.h	     \
	    $(srcdir)/LDBS/ldbs2dsk.c	     \
	    $(srcdir)/LDBS/dsk2ldbs.c	     \
	    $(srcdir)/LDBS/ldbsdump.c	     \
	    $(srcdir)/LDBS/ldbscopy.c	     \
	    $(srcdir)/LDBS/ldbstest.c	     \
	    $(srcdir)/LDBS/ldbs.html

install-man:
	cd man && make install-man



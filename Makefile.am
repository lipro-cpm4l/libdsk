## Process this file with automake to produce Makefile.in

SUBDIRS= . include lib tools man doc

EXTRA_DIST= $(srcdir)/config/* \
	    $(srcdir)/dskwrite/* \
	    $(srcdir)/dos16ppd/Makefile      \
	    $(srcdir)/dos16ppd/errno.h       \
	    $(srcdir)/dos16ppd/config.h	     \
	    $(srcdir)/dos32gcc/Makefile	     \
	    $(srcdir)/dos32gcc/config.h      \
	    $(srcdir)/win16vc1/*.MAK         \
	    $(srcdir)/win16vc1/config.h      \
	    $(srcdir)/win16vc1/linux/*.h     \
	    $(srcdir)/win16vc1/sys/*.h       \
	    $(srcdir)/win16vc1/libdsk.def    \
	    $(srcdir)/win16bc5/*.ide         \
	    $(srcdir)/win16bc5/libdsk.def    \
	    $(srcdir)/win16bc5/config.h	     \
	    $(srcdir)/win32bc5/*.ide         \
	    $(srcdir)/win32bc5/libdsk.def    \
	    $(srcdir)/win32bc5/config.h	     \
	    $(srcdir)/win32vc6/config.h      \
	    $(srcdir)/win32vc6/*.dsp         \
	    $(srcdir)/win32vc6/*.dsw         

install-man:
	cd man && make install-man



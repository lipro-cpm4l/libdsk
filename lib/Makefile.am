## Process this file with automake to produce Makefile.in

INCLUDES=-I$(top_srcdir)/include
JAVAC=@JAVAC@
JAVAH=@JAVAH@
JAVA=@JAVA@
JAVADOC=@JAVADOC@
JAR=@JAR@
CLASSCPRE=uk.co.demon.seasip.libdsk
CLASSDPRE=uk/co/demon/seasip/libdsk
HEADERPRE=uk_co_demon_seasip_libdsk

lib_LTLIBRARIES=libdsk.la
noinst_PROGRAMS=@LIBDSKJAR@
EXTRA_PROGRAMS=
EXTRA_DIST=$(CLASSDPRE)/Drive.java \
	   $(CLASSDPRE)/DskException.java \
	   $(CLASSDPRE)/FormatData.java \
	   $(CLASSDPRE)/FormatType.java \
	   $(CLASSDPRE)/Geometry.java \
	   $(CLASSDPRE)/LibDsk.java
#
# Binary compatibility. See libtool.info for full details, but:
# If the source has changed at all, increment the middle number
# If interfaces have been added, removed or changed, increment the
# first number and set the middle one to 0.
# If interfaces have been added increment the right-hand number.
# If interfaces have been removed set the right-hand number to 0.
# 
#
libdsk_la_LDFLAGS = -version-info 1:0:1
libdsk_la_SOURCES = dskread.c  dskwrite.c dskcheck.c dskstat.c \
		   dsklphys.c dskfmt.c   dskopen.c  dskpars.c \
		   dskerror.c dskseek.c  dsksecid.c dskgeom.c \
		   dsktread.c dsksgeom.c dskjni.c \
		   drv.h drvi.h drivers.h drivers.inc \
		   drvlinux.h drvlinux.c \
		   drvwin32.h drvwin32.c w95defs.h \
		   drvcpcem.h drvcpcem.c \
		   drvdqk.h   drvdqk.c \
		   drvposix.h drvposix.c \
		   drvmyz80.h drvmyz80.c \
		   drvcfi.h   drvcfi.c 

JARCLASSES=$(CLASSDPRE)/Drive.class \
	   $(CLASSDPRE)/DskException.class \
	   $(CLASSDPRE)/FormatData.class \
	   $(CLASSDPRE)/FormatType.class \
	   $(CLASSDPRE)/Geometry.class \
	   $(CLASSDPRE)/LibDsk.class
JARHEADERS=$(HEADERPRE)_Drive.h \
	   $(HEADERPRE)_Geometry.h \
	   $(HEADERPRE)_FormatType.h \
	   $(HEADERPRE)_LibDsk.h
CLEANFILES=$(JARCLASSES) $(JARHEADERS)

libdsk.jar:	$(JARCLASSES)
	$(JAR) c0f $@ uk/*
	$(JAVADOC) -d $(top_srcdir)/doc $(CLASSCPRE)

$(CLASSDPRE)/%.class:   $(srcdir)/$(CLASSDPRE)/%.java
	here=`pwd` && cd $(srcdir) && $(JAVAC) -d $$here $<

%.class:        $(srcdir)/%.java
	here=`pwd` && cd $(srcdir) && $(JAVAC) -d $$here $<


$(HEADERPRE)_%.h:         $(CLASSDPRE)/%.class
	$(JAVAH) -jni $(CLASSCPRE).`basename $< .class`
	touch $@

$(srcdir)/dskjni.c:	$(JARHEADERS)

install-exec-local:
	$(mkinstalldirs) $(libdir)
	if test x@LIBDSKJAR@ = x; then true; else $(INSTALL_DATA) @LIBDSKJAR@ $(libdir); fi


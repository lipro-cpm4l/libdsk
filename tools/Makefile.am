## Process this file with automake to produce Makefile.in

INCLUDES=-I$(top_srcdir)/include
LDADD=../lib/libdsk.la

CLASSPATH=@CLASSPATH@
JAVAC=@JAVAC@ 

bin_PROGRAMS=dsktrans dskform dskid md3serial
dsktrans_SOURCES=dsktrans.c utilopts.c utilopts.h formname.c formname.h
dskform_SOURCES=dskform.c utilopts.c utilopts.h formname.c formname.h
dskid_SOURCES=dskid.c utilopts.c utilopts.h
md3serial_SOURCES=md3serial.c utilopts.c utilopts.h

noinst_PROGRAMS=@TOOLCLASSES@
EXTRA_PROGRAMS=
EXTRA_DIST=DskTrans.java DskFormat.java DskID.java FormatNames.java UtilOpts.java

check_PROGRAMS = check1 check2 check3
check1_SOURCES = check1.c
check2_SOURCES = check2.c
check3_SOURCES = check3.c
CLEANFILES=*.class

%.class:        $(srcdir)/%.java
	here=`pwd` && cd $(srcdir) && $(JAVAC) -classpath $(CLASSPATH):$$here/../lib/libdsk.jar -d $$here $<


CC=gcc
CFLAGS=-g -Wall -I. -I../include -I../lib -DDOS32FLOPPY

OBS =   compbz2.o     dskerror.o    dskread.o    drvdos32.o \
	compgz.o      drvlinux.o    dskfmt.o     dsksecid.o  \
	compress.o    drvmyz80.o    dskgeom.o    dskseek.o  \
	compsq.o      drvposix.o    dskjni.o     dsksgeom.o  \
	drvdos16.o    drvwin16.o    dsklphys.o   dskstat.o  \
	drvcfi.o      drvwin32.o    dskopen.o    dsktread.o  \
	drvcpcem.o    dskcheck.o    dskpars.o    dskwrite.o 

OBS1 = dskid.o       utilopts.o    libdsk.a
OBS2 = dskform.o     utilopts.o    formname.o   libdsk.a
OBS3 = dsktrans.o   utilopts.o    formname.o   libdsk.a

all:	libdsk.a dskid.exe dsktrans.exe dskform.exe

dskid.exe:	$(OBS1)
	$(CC) -o $@ $(CFLAGS) $(OBS1) 

dskform.exe:	$(OBS2)
	$(CC) -o $@ $(CFLAGS) $(OBS2)

dsktrans.exe:	$(OBS3)
	$(CC) -o $@ $(CFLAGS) $(OBS3) 

libdsk.a:	$(OBS) 
	ar rc $@ $(OBS)
	ranlib $@

%.o:	../lib/%.c
	$(CC) -c $(CFLAGS) $<

%.o:	../tools/%.c
	$(CC) -c $(CFLAGS) $<

clean:
	del dskid.exe
	del dskform.exe
	del dsktrans.exe
	del libdsk.a
	del *.o

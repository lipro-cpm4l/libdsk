CC=pacc
CFLAGS=-Bl -I. -I../include -I../lib -DDOS16FLOPPY
OBS1=compbz2.obj     dskerror.obj    dskread.obj 
OBS2=compgz.obj      drvlinux.obj    dskfmt.obj     dsksecid.obj 
OBS3=compress.obj    drvmyz80.obj    dskgeom.obj    dskseek.obj 
OBS4=compsq.obj      drvposix.obj    dskjni.obj     dsksgeom.obj 
OBS5=drvdos16.obj    drvwin16.obj    dsklphys.obj   dskstat.obj 
OBS6=drvcfi.obj      drvwin32.obj    dskopen.obj    dsktread.obj 
OBS7=drvcpcem.obj    dskcheck.obj    dskpars.obj    dskwrite.obj 
OBS8=dskid.obj       utilopts.obj    libdsk.lib
OBS9=dskform.obj     utilopts.obj    formname.obj   libdsk.lib
OBS10=dsktrans.obj   utilopts.obj    formname.obj   libdsk.lib

all:	libdsk.lib dskid.exe dsktrans.exe dskform.exe

dskid.exe:	$(OBS8)
	$(CC) $(CFLAGS) $(OBS8)

dsktrans.exe:	$(OBS10)
	$(CC) $(CFLAGS) $(OBS10)

dskform.exe:	$(OBS9)
	$(CC) $(CFLAGS) $(OBS9)

libdsk.lib:	$(OBS1) $(OBS2) $(OBS3) $(OBS4) $(OBS5) $(OBS6) $(OBS7) $(OBS8)
	libr r $@ $(OBS1)
	libr r $@ $(OBS2)
	libr r $@ $(OBS3)
	libr r $@ $(OBS4)
	libr r $@ $(OBS5)
	libr r $@ $(OBS6)
	libr r $@ $(OBS7)

%.obj:	../lib/%.c
	$(CC) -c $(CFLAGS) $<

%.obj:	../tools/%.c
	$(CC) -c $(CFLAGS) $<

clean:
	del dskid.exe
	del dskform.exe
	del dsktrans.exe
	del libdsk.lib
	del *.obj

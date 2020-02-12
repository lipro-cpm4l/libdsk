[![Build status](https://ci.appveyor.com/api/projects/status/3t956eiqehhbakkd/branch/cpm4l/libdsk-1.5.12?svg=true)](https://ci.appveyor.com/project/rexut/libdsk/branch/cpm4l/libdsk-1.5.12)
[![Build Status](https://travis-ci.org/lipro-cpm4l/libdsk.svg?branch=cpm4l%2Flibdsk-1.5.12)](https://travis-ci.org/lipro-cpm4l/libdsk)

LibDsk - Library for accessing floppy drives and disc images
============================================================

LibDsk is a library intended to give transparent access to floppy drives
and to the "disc image files" used by emulators to represent floppy drives.

It is intended for use in:

- Emulator tools: converting between real floppy discs and disc images,
  as `CPCTRANS` / `PCWTRANS` do under *DOS*.
- Filesystem utilities: [CPMTOOLS](http://www.moria.de/~michael/cpmtools/)
  is configurable to use LibDsk , thus allowing the use of *CPMTOOLS* on
  emulator `.DSK` images. To do this, install LibDsk and then build
  *CPMTOOLS* , using `./configure --with-libdsk`. For *CPMTOOLS 1.9 or 2.0*,
  you will also need to apply
  [this patch](http://www.seasip.info/Unix/LibDsk/cpmtools.patch).
- Emulators: it is possible to use LibDsk as part of an emulator's
  floppy controller emulation, thus giving the emulator transparent
  access to `.DSK` files or real discs.

## Compilation

### Requirements

You will need an ANSI standard C compiler, prefered is the GNU CC.  Optional
you can build against the compression libraries `zlib` and `libbz2` and also
you can build an Java Native Interface binding to LibDsk.  You can use LibDsk
under DOS, Windows, Linux and MacOS.

```bash
sudo apt-get install zlib1g-dev
sudo apt-get install libbz2-dev
```

### Get the Code

```bash
git clone https://github.com/lipro-cpm4l/libdsk.git
cd libdsk
```

### Build and install the binary

```bash
./configure --with-zlib --with-bzlib
make all
sudo make install
```

## Documentation

- [LibDsk](https://www.seasip.info/Unix/LibDsk/)
- [LibDsk LDBS](https://www.seasip.info/Unix/LibDsk/ldbs.html)
- [LibDsk PDF](https://raw.githubusercontent.com/lipro-cpm4l/libdsk/upstream/doc/libdsk.pdf)

---

This is an unofficial fork!
===========================

Original written by John Elliott <seasip.webmaster@gmail.com> and distributed
under the GNU Library General Public License version 2.

*Primary-site*: https://www.seasip.info/Unix/LibDsk/

## License terms and liability

The author provides the software in accordance with the terms of
the GNU-GPL. The use of the software is free of charge and is
therefore only at your own risk! **No warranty or liability!**

**Any guarantee and liability is excluded!**

## Authorship

*Primary-site*: https://www.seasip.info/Unix/LibDsk/

### Source code

**John Elliott is the originator of the C source code and**
as well as the associated scripts, descriptions and help files.
This part is released and distributed under the GNU General
Public License (GNU-GPL) Version 2.

**A few parts of the source code are based on the work of other
authors:**

> - **Mark Ogden**:
>   Fixed some memory corruption issues detected by VS2017 analysis.
>   Widened `total_sectors32` to 32 bits in `dskform`.  Swapped
>   "sectors / track" and "sectors / cylinder" captions in `dskutil`.
> - **Steven Fosdick**:
>   Fixed compilation issue on recent glibc.
> - **Emulix75**:
>   New disk image format implemented, SAP.
> - **Will Kranz**:
>   Teledisk read-only support for 'advanced' compression.  Based on
>   [wteledsk](https://web.archive.org/web/20130116072335/http://www.fpns.net/willy/wteledsk.htm)
>   decompression code by Will Kranz (relicensed under LGPLv2, with
>   permission).
> - **Ralf-Peter Nerlich**:
>   Correct the description of the 'W' command in `dskutil`.
>   All utilities now use `basename()` (if available) to trim paths
>   from `argv[0]`.  Replacement CopyQM driver.
> - **Jurgen Sievers**:
>   Bug fixes an buffer overflow in `rcpmfs` driver when renaming
>   a file with a full 8.3 name and a nonzero user number.
> - **Alistair John Bush**:
>   Patched Makefile.am to pass JAVACFLAGS to Java compiler.
> - **Sven Klose**:
>   Corrected a compilation problem on FreeBSD.
> - **Stuart Brady**:
>   Added a new geometry (`FMT_TRDOS640`).
> - **Ramlaid <www.ramlaid.com>**:
>   Modified `cpcemu` driver so that the `dsk_trkids()` function more
>   accurately reflects the result from a real disk.  Doesn't leak file
>   handles if file not SQ compressed.  Uses the passed sector size in
>   `dsk_xread()` / `dsk_xwrite()` of the `ntwdm` driver rather than the
>   sector size in the geometry structure.
> - **Daniel Black**:
>   Corrected an install bug in `Makefile.am`.
> - **Simon Owen**:
>   Added `ntwdm` driver.
> - **Philip Kendall**:
>   Added `dsk_dirty()` function.
> - **Per Ola Ingvarsson**:
>   A read-only CopyQM driver and CopyQM format documentation.
> - **Thierry Jouin**:
>   Bug fixes in the extended `.DSK` format handler.
> - **Darren Salt**:
>   Bug fixes in the example utilities.
>   Manual pages added to the distribution.
> - **Cliff Lawson**:
>   Support added for `.CFI` format (a strange format used by Cliff
>   Lawson to distribute Amstrad PC boot floppies).
> - **Kevin Thacker**:
>   Make libdsk compile in Microsoft Visual C++.

*see*:
[doc/COPYING](https://raw.githubusercontent.com/lipro-cpm4l/libdsk/upstream/doc/COPYING),
[ChangeLog](https://raw.githubusercontent.com/lipro-cpm4l/libdsk/upstream/ChangeLog)

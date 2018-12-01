Debian 10 (Buster) - `libdsk-1.5.9`
===================================

This branch contains sorce code changes as made by Debian 10 (Buster) w/o `debian` folder,
[dists/buster/main/source/Sources.xz](http://debian.inet.de/debian/dists/buster/main/source/Sources.xz):

```
Package: libdsk
Binary: libdsk4, libdsk4-dev, libdsk-utils
Version: 1.5.9+dfsg-1
Maintainer: Dominik George <natureshadow@debian.org>
Build-Depends: debhelper (>= 11), dh-exec, exiftool, ghostscript, lyx, texlive-extra-utils, texlive-fonts-recommended, texlive-latex-recommended
Architecture: any
Standards-Version: 4.2.1
Format: 3.0 (quilt)
Files:
 441864045fa609a955a5ae4bfdb11b9e 2132 libdsk_1.5.9+dfsg-1.dsc
 3467b23fcdcf66fa95ac408d4199a507 660928 libdsk_1.5.9+dfsg.orig.tar.xz
 019e311d2c3e9e778db219dc8da55e0c 6480 libdsk_1.5.9+dfsg-1.debian.tar.xz
Vcs-Browser: https://salsa.debian.org/debian/libdsk
Vcs-Git: https://salsa.debian.org/debian/libdsk.git
Checksums-Sha256:
 2d2f3e80d165f36109d4c01f1dee219eb0ad16ce5d5d64ec99b8bb0c68609071 2132 libdsk_1.5.9+dfsg-1.dsc
 a39777775532872e774523b62814ad3dbf693b24181177362a5f6a44e0aacc46 660928 libdsk_1.5.9+dfsg.orig.tar.xz
 5664088ef95d67e929405194a4749d63f19df9291f4072f22a43b56620eaa51c 6480 libdsk_1.5.9+dfsg-1.debian.tar.xz
Homepage: http://www.seasip.info/Unix/LibDsk/
Package-List: 
 libdsk-utils deb utils optional arch=any
 libdsk4 deb libs optional arch=any
 libdsk4-dev deb libdevel optional arch=any
Directory: pool/main/libd/libdsk
Priority: extra
Section: misc
```

* snapshot.debian.org: https://snapshot.debian.org/package/libdsk/1.5.9%2Bdfsg-1/

  * [libdsk_1.5.9+dfsg-1.debian.tar.xz](https://snapshot.debian.org/archive/debian/20181201T152528Z/pool/main/libd/libdsk/libdsk_1.5.9%2Bdfsg-1.debian.tar.xz)
    SHA1:d11158bc3c10ed72de21eedb9120f59bb393a0d7
  * [libdsk_1.5.9+dfsg-1.dsc](https://snapshot.debian.org/archive/debian/20181201T152528Z/pool/main/libd/libdsk/libdsk_1.5.9%2Bdfsg-1.dsc)
    SHA1:141ea101adfe5f05f80afaad90a8595db4f88639
  * [libdsk_1.5.9+dfsg.orig.tar.xz](https://snapshot.debian.org/archive/debian/20181201T152528Z/pool/main/libd/libdsk/libdsk_1.5.9%2Bdfsg.orig.tar.xz)
    SHA1:3d33e2bac042f9d1a0e8a1e79e4c7126550c88da

* sources.debian.org/src: https://sources.debian.org/src/libdsk/1.5.9+dfsg-1
* sources.debian.org/patches: https://sources.debian.org/patches/libdsk/1.5.9+dfsg-1/

  * [series](https://sources.debian.org/src/libdsk/1.5.9+dfsg-1/debian/patches/series/)
    [(download)](https://sources.debian.org/data/main/libd/libdsk/1.5.9+dfsg-1/debian/patches/series)

    1. [fix_configure.patch](https://sources.debian.org/patches/libdsk/1.5.9+dfsg-1/fix_configure.patch/)
       [(download)](https://sources.debian.org/data/main/libd/libdsk/1.5.9+dfsg-1/debian/patches/fix_configure.patch)
       : Declare package non-GNU.
    1. [remove_dos_windows.patch](https://sources.debian.org/patches/libdsk/1.5.9+dfsg-1/remove_dos_windows.patch/)
       [(download)](https://sources.debian.org/data/main/libd/libdsk/1.5.9+dfsg-1/debian/patches/remove_dos_windows.patch)
       : Remove dependency on DOS and Windows stuff.
    1. [reproducible.patch](https://sources.debian.org/patches/libdsk/1.5.9+dfsg-1/reproducible.patch/)
       [(download)](https://sources.debian.org/data/main/libd/libdsk/1.5.9+dfsg-1/debian/patches/reproducible.patch)
       : Suppress date in LyX PDF output.

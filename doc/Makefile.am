
noinst_DATA = libdsk.lyx libdsk.pdf libdsk.txt \
	      COPYING  TODO INSTALL README cfi.html

EXTRA_DIST = $(noinst_DATA)

%.txt:	%.lyx
	lyx -e text $<

%.tex:	%.lyx
	lyx -e latex $<

%.pdf:	%.lyx
	lyx -e pdf $<

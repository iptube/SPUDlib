SUBDIRS = src samplecode . test
ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = README.md LICENSE spudlib.pc.in

clean-local:
	rm -rf *.info *lcov

## install check.pc
pcdatadir = $(libdir)/pkgconfig
pcdata_DATA = spudlib.pc

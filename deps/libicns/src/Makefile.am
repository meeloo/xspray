
lib_LTLIBRARIES = libicns.la

libicns_la_LDFLAGS = -version-info 3:0:2

libicns_la_LIBADD = @PNG_LIBS@ @JP2000_LIBS@

libicns_la_SOURCES = \
  icns_debug.c \
  icns_element.c \
  icns_family.c \
  icns_image.c \
  icns_io.c \
  icns_png.c \
  icns_jp2.c \
  icns_rle24.c \
  icns_utils.c \
  icns_colormaps.h \
  icns_internals.h \
  icns.h 

AM_CFLAGS = -Wall

libicns_includedir=$(includedir)
libicns_include_HEADERS = icns.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libicns.pc

EXTRA_DIST = \
  apidocs.html \
  apidocs.txt

MAINTAINERCLEANFILES = \
  Makefile.in

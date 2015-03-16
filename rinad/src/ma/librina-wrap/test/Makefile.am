#
# Makefile.am
#
# Written by: Bernat Gastón <bernat DOT gaston AT i2cat DOT net>
#

SUBDIRS      =

EXTRA_DIST   =
CLEANFILES   =

noinst_LTLIBRARIES = librinad_ma_ribs.la

librinad_ma_ribs_la_CFLAGS   =
librinad_ma_ribs_la_CPPFLAGS =			\
	$(CPPFLAGS_EXTRA)			\
	$(LIBRINA_CFLAGS)			\
	$(LIBPROTOBUF_CFLAGS)			

librinad_ma_ribs_la_LIBADD   =			\
	$(LIBRINA_LIBS)				\
	$(LIBPROTOBUF_LIBS)

librinad_ma_ribs_la_SOURCES  =			\
	test-rib.cc						

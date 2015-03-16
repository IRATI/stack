#
# Makefile.am
#
# Written by: Bernat Gastón <marc DOT sune AT bisdn DOT de>
#

SUBDIRS      = test

EXTRA_DIST   =
CLEANFILES   =

noinst_LTLIBRARIES = librina_cdap_rib.la

librina_cdap_rib_la_CFLAGS   =
librina_cdap_rib_la_CPPFLAGS =			\
	$(CPPFLAGS_EXTRA)			\
	$(LIBRINA_CFLAGS)			\
	$(LIBPROTOBUF_CFLAGS)			\
	-I$(srcdir)/../../			\
	-I$(srcdir)/../../common

librina_cdap_rib_la_LIBADD   =			\
	$(LIBRINA_LIBS)				\
	$(LIBPROTOBUF_LIBS)

librina_cdap_rib_la_SOURCES  =			\
	rib_provider.cc						\
	cdap_provider.cc

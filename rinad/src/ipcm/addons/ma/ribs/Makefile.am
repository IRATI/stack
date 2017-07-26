#
# Makefile.am
#
# Written by: Marc Sune <marc DOT sune AT bisdn DOT de>
#

SUBDIRS      =

EXTRA_DIST   =
CLEANFILES   =

noinst_LTLIBRARIES = librinad_ma_ribs.la

librinad_ma_ribs_la_CFLAGS   =
librinad_ma_ribs_la_CPPFLAGS =				\
	$(CPPFLAGS_EXTRA)				\
	$(LIBRINA_CFLAGS)				\
	$(LIBPROTOBUF_CFLAGS)				\
	-I$(srcdir)/../../				\
	-I$(srcdir)/../../../../common

librinad_ma_ribs_la_LIBADD   =				\
	$(LIBRINA_LIBS)					\
	$(LIBPROTOBUF_LIBS)

librinad_ma_ribs_la_SOURCES  =				\
	ipcp_obj.cc   					\
	ribd_v1.cc

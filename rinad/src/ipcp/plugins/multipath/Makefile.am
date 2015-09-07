#
# Makefile.am
#
# Written by: Vincenzo Maffione <v DOT maffione AT nextworks DOT it>
#

EXTRA_DIST   =
CLEANFILES   =

COMMONCPPFLAGS =				\
	$(CPPFLAGS_EXTRA)			\
	$(LIBRINA_CFLAGS)			\
	-I$(top_srcdir)/src			\
	-I$(top_srcdir)/src/common

pluginsdir          = $(pkglibdir)/ipcp
plugins_LTLIBRARIES =

multipath_la_CFLAGS   =
multipath_la_CPPFLAGS =				\
	$(COMMONCPPFLAGS)
multipath_la_LIBADD   =
multipath_la_LDFLAGS  = -module
multipath_la_SOURCES  =				\
	mp-routing-ps.cc		\
	../../components.h				\
	../../components.cc

plugins_LTLIBRARIES += multipath.la

dist_plugins_DATA = multipath.manifest

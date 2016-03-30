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

sm_cbac_la_CFLAGS   =
sm_cbac_la_CPPFLAGS =			\
	$(COMMONCPPFLAGS)
sm_cbac_la_LIBADD   =
sm_cbac_la_LDFLAGS  = -module
sm_cbac_la_SOURCES  =			\
	sm-cbac-plugin.cc		\
	security-manager-cbac.cc	\
	security-manager-cbac.h		\
	enrollment-task-ps.cc		\
	../../components.h		\
	../../components.cc

plugins_LTLIBRARIES += sm-cbac.la

dist_plugins_DATA = sm-cbac.manifest
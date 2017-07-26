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

sm_auth_psoc_la_CFLAGS   =
sm_auth_psoc_la_CPPFLAGS =			\
	$(COMMONCPPFLAGS)
sm_auth_psoc_la_LIBADD   =
sm_auth_psoc_la_LDFLAGS  = -module
sm_auth_psoc_la_SOURCES  =			\
	sm-auth-psoc-plugin.cc			\
	../../components.h			\
	../../components.cc

plugins_LTLIBRARIES += sm-auth-psoc.la

dist_plugins_DATA = sm-auth-psoc.manifest

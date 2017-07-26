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

sm_passwd_la_CFLAGS   =
sm_passwd_la_CPPFLAGS =				\
	$(COMMONCPPFLAGS)
sm_passwd_la_LIBADD   =
sm_passwd_la_LDFLAGS  = -module
sm_passwd_la_SOURCES  =				\
	security-manager-passwd.cc		\
	../../components.h				\
	../../components.cc

plugins_LTLIBRARIES += sm-passwd.la

dist_plugins_DATA = sm-passwd.manifest

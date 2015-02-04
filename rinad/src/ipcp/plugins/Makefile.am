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

pluginsdir = $(pkglibdir)/ipcp

plugins_LTLIBRARIES = ps_sm_passwd.la

ps_sm_passwd_la_CFLAGS   =
ps_sm_passwd_la_CPPFLAGS =			\
	$(COMMONCPPFLAGS)
ps_sm_passwd_la_LIBADD   =
ps_sm_passwd_la_LDFLAGS  = -module
ps_sm_passwd_la_SOURCES  =			\
	security-manager-passwd.cc		\
	component.h

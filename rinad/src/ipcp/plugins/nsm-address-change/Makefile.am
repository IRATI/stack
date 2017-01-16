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

nsm_address_change_la_CFLAGS   =
nsm_address_change_la_CPPFLAGS =			\
	$(COMMONCPPFLAGS)
nsm_address_change_la_LIBADD   =
nsm_address_change_la_LDFLAGS  = -module
nsm_address_change_la_SOURCES  =			\
	nsm-address-change-plugin.cc			\
	../../components.h			\
	../../components.cc

plugins_LTLIBRARIES += nsm-address-change.la

dist_plugins_DATA = nsm-address-change.manifest
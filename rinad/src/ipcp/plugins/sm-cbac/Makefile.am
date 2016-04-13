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
	
COMMONLIBS =								\
	$(builddir)/../../../common/librinad.la	\
	$(LIBRINA_LIBS)

pluginsdir          = $(pkglibdir)/ipcp
plugins_LTLIBRARIES =

sm_cbac_la_CFLAGS   =
sm_cbac_la_CPPFLAGS =			\
	$(COMMONCPPFLAGS)
sm_cbac_la_LIBADD   = $(COMMONLIBS) -ldl
sm_cbac_la_LDFLAGS  = -module
sm_cbac_la_SOURCES  =			\
	sm-cbac-plugin.cc		\
	../../components.h		\
	../../components.cc

plugins_LTLIBRARIES += sm-cbac.la

dist_plugins_DATA = sm-cbac.manifest
#
# Makefile.am
#
# Written by: Vincenzo Maffione <v DOT maffione AT nextworks DOT it>
#

include Makefile.inc

# EXTRA_DIST   =
# CLEANFILES   =
# MOSTLYCLEANFILES = 
# DISTCLEANFILES = 
# protoSOURCES = 
# 
# EXTRA_DIST += sm-cbac.proto
# protoSOURCES += sm-cbac.pb.cc sm-cbac.pb.h
# #DISTCLEANFILES += sm-cbac.pb.h
# BUILT_SOURCES = 
#EXTRA_DIST = sm-cbac.proto

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
	$(COMMONCPPFLAGS)		\
	$(LIBPROTOBUF_CFLAGS)
sm_cbac_la_LIBADD   =  $(LIBPROTOBUF_LIBS)	\
	$(COMMONLIBS) -ldl
	
	
sm_cbac_la_LDFLAGS  = -module
sm_cbac_la_SOURCES  =			\
	sm-cbac-plugin.cc		\
	security-manager-cbac.cc	\
	security-manager-cbac.h		\
	../../components.h		\
	../../components.cc	\
	$(protoSOURCES)
	
plugins_LTLIBRARIES += sm-cbac.la
BUILT_SOURCES = $(protoSOURCES)
dist_plugins_DATA = sm-cbac.manifest




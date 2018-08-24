#
# Makefile.am
#

bin_PROGRAMS                       =
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT =

rinaperf_SOURCES = rinaperf.c
rinaperf_LDADD = $(LIBRINA_API_LIBS) -lm
rinaperf_CPPFLAGS = $(LIBRINA_API_CFLAGS)

rina_echo_async_SOURCES = rina-echo-async.c
rina_echo_async_LDADD = $(LIBRINA_API_LIBS)
rina_echo_async_CPPFLAGS = $(LIBRINA_API_CFLAGS)

rina_gw_SOURCES = rina-gw.cpp fdfwd.cpp
rina_gw_LDADD = $(LIBRINA_API_LIBS)
rina_gw_CPPFLAGS = $(LIBRINA_API_CFLAGS) -std=c++11

MOSTLYCLEANFILES =
EXTRA_DIST =
include $(srcdir)/Makefile.inc

noinst_LTLIBRARIES =				\
	libiporina.la

BUILT_SOURCES = $(protoSOURCES)

libiporina_la_SOURCES =			\
	$(protoSOURCES)
libiporina_la_CPPFLAGS =			\
	-I$(builddir)/..			\
	-I$(srcdir)/..				\
	$(CPPFLAGS_EXTRA)

iporinad_SOURCES = iporinad.cpp fdfwd.cpp cdap.cpp
iporinad_LDADD = $(LIBRINA_API_LIBS) -liporina
iporinad_CPPFLAGS = $(LIBRINA_API_CFLAGS) -std=c++11

bin_PROGRAMS += rinaperf rina-echo-async rina-gw iporinad
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT += rinaperf rina-echo-async rina-gw iporinad

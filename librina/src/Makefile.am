#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

lib_LTLIBRARIES =

librina_la_SOURCES  =				\
	logs.cc					\
	concurrency.cc				\
	librina.cc				\
	librina-common.cc			\
	librina-application.cc			\
	librina-ipc-manager.cc			\
	librina-ipc-process.cc			\
	librina-faux-sockets.cc			\
	librina-cdap.cc				\
	librina-sdu-protection.cc		\
	netlink-manager.cc			\
	netlink-parsers.cc

EXTRA_DIST =					\
	logs.h

librina_la_LDFLAGS =

librina_la_CPPFLAGS =				\
	-I$(top_srcdir)/include			\
	$(LIBNL3_CFLAGS)			\
	$(LIBNLGENL3_CFLAGS)			\
	$(CPPFLAGS_EXTRA)

librina_la_LIBADD =				\
	$(LIBNL3_LIBS)				\
	$(LIBNLGENL3_LIBS)

lib_LTLIBRARIES += librina.la

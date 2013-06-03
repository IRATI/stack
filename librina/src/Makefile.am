#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

lib_LTLIBRARIES =

librinapp_la_SOURCES  =				\
	librina.cc				\
	librina-common.cc			\
	librina-application.cc			\
	librina-ipc-manager.cc			\
	librina-ipc-process.cc			\
	librina-faux-sockets.cc			\
	librina-cdap.cc				\
	librina-sdu-protection.cc

EXTRA_DIST =					\
	logs.h

librinapp_la_LDFLAGS  =
librinapp_la_CPPFLAGS = -I$(top_srcdir)/include

lib_LTLIBRARIES += librinapp.la

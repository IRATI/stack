#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

lib_LTLIBRARIES =

librinapp_la_SOURCES  =				\
	librina++.c++				\
	librina.c				\
	librina-common.c			\
	librina-application.c			\
	librina-ipc-manager.c			\
	librina-ipc-process.c			\
	librina-faux-sockets.c			\
	librina-cdap.c				\
	librina-sdu-protection.c

EXTRA_DIST =					\
	logs.h

librinapp_la_LDFLAGS  =
librinapp_la_CPPFLAGS = -I$(top_srcdir)/include

lib_LTLIBRARIES += librinapp.la

#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

librina_la_SOURCES  =				\
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

librina_la_LDFLAGS  =
librina_la_CPPFLAGS = -I$(top_srcdir)/include

lib_LTLIBRARIES = librina.la

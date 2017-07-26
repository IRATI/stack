#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

SUBDIRS = tclap

noinst_LTLIBRARIES = librinaapp.la

librinaapp_la_CFLAGS   =
librinaapp_la_CPPFLAGS =				\
	$(LIBRINA_CFLAGS)			\
	$(CPPFLAGS_EXTRA)			
librinaapp_la_LIBADD   =				\
	$(LIBRINA_LIBS)

librinaapp_la_SOURCES  =					\
	application.cc         application.h			\
	server.cc	       server.h				\
	utils.cc	       utils.h				
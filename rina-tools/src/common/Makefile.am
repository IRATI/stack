#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

SUBDIRS = tclap

noinst_LTLIBRARIES = rinaapp.la

rinaapp_la_CFLAGS   =
rinaapp_la_CPPFLAGS =				\
	$(LIBRINA_CFLAGS)			\
	$(CPPFLAGS_EXTRA)			\
	-std=c++11
rinaapp_la_LIBADD   =				\
	$(LIBRINA_LIBS)

rinaapp_la_SOURCES  =					\
	cdap-echo-application.cc         cdap-echo-application.h
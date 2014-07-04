#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

noinst_LTLIBRARIES = librinad.la

librinad_la_CFLAGS   =
librinad_la_CPPFLAGS = $(CPPFLAGS_EXTRA) $(LIBRINA_CFLAGS) -I$(srcdir)/tclap
librinad_la_LIBADD   = $(LIBRINA_LIBS)

librinad_la_SOURCES  =					\
	rina-configuration.cc rina-configuration.h	\
	event-loop.cc	      event-loop.h		\
                              concurrency.h		\
	$(tclapSOURCES)

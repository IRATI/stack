#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

SUBDIRS = tclap encoders

noinst_LTLIBRARIES = librinad.la

librinad_la_CFLAGS   =
librinad_la_CPPFLAGS =				\
	$(CPPFLAGS_EXTRA)			\
	$(LIBRINA_CFLAGS)			\
	$(LIBPROTOBUF_CFLAGS)			\
	-I$(srcdir)/tclap
librinad_la_LIBADD   =				\
	$(LIBRINA_LIBS)				\
	$(LIBPROTOBUF_LIBS)			\
	-Lencoders -lencoders

librinad_la_SOURCES  =					\
	rina-configuration.cc rina-configuration.h	\
	event-loop.cc	      event-loop.h		\
	helpers.cc	      helpers.h			\
	encoder.cc	      encoder.h			\
                      concurrency.h


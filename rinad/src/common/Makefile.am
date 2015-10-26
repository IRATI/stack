#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

SUBDIRS = tclap encoders

#
#Libs
#
lib_LTLIBRARIES = librinad.la

librinad_ladir = $(pkgincludedir)/ipcm

#Distributable headers
librinad_la_HEADERS = encoder.h

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
	debug.cc              debug.h                   \
	rina-configuration.cc rina-configuration.h	\
	configuration.h    configuration.cc   \
	encoder.cc	      encoder.h			\
			      concurrency.h

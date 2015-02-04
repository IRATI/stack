#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

EXTRA_DIST   =
CLEANFILES   =

SUBDIRS=

#
#Addons
#
noinst_LTLIBRARIES = libaddons.la
libaddons_la_SOURCES  =				\
	console.h				\
	console.cc

libaddons_la_CPPFLAGS =				\
	$(CPPFLAGS_EXTRA)			\
	$(LIBRINA_CFLAGS)			\
	-I$(srcdir)/../../			\
	-I$(srcdir)/../../common

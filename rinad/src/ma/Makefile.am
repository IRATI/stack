#
# Makefile.am
#
# Written by: Marc Sune <marc DOT sune AT bisdn DOT de>
#

SUBDIRS      =

EXTRA_DIST   =
CLEANFILES   =

sbin_PROGRAMS = mad 

mad_SOURCES  = 					\
	agent.cc 				\
	configuration.cc 			\
	main.cc

mad_CPPFLAGS =					\
	$(CPPFLAGS_EXTRA)			\
	$(LIBRINA_CFLAGS)			\
	-I$(srcdir)/../			\
	-I$(srcdir)/../common

mad_LDADD = 					\
	$(builddir)/../common/librinad.la	\
	$(LIBRINA_LIBS)


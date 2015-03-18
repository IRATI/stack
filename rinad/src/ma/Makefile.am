#
# Makefile.am
#
# Written by: Marc Sune <marc DOT sune AT bisdn DOT de>
#

SUBDIRS      = ribs librina-wrap

EXTRA_DIST   =
CLEANFILES   =

sbin_PROGRAMS = mad

mad_SOURCES  = 					\
	agent.cc				\
	bgtm.cc					\
	confm.cc				\
	flowm.cc				\
	ribf.cc					\
	commm.cc				\
	main.cc

mad_CPPFLAGS =					\
	$(CPPFLAGS_EXTRA)			\
	$(LIBRINA_CFLAGS)			\
	-I$(srcdir)/../			\
	-I$(srcdir)/../common   

mad_LDADD = 					\
	ribs/librinad_ma_ribs.la		\
	$(builddir)/../common/librinad.la	\
	$(LIBRINA_LIBS)


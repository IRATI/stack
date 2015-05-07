#
# Makefile.am
#
# Written by: Marc Sune <marc DOT sune AT bisdn DOT de>
#

SUBDIRS      = ribs

noinst_LTLIBRARIES = libaddons_ma.la

libaddons_ma_la_SOURCES  = 			\
	agent.cc				\
	bgtm.cc					\
	confm.cc				\
	flowm.cc				\
	ribf.cc					

libaddons_ma_la_CPPFLAGS =			\
	$(CPPFLAGS_EXTRA)			\
	$(LIBRINA_CFLAGS)			\
	-I$(srcdir)/../../../			\
	-I$(srcdir)/../../../common		\
	-I${srcdir}/../../jsoncpp

libaddons_ma_la_LIBADD = 			\
	$(builddir)/../../../common/librinad.la	\
	ribs/librinad_ma_ribs.la		\
	$(LIBRINA_LIBS)


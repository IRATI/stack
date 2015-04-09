#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

EXTRA_DIST   =
CLEANFILES   =

bin_PROGRAMS = manager

COMMONCPPFLAGS =				\
	$(CPPFLAGS_EXTRA)			\
	$(LIBRINA_CFLAGS)			\
	-I$(srcdir)/..				\
	-I$(srcdir)/../common

COMMONLIBS =					\
	$(builddir)/../common/librinad.la	\
	$(LIBRINA_LIBS)

manager_CFLAGS   =
manager_CPPFLAGS =					\
	$(COMMONCPPFLAGS)			
manager_LDADD    = $(COMMONLIBS) -ldl
manager_SOURCES  =						\
	main.cc						\
	manager.cc		manager.h		

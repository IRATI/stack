#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

EXTRA_DIST   =
CLEANFILES   =

bin_PROGRAMS = manager	

manager_CPPFLAGS =					\
	$(CPPFLAGS_EXTRA)			\
	$(LIBRINA_CFLAGS)		 \
	-I$(srcdir)/../common	
	
manager_LDADD    = 	$(LIBRINA_LIBS) -ldl -lrinad \
		../common/librinaapp.la	
	
manager_SOURCES  =						\
	main.cc						\
	manager.cc		manager.h		

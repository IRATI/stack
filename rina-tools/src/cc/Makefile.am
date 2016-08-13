#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

EXTRA_DIST   =
CLEANFILES   =

bin_PROGRAMS = cc	

cc_CPPFLAGS =					\
	-std=c++11          \
	$(CPPFLAGS_EXTRA)			\
	$(LIBRINA_CFLAGS)		 \
	-I$(srcdir)/../common	
	
cc_LDADD    = 	$(LIBRINA_LIBS) -ldl -lrinad \
		../common/librinaapp.la	
	
cc_SOURCES  =						\
	main.cc						\
	connector.cc			connector.h	\
	easywsclient.cc	  easywsclient.h \
	dmsworker.cc 			maworker.cc \
	eventfilters.cc 		eventfilters.h \
	eventsystem.h	

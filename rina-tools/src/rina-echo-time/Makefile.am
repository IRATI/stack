#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

bin_PROGRAMS                       =
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT =

rina_echo_time_SOURCES  =				\
	et-server.cc              et-server.h			\
	et-client.cc              et-client.h			\
	main.cc
rina_echo_time_LDADD    = $(LIBRINA_LIBS) -lrt \
	../common/librinaapp.la	
rina_echo_time_CPPFLAGS =			\
	$(LIBRINA_CFLAGS)			\
	$(CPPFLAGS_EXTRA)			\
	-I$(srcdir)/../common
rina_echo_time_CXXFLAGS =			\
	$(CPPFLAGS_EXTRA)

bin_PROGRAMS            += rina-echo-time
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT += rina-echo-time

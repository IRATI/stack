#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

bin_PROGRAMS                       =
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT =

rina_echo_time_SOURCES  =				\
	application.cc         application.h		\
	server.cc              server.h			\
	client.cc              client.h			\
	main.cc
rina_echo_time_LDADD    = $(LIBRINA_LIBS) -lrt
rina_echo_time_CPPFLAGS =			\
	$(LIBRINA_CFLAGS)			\
	$(CPPFLAGS_EXTRA)			\
	-std=c++11				\
	-I$(srcdir)/../common
bin_PROGRAMS            += rina-echo-time
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT += rina-echo-time

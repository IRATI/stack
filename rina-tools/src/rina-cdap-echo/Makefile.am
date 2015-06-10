#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

bin_PROGRAMS                       =
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT =

rina_cdap_echo_SOURCES  =				\
	cdap-echo-server.cc              cdap-echo-server.h			\
	cdap-echo-client.cc              cdap-echo-client.h			\
	main.cc
	
rina_cdap_echo_LDADD    = $(LIBRINA_LIBS) -lrt \
	../common/librinaapp.la	
	
rina_cdap_echo_CPPFLAGS =			\
	$(LIBRINA_CFLAGS)			\
	$(CPPFLAGS_EXTRA)			\
	-I$(srcdir)/../common
rina_cdap_echo_CXXFLAGS =			\
	$(CPPFLAGS_EXTRA)

bin_PROGRAMS            += rina-cdap-echo
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT += rina-cdap-echo

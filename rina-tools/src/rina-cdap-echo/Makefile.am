#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

bin_PROGRAMS                       =
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT =

rina_cdap_echo_SOURCES  =			\
	main.cc
rina_cdap_echo_LDADD    = $(LIBRINA_LIBS) -lrt
rina_cdap_echo_CPPFLAGS =			\
	$(LIBRINA_CFLAGS)			\
	$(CPPFLAGS_EXTRA)			\
	-std=c++11				\
	-I$(srcdir)/../common
bin_PROGRAMS            += rina-cdap-echo
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT += rina-cdap-echo

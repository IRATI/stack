#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
# modified by Steve Bunch @ trianetworksystems.com
#

bin_PROGRAMS                       =
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT =

rina_cat_SOURCES  =				\
	cat_server.cc              cat_server.h			\
	cat_client.cc              cat_client.h			\
	cat_utils.cc		   cat_utils.h			\
	main.cc
rina_cat_LDADD    = $(LIBRINA_LIBS) -lrt \
	../common/librinaapp.la	 -lrina-api
rina_cat_CPPFLAGS =			\
	$(LIBRINA_CFLAGS)			\
	$(CPPFLAGS_EXTRA)			\
	-I$(srcdir)/../common
rina_cat_CXXFLAGS =			\
	$(CPPFLAGS_EXTRA)

bin_PROGRAMS            += rina-cat
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT += rina-cat

#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
# modified by Steve Bunch @ trianetworksystems.com
#

bin_PROGRAMS                       =
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT =

rinacat_SOURCES  =				\
	rinacat.c
rinacat_LDADD    = $(LIBRINA_LIBS) -lrt \
	-lrina-api
rinacat_CFLAGS =			\
	-std=gnu11				\
	$(LIBRINA_CFLAGS)			\
	-I$(srcdir)/../common
rinacat_CXXFLAGS =			\
	$(CPPFLAGS_EXTRA)

bin_PROGRAMS            += rinacat
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT += rinacat

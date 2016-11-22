#
# Makefile.am
#
# Written by: Eduard Grasa <eduard DOT grasa AT i2cat DOT net>
#

bin_PROGRAMS                       =
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT =

key_managers_SOURCES  =				\
	ckm.cc              ckm.h			\
	kma.cc              kma.h			\
	main.cc
key_managers_LDADD    = $(LIBRINA_LIBS) -lrt \
	../common/librinaapp.la	
key_managers_CPPFLAGS =			\
	$(LIBRINA_CFLAGS)			\
	$(CPPFLAGS_EXTRA)			\
	-I$(srcdir)/../common
key_managers_CXXFLAGS =			\
	$(CPPFLAGS_EXTRA)

bin_PROGRAMS            += key_managers
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT += key_managers

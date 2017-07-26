#
# Makefile.am
#
# Written by: Eduard Grasa <eduard DOT grasa AT i2cat DOT net>
#

CLEANFILES       =
MOSTLYCLEANFILES =
EXTRA_DIST       =
bin_PROGRAMS                       =
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT =

include $(srcdir)/Makefile.inc

BUILT_SOURCES = $(protoSOURCES)

key_managers_SOURCES  =				\
    km-common.h         km-common.cc    \
	ckm.cc              ckm.h			\
	kma.cc              kma.h			\
	main.cc \
	$(protoSOURCES)
key_managers_LDADD    = $(LIBRINA_LIBS) -ldl \
	../common/librinaapp.la	
key_managers_CPPFLAGS =			\
	$(LIBRINA_CFLAGS)			\
	$(CPPFLAGS_EXTRA)			\
    $(LIBPROTOBUF_CFLAGS)  		\
	-I$(srcdir)/../common
key_managers_CXXFLAGS =			\
	$(CPPFLAGS_EXTRA)

bin_PROGRAMS            += key_managers
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT += key_managers

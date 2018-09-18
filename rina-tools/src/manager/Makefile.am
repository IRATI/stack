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

net_manager_SOURCES  =				\
    dif-template-manager.h  dif-template-manager.cc \
    net-manager.h    net-manager.cc \
	main.cc \
	$(protoSOURCES)
net_manager_LDADD    =          \
    $(LIBRINA_LIBS)             \
    $(LIBRINA_API_LIBS) -ldl    \
    $(builddir)/../../../rinad/src/common/librinad.la
net_manager_CPPFLAGS =			\
	$(LIBRINA_CFLAGS)			\
	$(CPPFLAGS_EXTRA)			\
    $(LIBPROTOBUF_CFLAGS)  		\
    $(LIBRINA_API_CFLAGS)       \
	-I$(srcdir)/../common       \
	-I$(srcdir)/../../../rinad/src/common
net_manager_CXXFLAGS =			\
	$(CPPFLAGS_EXTRA)

bin_PROGRAMS            += net_manager
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT += net_manager

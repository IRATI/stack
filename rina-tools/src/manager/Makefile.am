#
# Makefile.am
#
# Written by: Eduard Grasa <eduard DOT grasa AT i2cat DOT net>
#

CLEANFILES       =
MOSTLYCLEANFILES =
EXTRA_DIST       =
bin_PROGRAMS                       =
bin_SCRIPTS                        =
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT =

include $(srcdir)/Makefile.inc

BUILT_SOURCES = $(protoSOURCES)

net_manager_SOURCES  =				\
    dif-template-manager.h  dif-template-manager.cc \
    net-manager.h    net-manager.cc  promise.cc \
	main.cc \
	$(protoSOURCES)
net_manager_LDADD    =          \
    $(LIBRINA_LIBS)             \
    $(LIBRINA_API_LIBS) -ldl    \
    $(LIBRINAJSONCPP_LIBS) -ldl    \
    $(LIBRINAD_LIBS)
net_manager_CPPFLAGS =			\
	$(LIBRINA_CFLAGS)			\
	$(CPPFLAGS_EXTRA)			\
    $(LIBPROTOBUF_CFLAGS)  		\
    $(LIBRINA_API_CFLAGS)       \
	-I$(srcdir)/../common       \
	$(LIBRINAD_CFLAGS)
net_manager_CXXFLAGS =			\
	$(CPPFLAGS_EXTRA)

bin_PROGRAMS            += net_manager
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT += net_manager

bin_SCRIPTS    += mgr-ctl
EXTRA_DIST     += mgr-ctl

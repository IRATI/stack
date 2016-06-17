#
# Makefile.am
#
# Written by: Vincenzo Maffione <v DOT maffione AT nextworks DOT it>
#

EXTRA_DIST   =
CLEANFILES   =

COMMONCPPFLAGS =				\
	$(CPPFLAGS_EXTRA)			\
	$(LIBRINA_CFLAGS)			\
	-I$(top_srcdir)/src			\
	-I$(top_srcdir)/src/common
	
COMMONLIBS =								\
	$(builddir)/../../../common/librinad.la	\
	$(LIBRINA_LIBS)

pluginsdir          = $(pkglibdir)/ipcp
plugins_LTLIBRARIES = 

default_la_CFLAGS =
default_la_CPPFLAGS =				\
	$(COMMONCPPFLAGS)
default_la_LIBADD = $(COMMONLIBS) -ldl
default_la_LDFLAGS = -module
default_la_SOURCES =		 \
	plugin.cc		 \
	../../components.h	 \
	../../components.cc	 \
	security-manager-ps.cc	 \
	flow-allocator-ps.cc     \
        namespace-manager-ps.cc  \
        resource-allocator-ps.cc \
        enrollment-task-ps.cc    \
        routing-ps.h 	 	 \
        routing-ps.cc

plugins_LTLIBRARIES += default.la

testsCPPFLAGS = $(COMMONCPPFLAGS)
testsLIBS =	$(COMMONLIBS)
	
test_routing_SOURCES  =					    \
	test-routing.cc							\
	../../components.cc	   ../../components.h \
	../../utils.cc	   ../../utils.h \
	../../ipc-process.cc	   ../../ipc-process.h \
	../../namespace-manager.cc ../../namespace-manager.h \
	../../flow-allocator.cc    ../../flow-allocator.h \
	../../enrollment-task.cc    ../../enrollment-task.h \
	../../resource-allocator.cc    ../../resource-allocator.h \
	../../rib-daemon.h	   ../../rib-daemon.cc \
	../../routing.cc           \
	../../security-manager.cc \
	routing-ps.cc 	     routing-ps.h
test_routing_CPPFLAGS = $(testsCPPFLAGS) \
			-DPLUGINSDIR=\"$(pkglibdir)/ipcp\"
test_routing_LDADD    = $(testsLIBS)

test_encoders_SOURCES  =			\
	test-encoders.cc			\
	../../components.cc	   ../../components.h \
	../../utils.cc	   ../../utils.h \
	../../ipc-process.cc	   ../../ipc-process.h \
	../../namespace-manager.cc ../../namespace-manager.h \
	../../flow-allocator.cc    ../../flow-allocator.h \
	../../enrollment-task.cc    ../../enrollment-task.h \
	../../resource-allocator.cc    ../../resource-allocator.h \
	../../rib-daemon.h	   ../../rib-daemon.cc \
	../../routing.cc           \
	../../security-manager.cc \
	routing-ps.cc 	     routing-ps.h
test_encoders_CPPFLAGS = $(testsCPPFLAGS) \
			-DPLUGINSDIR=\"$(pkglibdir)/ipcp\"
test_encoders_LDADD    = $(testsLIBS)

check_PROGRAMS =				\
	test-routing test-encoders

XFAIL_TESTS =
PASS_TESTS  = test-routing test-encoders

TESTS = $(PASS_TESTS) $(XFAIL_TESTS)

dist_plugins_DATA = default.manifest rina-default-plugin.manifest

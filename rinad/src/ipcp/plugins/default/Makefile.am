#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

EXTRA_DIST   =
CLEANFILES   =

COMMONCPPFLAGS =				\
	$(CPPFLAGS_EXTRA)			\
	$(LIBRINA_CFLAGS)			\
	-I$(top_srcdir)/src			\
	-I$(top_srcdir)/src/common

pluginsdir          = $(pkglibdir)/ipcp
plugins_LTLIBRARIES = 

default_la_CFLAGS =
default_la_CPPFLAGS =				\
	$(COMMONCPPFLAGS)
default_la_LIBADD = $(COMMONLIBS) -ldl
default_la_LDFLAGS = -module
default_la_SOURCES =			 \
		plugin.cc		 		 \
		../../components.h		 \
		../../components.cc		 \
		security-manager-ps.cc	 \
		flow-allocator-ps.cc     \
        namespace-manager-ps.cc  \
        resource-allocator-ps.cc \
        routing-ps.h 	 		 \
        routing-ps.cc

plugins_LTLIBRARIES += default.la

testsCPPFLAGS = $(COMMONCPPFLAGS)
testLIBS =									\
	$(builddir)/../../../common/librinad.la	\
	$(LIBRINA_LIBS)
	
test_routing_SOURCES  =					    \
	test-routing.cc							\
	../../components.cc	 ../../components.h \
	../../events.cc      ../../events.h	    \
	routing-ps.cc 	     routing-ps.h
test_routing_CPPFLAGS = $(testsCPPFLAGS)
test_routing_LDADD    = $(testsLIBS)

check_PROGRAMS =				\
	test-routing

XFAIL_TESTS =
PASS_TESTS  = test-routing

TESTS = $(PASS_TESTS) $(XFAIL_TESTS)

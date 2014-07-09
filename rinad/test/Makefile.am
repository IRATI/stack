#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

include Makefile.inc

EXTRA_DIST =

EXTRA_DIST += run-java.in
EXTRA_DIST += SWIGTest.java

COMMONCPPFLAGS =				\
	-I$(top_srcdir)/src/common			\
	-I$(top_srcdir)/../librina/include \
	$(LIBNL3_CFLAGS)			\
	$(LIBNLGENL3_CFLAGS)			\
	$(CPPFLAGS_EXTRA)

COMMONLDFLAGS =					\
	$(LIBNL3_LIBS)				\
	$(LIBNLGENL3_LIBS)

REGRESSIONLDFLAGS = $(COMMONLDFLAGS) $(top_builddir)/src/librina-stubbed.la
FUNCTIONALLDFLAGS = $(COMMONLDFLAGS) $(top_builddir)/src/librina.la

#
# Regression tests
#


#
# Functional tests
#
test_encoders_SOURCES  = test-encoders.cc
test_encoders_CPPFLAGS = $(COMMONCPPFLAGS) -I$(top_srcdir)/src/common
test_encoders_LDFLAGS  = $(FUNCTIONALLDFLAGS)

check_PROGRAMS =				\
	test-encoders

XFAIL_TESTS =

PASS_TESTS =

TESTS = $(PASS_TESTS) $(XFAIL_TESTS)

FUNCTIONAL_PASS_TESTS =				\
	test-encoders

FUNCTIONAL_XFAIL_TESTS =

FUNCTIONAL_TESTS = $(FUNCTIONAL_PASS_TESTS) $(FUNCTIONAL_XFAIL_TESTS)

KERNEL_DEBUG_PASS_TESTS =

KERNEL_DEBUG_XFAIL_TESTS =

KERNEL_DEBUG_TESTS = $(KERNEL_DEBUG_PASS_TESTS) $(KERNEL_DEBUG_XFAIL_TESTS)

check-kernel-debug: check-kernel-debug-am

check-functional: check-functional-am

check-all: check check-functional

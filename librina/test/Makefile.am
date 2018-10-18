#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

include Makefile.inc

EXTRA_DIST =

if BUILD_BINDINGS_JAVA

edit = $(SED)							\
        -e 's|@abs_top_builddir[@]|$(abs_top_builddir)|g'	\
        -e 's|@builddir[@]|$(builddir)|g'			\
        -e 's|@libdir[@]|$(libdir)|g'				\
        -e 's|@SHELL[@]|$(SHELL)|g'				\
        -e 's|@JAVA[@]|$(JAVA)|g'				\
        -e 's|@JAVAC[@]|$(JAVAC)|g'

run-java: Makefile run-java.in
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.in >$@.tmp
	chmod +x $@.tmp
	chmod a-w $@.tmp
	mv $@.tmp $@

CLEANFILES = run-java

noinst_SCRIPTS = run-java

endif

EXTRA_DIST += run-java.in
EXTRA_DIST += SWIGTest.java

COMMONCPPFLAGS =				\
	-I$(top_srcdir)/include			\
	$(LIBNL3_CFLAGS)			\
	$(LIBNLGENL3_CFLAGS)			\
	$(CPPFLAGS_EXTRA)

COMMONCXXFLAGS =				\
	$(CXXFLAGS_EXTRA)

COMMONLDFLAGS =					\
	$(LIBNL3_LIBS)				\
	$(LIBNLGENL3_LIBS)

REGRESSIONLDFLAGS = $(COMMONLDFLAGS) $(top_builddir)/src/librinairati.la $(top_builddir)/src/librina-stubbed.la
FUNCTIONALLDFLAGS = $(COMMONLDFLAGS) $(top_builddir)/src/librinairati.la $(top_builddir)/src/librina.la

#
# Regression tests
#

test_01_SOURCES  = test-01.cc
test_01_CPPFLAGS = $(COMMONCPPFLAGS) -I$(top_srcdir)/src
test_01_CXXFLAGS = $(COMMONCXXFLAGS)
test_01_LDFLAGS  = $(REGRESSIONLDFLAGS)

test_02_SOURCES  = test-02.cc
test_02_CPPFLAGS = $(COMMONCPPFLAGS) -I$(top_srcdir)/src
test_02_CXXFLAGS = $(COMMONCXXFLAGS)
test_02_LDFLAGS  = $(REGRESSIONLDFLAGS)

test_03_SOURCES  = test-03.cc
test_03_CPPFLAGS = $(COMMONCPPFLAGS) -I$(top_srcdir)/src
test_03_CXXFLAGS = $(COMMONCXXFLAGS)
test_03_LDFLAGS  = $(REGRESSIONLDFLAGS)

#
# Functional tests
#

test_parsers_SOURCES  = test-parsers.cc
test_parsers_CPPFLAGS = $(COMMONCPPFLAGS) -I$(top_srcdir)/src
test_parsers_CXXFLAGS = $(COMMONCXXFLAGS)
test_parsers_LDFLAGS  = $(FUNCTIONALLDFLAGS)

test_concurrency_SOURCES  = test-concurrency.cc
test_concurrency_CPPFLAGS = $(COMMONCPPFLAGS) -I$(top_srcdir)/src
test_concurrency_CXXFLAGS = $(COMMONCXXFLAGS)
test_concurrency_LDFLAGS  = $(FUNCTIONALLDFLAGS)

test_timer_SOURCES  = test-timer.cc
test_timer_CPPFLAGS = $(COMMONCPPFLAGS) -I$(top_srcdir)/src
test_timer_CXXFLAGS = $(COMMONCXXFLAGS)
test_timer_LDFLAGS  = $(FUNCTIONALLDFLAGS)

check_PROGRAMS =				\
	test-01					\
	test-02					\
	test-03					\
	test-parsers			\
	test-concurrency			\
	test-timer				

XFAIL_TESTS =				\
	test-03

PASS_TESTS =					\
	test-01					\
	test-02					

TESTS = $(PASS_TESTS) $(XFAIL_TESTS)			
	
FUNCTIONAL_PASS_TESTS = \
	test-parsers \
	test-concurrency \
	test-timer

FUNCTIONAL_XFAIL_TESTS =

FUNCTIONAL_TESTS = $(FUNCTIONAL_PASS_TESTS) $(FUNCTIONAL_XFAIL_TESTS)

KERNEL_DEBUG_XFAIL_TESTS =

KERNEL_DEBUG_TESTS = $(KERNEL_DEBUG_PASS_TESTS) $(KERNEL_DEBUG_XFAIL_TESTS)

check-kernel-debug: check-kernel-debug-am

check-functional: check-functional-am

check-all: check check-functional			
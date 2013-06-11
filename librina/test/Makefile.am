#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

EXTRA_DIST =

if BUILD_WRAPPERS_JAVA

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

COMMONLDFLAGS =					\
	$(LIBNL3_LIBS)				\
	$(LIBNLGENL3_LIBS)			\
	$(top_builddir)/src/librina.la

test_01_SOURCES  = test-01.cc
test_01_CPPFLAGS = $(COMMONCPPFLAGS)
test_01_LDFLAGS  = $(COMMONLDFLAGS)

test_02_SOURCES  = test-02.cc
test_02_CPPFLAGS = $(COMMONCPPFLAGS)
test_02_LDFLAGS  = $(COMMONLDFLAGS)

test_03_SOURCES  = test-03.cc
test_03_CPPFLAGS = $(COMMONCPPFLAGS)
test_03_LDFLAGS  = $(COMMONLDFLAGS)

test_netlink_SOURCES  = test-netlink.cc
test_netlink_CPPFLAGS = $(COMMONCPPFLAGS)
test_netlink_LDFLAGS  = $(COMMONLDFLAGS)

check_PROGRAMS =				\
	test-01					\
	test-02					\
	test-03					\
	test-netlink

XFAIL_TESTS =					\
	test-03

PASS_TESTS =					\
	test-01					\
	test-02

# Explicitly list the tests, in order to ensure ordering
TESTS =						\
	test-01					\
	test-02					\
	test-03

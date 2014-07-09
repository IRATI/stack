#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

COMMONCPPFLAGS =				\
	$(CPPFLAGS_EXTRA)			\
	$(LIBRINA_CFLAGS)			\
	-I$(srcdir)/..				\
	-I$(srcdir)/../common
COMMONLDADD = $(builddir)/../src/common/librinad.la $(LIBRINA_LIBS)

test_encoders_SOURCES  = test-encoders.cc
test_encoders_CPPFLAGS = $(COMMONCPPFLAGS) -I$(top_srcdir)/src/common
test_encoders_LDADD    = $(COMMONLDADD)

check_PROGRAMS =				\
	test-encoders

XFAIL_TESTS =
PASS_TESTS  = test-encoders

TESTS = $(PASS_TESTS) $(XFAIL_TESTS)

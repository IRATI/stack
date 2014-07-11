#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

bin_PROGRAMS = ipcp

ipcp_CFLAGS   =
ipcp_CPPFLAGS =					\
	$(CPPFLAGS_EXTRA)			\
	$(LIBRINA_CFLAGS)			\
	-I$(srcdir)/..				\
	-I$(srcdir)/../common
ipcp_LDADD    = $(builddir)/../common/librinad.la $(LIBRINA_LIBS)
ipcp_SOURCES  =						\
	main.cc						\
	events.cc		events.h		\
	components.cc		components.h		\
	rib-daemon.cc		rib-daemon.h		\
	enrollment-task.cc	enrollment-task.h	\
	namespace-manager.cc	namespace-manager.h	\
	pduft-generator.cc      pduft-generator.h	\
	resource-allocator.cc	resource-allocator.h	\
	flow-allocator.cc       flow-allocator.h

test_encoders_SOURCES  =			\
	test-encoders.cc			\
	components.cc	  components.h		\
	rib-daemon.cc     rib-daemon.h		\
	flow-allocator.cc flow-allocator.h
test_encoders_CPPFLAGS =			\
	$(CPPFLAGS_EXTRA)			\
	$(LIBRINA_CFLAGS)			\
	-I$(srcdir)/..				\
	-I$(srcdir)/../common
test_encoders_LDADD    = $(builddir)/../common/librinad.la $(LIBRINA_LIBS)

test_pduftg_SOURCES  =			\
	test-pduftg.cc			\
	components.cc	  components.h		\
	events.cc         events.h          \
	pduft-generator.cc pduft-generator.h
test_pduftg_CPPFLAGS =			\
	$(CPPFLAGS_EXTRA)			\
	$(LIBRINA_CFLAGS)			\
	-I$(srcdir)/..				\
	-I$(srcdir)/../common
test_pduftg_LDADD    = $(builddir)/../common/librinad.la $(LIBRINA_LIBS)

check_PROGRAMS =				\
	test-encoders               \
	test-pduftg

XFAIL_TESTS =
PASS_TESTS  = test-encoders   \
              test-pduftg

TESTS = $(PASS_TESTS) $(XFAIL_TESTS)

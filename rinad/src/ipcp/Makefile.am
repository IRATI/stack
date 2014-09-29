#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

EXTRA_DIST   =
CLEAN_FILES  =

bin_PROGRAMS = ipcp

COMMONCPPFLAGS =				\
	$(CPPFLAGS_EXTRA)			\
	$(LIBRINA_CFLAGS)			\
	-I$(srcdir)/..				\
	-I$(srcdir)/../common

COMMONLIBS =					\
	$(builddir)/../common/librinad.la	\
	$(LIBRINA_LIBS)

ipcp_CFLAGS   =
ipcp_CPPFLAGS = $(COMMONCPPFLAGS)
ipcp_LDADD    = $(COMMONLIBS)
ipcp_SOURCES  =						\
	main.cc						\
	events.cc		events.h		\
	components.cc		components.h		\
	rib-daemon.cc		rib-daemon.h		\
	enrollment-task.cc	enrollment-task.h	\
	namespace-manager.cc	namespace-manager.h	\
	pduft-generator.cc      pduft-generator.h	\
	resource-allocator.cc	resource-allocator.h	\
	flow-allocator.cc       flow-allocator.h   	\
	security-manager.cc     security-manager.h 	\
	ipc-process.cc          ipc-process.h

testsCPPFLAGS = $(COMMONCPPFLAGS)
testsLIBS     = $(COMMONLIBS)

test_encoders_SOURCES  =			\
	test-encoders.cc			\
	components.cc	     components.h	\
	events.cc            events.h		\
	rib-daemon.cc        rib-daemon.h	\
	flow-allocator.cc    flow-allocator.h	\
	enrollment-task.cc   enrollment-task.h	\
        pduft-generator.cc   pduft-generator.h
test_encoders_CPPFLAGS = $(testsCPPFLAGS)
test_encoders_LDADD    = $(testsLIBS)

test_pduftg_SOURCES  =				\
	test-pduftg.cc				\
	components.cc	   components.h		\
	events.cc          events.h		\
	pduft-generator.cc pduft-generator.h
test_pduftg_CPPFLAGS = $(testsCPPFLAGS)
test_pduftg_LDADD    = $(testsLIBS)

check_PROGRAMS =				\
	test-encoders				\
	test-pduftg

XFAIL_TESTS =
PASS_TESTS  = test-encoders   \
              test-pduftg

TESTS = $(PASS_TESTS) $(XFAIL_TESTS)

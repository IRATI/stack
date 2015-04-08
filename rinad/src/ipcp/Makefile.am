#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

SUBDIRS      = include plugins

EXTRA_DIST   =
CLEANFILES   =

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
ipcp_CPPFLAGS =					\
	$(COMMONCPPFLAGS)			\
	-DPLUGINSDIR=\"$(pkglibdir)/ipcp\"
ipcp_CXXFLAGS =					\
	-std=c++98				\
	$(CXXFLAGS_EXTRA)
ipcp_LDADD    = $(COMMONLIBS) -ldl
ipcp_SOURCES  =						\
	main.cc						\
	events.cc		events.h		\
	components.cc		components.h		\
	rib-daemon.cc		rib-daemon.h		\
	enrollment-task.cc	enrollment-task.h	\
	namespace-manager.cc	namespace-manager.h	\
	resource-allocator.cc	resource-allocator.h	\
	flow-allocator.cc       flow-allocator.h   	\
	security-manager.cc				\
	routing.cc                      \
	ipc-process.cc          ipc-process.h

testsCPPFLAGS = $(COMMONCPPFLAGS)
testsLIBS     = $(COMMONLIBS)

pluginsdir          = $(pkglibdir)/ipcp
plugins_LTLIBRARIES = 

default_la_CFLAGS =
default_la_CPPFLAGS =				\
	$(COMMONCPPFLAGS)
default_la_CXXFLAGS =				\
	-std=c++98				\
	$(CXXFLAGS_EXTRA)
default_la_LIBADD = $(COMMONLIBS) -ldl
default_la_LDFLAGS = -module
default_la_SOURCES =				\
		default-plugin.cc		\
		components.h			\
		default-security-manager-ps.cc	\
		default-flow-allocator-ps.cc    \
        default-namespace-manager-ps.cc \
        default-resource-allocator-ps.cc \
        link-state-routing-ps.h \
        link-state-routing-ps.cc

plugins_LTLIBRARIES += default.la

test_encoders_SOURCES  =			\
	test-encoders.cc			\
	components.cc	     components.h	\
	events.cc            events.h		\
	rib-daemon.cc        rib-daemon.h	\
	flow-allocator.cc    flow-allocator.h	\
	enrollment-task.cc   enrollment-task.h	\
    link-state-routing-ps.cc   link-state-routing-ps.h
test_encoders_CPPFLAGS = $(testsCPPFLAGS)
test_encoders_CXXFLAGS =			\
	-std=c++98				\
	$(CXXFLAGS_EXTRA)
test_encoders_LDADD    = $(testsLIBS)

test_pduftg_SOURCES  =				\
	test-link-state-routing.cc				\
	components.cc	   components.h		\
	events.cc          events.h		\
	link-state-routing-ps.cc link-state-routing-ps.h
test_pduftg_CPPFLAGS = $(testsCPPFLAGS)
test_pduftg_CXXFLAGS =				\
	-std=c++98				\
	$(CXXFLAGS_EXTRA)
test_pduftg_LDADD    = $(testsLIBS)

check_PROGRAMS =				\
	test-encoders				\
	test-link-state-routing

XFAIL_TESTS =
PASS_TESTS  = test-encoders   \
              test-link-state-routing

TESTS = $(PASS_TESTS) $(XFAIL_TESTS)

edit = $(SED)						\
	-e 's|@SHELL[@]|$(SHELL)|g'			\
	-e 's|@PKG_CONFIG[@]|$(PKG_CONFIG)|g'		\
        -e 's|@PKG_CONFIG_PATH[@]|$(PKG_CONFIG_PATH)|g' \
	-e 's|@PACKAGE_VERSION[@]|$(PACKAGE_VERSION)|g'	\
	-e 's|@PACKAGE_URL[@]|$(PACKAGE_VERSION)|g'	\
	-e 's|@prefix[@]|$(prefix)|g'			\
	-e 's|@includedir[@]|$(includedir)|g'		\
	-e 's|@pkgincludedir[@]|$(pkgincludedir)|g'	\
	-e 's|@pkglibdir[@]|$(pkglibdir)|g'		\
	-e 's|@libdir[@]|$(libdir)|g'

rinad-ipcp.pc: Makefile rinad-ipcp.pc.in
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.in >$@.tmp
	chmod a-w $@.tmp
	mv $@.tmp $@

EXTRA_DIST += rinad-ipcp.pc.in
CLEANFILES += rinad-ipcp.pc

pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = rinad-ipcp.pc

bin_SCRIPTS = ipcp-plugins

ipcp-plugins: Makefile ipcp-plugins.in
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.in >$@.tmp
	chmod a-w $@.tmp
	chmod u+x $@.tmp
	mv $@.tmp $@

EXTRA_DIST += ipcp-plugins.in
CLEANFILES += ipcp-plugins

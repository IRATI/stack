#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

SUBDIRS      = plugins

EXTRA_DIST   =
CLEANFILES   =

bin_PROGRAMS = ipcp

COMMONCPPFLAGS =				\
	$(CPPFLAGS_EXTRA)			\
	$(LIBRINA_CFLAGS)			\
	-I$(srcdir)/../				\
	-I$(srcdir)/../common

COMMONLIBS =					\
	$(builddir)/../common/librinad.la	\
	$(LIBRINA_LIBS)

ipcp_CFLAGS   =
ipcp_CPPFLAGS =					\
	$(COMMONCPPFLAGS)			\
	-DPLUGINSDIR=\"$(pkglibdir)/ipcp\"
ipcp_LDADD    = $(COMMONLIBS)
ipcp_LDFLAGS = -Wl,--export-dynamic
ipcp_SOURCES  =						\
	main.cc						\
	components.cc		components.h		\
	utils.cc		utils.h			\
	rib-daemon.cc		rib-daemon.h		\
	enrollment-task.cc	enrollment-task.h	\
	namespace-manager.cc	namespace-manager.h	\
	resource-allocator.cc	resource-allocator.h	\
	flow-allocator.cc       flow-allocator.h   	\
	security-manager.cc				\
	routing.cc                      \
	ipc-process.cc          ipc-process.h

ipcpheadersdir = $(includedir)/rinad/ipcp
ipcpheaders_HEADERS = components.h		\
		ipcp-logging.h

testsCPPFLAGS = $(COMMONCPPFLAGS)
testsLIBS     = $(COMMONLIBS)

test_encoders_SOURCES  =			\
	test-encoders.cc			\
	components.cc	   components.h \
	ipc-process.cc	   ipc-process.h \
	namespace-manager.cc namespace-manager.h \
	flow-allocator.cc    flow-allocator.h \
	enrollment-task.cc    enrollment-task.h \
	resource-allocator.cc    resource-allocator.h \
	rib-daemon.h	   rib-daemon.cc \
	routing.cc          security-manager.cc
test_encoders_CPPFLAGS = $(testsCPPFLAGS)
test_encoders_LDADD    = $(testsLIBS)

check_PROGRAMS =				\
	test-encoders		

XFAIL_TESTS =
PASS_TESTS  = test-encoders 

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

#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

SUBDIRS=addons

EXTRA_DIST   =
CLEANFILES   =


CPPFLAGS_EXTRA =					\
	-DDEFAULT_BINDIR=\"$(bindir)\"			\
	-DDEFAULT_LIBDIR=\"$(libdir)\"			\
	-DDEFAULT_LOGDIR=\"$(localstatedir)/log\"	\
	-DDEFAULT_RUNDIR=\"$(runstatedir)\"

#
#IPCM
#
bin_PROGRAMS = ipcm
ipcm_CFLAGS   =
ipcm_CPPFLAGS =					\
	$(CPPFLAGS_EXTRA)			\
	$(LIBRINA_CFLAGS)			\
	$(LIBRINA_API_CFLAGS)			\
	-I$(srcdir)/..				\
	-I$(srcdir)/../common			\
	-DPLUGINSDIR=\"$(pkglibdir)/ipcm\"	\
	-DIPCPPLUGINSDIR=\"$(pkglibdir)/ipcp\"

ipcm_LDADD    =					\
	$(builddir)/../common/librinad.la	\
	$(builddir)/addons/libaddons.la		\
	$(LIBRINA_API_LIBS) \
	$(LIBRINA_LIBS)
ipcm_SOURCES  =								\
	addon.cc			addon.h				\
	app-handlers.cc			app-handlers.h			\
	dif-validator.cc		dif-validator.h			\
	helpers.cc							\
	main.cc								\
	misc-handlers.cc		misc-handlers.h			\
	ipcm.cc				ipcm.h				\
	ipcp.cc				ipcp.h				\
	ipcp-handlers.cc		ipcp-handlers.h			\
	configuration.cc		configuration.h			\
	flow-alloc-handlers.cc		flow-alloc-handlers.h		\
	policies-handlers.cc						\
	dif-allocator.cc		dif-allocator.h			\
	dif-template-manager.cc		dif-template-manager.h		\
	process-event-listener.cc process-event-listener.h \
	ip-vpn-manager.cc   ip-vpn-manager.h \
	catalog.cc			catalog.h

test_empty_SOURCES  =				\
	test-empty.cc
test_empty_CPPFLAGS =				\
	$(CPPFLAGS_EXTRA)			\
	$(LIBRINA_CFLAGS)			\
	$(LIBRINA_API_CFLAGS)			\
	-I$(srcdir)/..				\
	-I$(srcdir)/../common
test_empty_LDADD    = $(builddir)/../common/librinad.la $(LIBRINA_LIBS) $(LIBRINA_API_LIBS)

check_PROGRAMS =				\
	test-empty

XFAIL_TESTS =
PASS_TESTS  = test-empty

TESTS = $(PASS_TESTS) $(XFAIL_TESTS)


edit = $(SED)						\
	-e 's|@SHELL[@]|$(SHELL)|g'			\
	-e 's|@PKG_CONFIG[@]|$(PKG_CONFIG)|g'		\
	-e 's|@PKG_CONFIG_PATH[@]|$(PKG_CONFIG_PATH)|g'	\
	-e 's|@PACKAGE_VERSION[@]|$(PACKAGE_VERSION)|g'	\
	-e 's|@PACKAGE_URL[@]|$(PACKAGE_VERSION)|g'	\
	-e 's|@prefix[@]|$(prefix)|g'			\
	-e 's|@includedir[@]|$(includedir)|g'		\
	-e 's|@pkgincludedir[@]|$(pkgincludedir)|g'	\
	-e 's|@pkglibdir[@]|$(pkglibdir)|g'		\
	-e 's|@libdir[@]|$(libdir)|g'

rinad-ipcm.pc: Makefile rinad-ipcm.pc.in
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.in >$@.tmp
	chmod a-w $@.tmp
	mv $@.tmp $@

EXTRA_DIST += rinad-ipcm.pc.in
CLEANFILES += rinad-ipcm.pc

pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = rinad-ipcm.pc

# --runstatedir is new to Autoconf 2.70
runstatedir ?= $(localstatedir)/run

install-data-local:
	$(MKDIR_P) $(runstatedir)

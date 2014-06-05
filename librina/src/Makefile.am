#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

allsources =					\
	logs.cc					\
	concurrency.cc				\
	core.cc					\
	librina.cc				\
	common.cc				\
	application.cc				\
	ipc-manager.cc				\
	ipc-process.cc				\
	faux-sockets.cc				\
	cdap.cc					\
	sdu-protection.cc			\
	netlink-messages.cc			\
	netlink-parsers.cc			\
	netlink-manager.cc			\
	syscalls-wrappers.cc

EXTRA_DIST =					\
	logs.h					\
	utils.h					\
	core.h					\
	netlink-messages.h			\
	netlink-manager.h			\
	netlink-parsers.h			\
	rina-syscalls.h

librina_la_SOURCES  = $(allsources)
librina_la_LDFLAGS  =
librina_la_LIBADD   =				\
	$(LIBNL3_LIBS)				\
	$(LIBNLGENL3_LIBS)
librina_la_CPPFLAGS =				\
	-DSTUB_API=0				\
	-I$(top_srcdir)/include			\
	-I$(top_srcdir)/include-system		\
	$(LIBNL3_CFLAGS)			\
	$(LIBNLGENL3_CFLAGS)			\
	$(CPPFLAGS_EXTRA)

librina_stubbed_la_SOURCES  = $(allsources)
librina_stubbed_la_LDFLAGS  =
librina_stubbed_la_LIBADD   =			\
	$(LIBNL3_LIBS)				\
	$(LIBNLGENL3_LIBS)
librina_stubbed_la_CPPFLAGS =			\
	-DSTUB_API=1				\
	-I$(top_srcdir)/include			\
	-I$(top_srcdir)/include-system		\
	$(LIBNL3_CFLAGS)			\
	$(LIBNLGENL3_CFLAGS)			\
	$(CPPFLAGS_EXTRA)

lib_LTLIBRARIES    = librina.la
noinst_LTLIBRARIES = librina-stubbed.la

EXTRA_DIST +=					\
	librina.pc.in

edit = $(SED)							\
        -e 's|@PACKAGE_VERSION[@]|$(PACKAGE_VERSION)|g'		\
	-e 's|@LIBNL_MIN_VERSION[@]|$(LIBNL_MIN_VERSION)|g'     \
	-e 's|@LIBNL3_LIBS[@]|$(LIBNL3_LIBS)|g'			\
	-e 's|@LIBNLGENL3_LIBS[@]|$(LIBNLGENL3_LIBS)|g'		\
	-e 's|@LIBNL3_CFLAGS[@]|$(LIBNL3_CFLAGS)|g'		\
	-e 's|@LIBNLGENL3_CFLAGS[@]|$(LIBNLGENL3_CFLAGS)|g'	\
        -e 's|@prefix[@]|$(prefix)|g'				\
        -e 's|@includedir[@]|$(includedir)|g'			\
        -e 's|@libdir[@]|$(libdir)|g'

librina.pc: Makefile librina.pc.in
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.in >$@.tmp
	chmod +x $@.tmp
	chmod a-w $@.tmp
	mv $@.tmp $@

CLEANFILES = librina.pc

pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = librina.pc

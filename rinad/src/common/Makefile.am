#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

EXTRA_DIST =

lib_LTLIBRARIES = librinad.la

librinad_la_CFLAGS   =
librinad_la_CPPFLAGS = $(CPPFLAGS_EXTRA) $(LIBRINA_CFLAGS)
librinad_la_LIBADD   = $(LIBRINA_LIBS)
librinad_la_SOURCES  =				\
	options.cc

EXTRA_DIST +=					\
	librinad.pc.in

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

librinad.pc: Makefile librinad.pc.in
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.in >$@.tmp
	chmod a-w $@.tmp
	mv $@.tmp $@

CLEANFILES = librinad.pc

pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = librinad.pc

#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#	      Dimitri Staessens     <dimitri DOT staessens AT ugent DOT be>
#

if BUILD_BINDINGS_C

librina_c_la_SOURCES   = 	\
	librina-c.cc

librina_c_la_LDFLAGS   =
librina_c_la_LIBADD = 			\
	$(top_builddir)/src/librina.la	\
	-ldl
librina_c_la_CPPFLAGS =			\
	$(CXX_SYSINCLUDES)		\
	-I$(top_srcdir)/include		\
	-I$(srcdir)/../include		\
	$(CPPFLAGS_EXTRA)
librina_c_la_CXXFLAGS =		\
	$(CXXFLAGS_EXTRA)

lib_LTLIBRARIES = librina-c.la

EXTRA_DIST =					\
	librina-c.pc.in

edit = $(SED)								\
        -e 's|@PACKAGE_VERSION[@]|$(PACKAGE_VERSION)|g'			\
        -e 's|@PACKAGE_URL[@]|$(PACKAGE_URL)|g'				\
        -e 's|@prefix[@]|$(prefix)|g'					\
        -e 's|@includedir[@]|$(includedir)|g'				\
        -e 's|@libdir[@]|$(libdir)|g'


librina-c.pc: Makefile librina-c.pc.in
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.in >$@.tmp
	chmod a-w $@.tmp
	mv $@.tmp $@

CLEANFILES = librina-c.pc

pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = librina-c.pc

endif

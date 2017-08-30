#
# Makefile.am

librina_api_la_SOURCES   = 	\
	rina-api.cc

librina_api_la_LDFLAGS   =
librina_api_la_LIBADD = 			\
	$(top_builddir)/src/librina.la	\
	-ldl
librina_api_la_CPPFLAGS =			\
	$(CXX_SYSINCLUDES)		\
	-I$(top_srcdir)/include		\
	-I$(srcdir)/../include		\
	-I$(top_srcdir)/src  \
	$(CPPFLAGS_EXTRA)
librina_api_la_CXXFLAGS =		\
	$(CXXFLAGS_EXTRA)

lib_LTLIBRARIES = librina-api.la

EXTRA_DIST =					\
	librina-api.pc.in

edit = $(SED)								\
        -e 's|@PACKAGE_VERSION[@]|$(PACKAGE_VERSION)|g'			\
        -e 's|@PACKAGE_URL[@]|$(PACKAGE_URL)|g'				\
        -e 's|@prefix[@]|$(prefix)|g'					\
        -e 's|@includedir[@]|$(includedir)|g'				\
        -e 's|@libdir[@]|$(libdir)|g'


librina-api.pc: Makefile librina-api.pc.in
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.in >$@.tmp
	chmod a-w $@.tmp
	mv $@.tmp $@

CLEANFILES = librina-api.pc

pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = librina-api.pc


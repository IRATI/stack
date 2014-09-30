#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

SUBDIRS      = include

EXTRA_DIST   =
CLEANFILES   =

bin_PROGRAMS = ipcm

noinst_LTLIBRARIES = libjsoncpp.la

libjsoncpp_la_SOURCES  =			\
	jsoncpp/json/autolink.h			\
	jsoncpp/json/config.h			\
	jsoncpp/json/features.h			\
	jsoncpp/json/forwards.h			\
	jsoncpp/json/json.h			\
	jsoncpp/json/reader.h			\
	jsoncpp/json/value.h			\
	jsoncpp/json/writer.h			\
	jsoncpp/json_batchallocator.h		\
	jsoncpp/json_internalarray.inl		\
	jsoncpp/json_internalmap.inl		\
	jsoncpp/json_reader.cpp			\
	jsoncpp/json_value.cpp			\
	jsoncpp/json_valueiterator.inl		\
	jsoncpp/json_writer.cpp

libjsoncpp_la_LDFLAGS  =
libjsoncpp_la_LIBADD   =
libjsoncpp_la_CPPFLAGS = -I$(srcdir)/jsoncpp

ipcm_CFLAGS   =
ipcm_CPPFLAGS =					\
	$(CPPFLAGS_EXTRA)			\
	$(LIBRINA_CFLAGS)			\
	-I$(srcdir)/..				\
	-I$(srcdir)/../common			\
	-I$(srcdir)/jsoncpp
ipcm_LDADD    =					\
	$(builddir)/../common/librinad.la	\
	$(builddir)/libjsoncpp.la		\
	$(LIBRINA_LIBS)
ipcm_SOURCES  =							\
	main.cc							\
	ipcm.cc                     ipcm.h			\
	configuration.cc            configuration.h		\
	registration.cc 	    registration.h		\
	flow-allocation.cc          flow-allocation.h		\
	console.cc		    console.h

test_empty_SOURCES  =				\
	test-empty.cc
test_empty_CPPFLAGS =				\
	$(CPPFLAGS_EXTRA)			\
	$(LIBRINA_CFLAGS)			\
	-I$(srcdir)/..				\
	-I$(srcdir)/../common
test_empty_LDADD    = $(builddir)/../common/librinad.la $(LIBRINA_LIBS)

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

bin_SCRIPTS = ipcm-plugins

ipcm-plugins: Makefile ipcm-plugins.in
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.in >$@.tmp
	chmod a-w $@.tmp
	chmod u+x $@.tmp
	mv $@.tmp $@

EXTRA_DIST += ipcm-plugins.in
CLEANFILES += ipcm-plugins

#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

CLEANFILES       =
MOSTLYCLEANFILES =
EXTRA_DIST       =

include $(srcdir)/Makefile.inc

#
#Libs
#
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

#
# librina
#
BUILT_SOURCES = $(protoSOURCES)

allsources =							\
        logs.cc							\
        utils.cc             utils.h				\
        concurrency.cc						\
        time.cc							\
        timer.cc						\
        core.cc              core.h				\
        exceptions.cc						\
        librina.cc						\
        common.cc						\
        ipc-api.cc                                              \
        application.cc						\
        internal-events.cc					\
        configuration.cc					\
        ipc-daemons.cc						\
        ipc-manager.cc						\
        ipc-process.cc						\
        faux-sockets.cc						\
        cdap.cc							\
        rib.cc							\
        irm.cc							\
        sdu-protection.cc					\
        netlink-messages.cc  netlink-messages.h			\
        netlink-parsers.cc   netlink-parsers.h			\
        netlink-manager.cc   netlink-manager.h			\
        syscalls-wrappers.cc rina-systypes.h    rina-syscalls.h	\
        $(protoSOURCES)						\
        cdap-impl.cc         cdap-impl.h			\
	plugin-info.cc	      plugin-info.h

librina_la_SOURCES  = $(allsources)
librina_la_LDFLAGS  =
librina_la_LIBADD   =			\
	$(LIBNL3_LIBS)				\
	$(LIBNLGENL3_LIBS)			\
	$(LIBPROTOBUF_LIBS)			\
	$(builddir)/libjsoncpp.la		\
	-ldl
librina_la_CPPFLAGS =				\
	$(CXX_SYSINCLUDES)			\
	-DSTUB_API=0				\
	-I$(top_srcdir)/include			\
	-I$(srcdir)/jsoncpp			\
	$(LIBNL3_CFLAGS)			\
	$(LIBNLGENL3_CFLAGS)			\
	$(LIBPROTOBUF_CFLAGS)			\
	$(CPPFLAGS_EXTRA)
librina_la_CXXFLAGS =				\
	$(CXXFLAGS_EXTRA)

librina_stubbed_la_SOURCES  = $(allsources)
librina_stubbed_la_LDFLAGS  =
librina_stubbed_la_LIBADD   =	\
	$(LIBNL3_LIBS)				\
	$(LIBNLGENL3_LIBS)			\
	$(LIBPROTOBUF_LIBS)			\
	$(builddir)/libjsoncpp.la		\
	-ldl
librina_stubbed_la_CPPFLAGS =			\
	$(CXX_SYSINCLUDES)			\
	-DSTUB_API=1				\
	-I$(top_srcdir)/include			\
	-I$(srcdir)/jsoncpp			\
	$(LIBNL3_CFLAGS)			\
	$(LIBNLGENL3_CFLAGS)			\
	$(LIBPROTOBUF_CFLAGS)			\
	$(CPPFLAGS_EXTRA)
librina_stubbed_la_CXXFLAGS =			\
	$(CXXFLAGS_EXTRA)

lib_LTLIBRARIES    = librina.la
noinst_LTLIBRARIES += librina-stubbed.la

noinst_PROGRAMS = test-linking

test_linking_SOURCES  = test-linking.cc
test_linking_CFLAGS   =
test_linking_CPPFLAGS =				\
	-DSTUB_API=0				\
	-I$(top_srcdir)/include			\
	$(LIBNL3_CFLAGS)			\
	$(LIBNLGENL3_CFLAGS)			\
	$(LIBPROTOBUF_CFLAGS)			\
	$(CPPFLAGS_EXTRA)
test_linking_CXXFLAGS =				\
	$(CXXFLAGS_EXTRA)
test_linking_LDADD    =			\
	$(LIBNL3_LIBS)				\
	$(LIBNLGENL3_LIBS)			\
	$(LIBPROTOBUF_LIBS)			\
	-ldl						\
	librina.la

check-local: test-linking

EXTRA_DIST +=					\
	librina.pc.in

edit = $(SED)								\
        -e 's|@PACKAGE_VERSION[@]|$(PACKAGE_VERSION)|g'			\
        -e 's|@PACKAGE_URL[@]|$(PACKAGE_URL)|g'				\
	-e 's|@LIBNL_MIN_VERSION[@]|$(LIBNL_MIN_VERSION)|g'		\
	-e 's|@LIBNL3_CFLAGS[@]|$(LIBNL3_CFLAGS)|g'			\
	-e 's|@LIBNL3_LIBS[@]|$(LIBNL3_LIBS)|g'				\
	-e 's|@LIBNLGENL3_CFLAGS[@]|$(LIBNLGENL3_CFLAGS)|g'		\
	-e 's|@LIBNLGENL3_LIBS[@]|$(LIBNLGENL3_LIBS)|g'			\
	-e 's|@LIBPROTOBUF_MIN_VERSION[@]|$(LIBPROTOBUF_MIN_VERSION)|g'	\
	-e 's|@LIBPROTOBUF_CFLAGS[@]|$(LIBPROTOBUF_CFLAGS)|g'		\
	-e 's|@LIBPROTOBUF_LIBS[@]|$(LIBPROTOBUF_LIBS)|g'		\
        -e 's|@prefix[@]|$(prefix)|g'					\
        -e 's|@includedir[@]|$(includedir)|g'				\
        -e 's|@libdir[@]|$(libdir)|g'

librina.pc: Makefile librina.pc.in
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.in >$@.tmp
	chmod a-w $@.tmp
	mv $@.tmp $@

CLEANFILES += librina.pc

pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = librina.pc

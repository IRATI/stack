#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

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
	$(LIBRINA_CFLAGS)			\
	-I$(srcdir)/..				\
	-I$(builddir)/../common			\
	-I$(srcdir)/jsoncpp
ipcm_LDADD    =					\
	$(builddir)/../common/librinad.la	\
	$(builddir)/libjsoncpp.la		\
	$(LIBRINA_LIBS)
ipcm_SOURCES  =					\
	main.cc event-loop.h event-loop.cc

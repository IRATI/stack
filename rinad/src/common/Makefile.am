#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

SUBDIRS = tclap encoders

#
#Libs
#
noinst_LTLIBRARIES = libjsoncpp.la librinad.la

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


librinad_la_CFLAGS   =
librinad_la_CPPFLAGS =				\
	$(CPPFLAGS_EXTRA)			\
	$(LIBRINA_CFLAGS)			\
	$(LIBPROTOBUF_CFLAGS)			\
	-I$(srcdir)/tclap			\
	-I$(srcdir)/jsoncpp			\
	-I$(srcdir)/..
librinad_la_LIBADD   =				\
	$(builddir)/libjsoncpp.la		\
	$(LIBRINA_LIBS)				\
	$(LIBPROTOBUF_LIBS)			\
	-Lencoders -lencoders

librinad_la_SOURCES  =					\
	debug.cc              debug.h                   \
	rina-configuration.cc rina-configuration.h	\
	event-loop.cc	      event-loop.h		\
	encoder.cc	      encoder.h			\
			      concurrency.h

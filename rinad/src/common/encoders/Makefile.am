#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

CLEANFILES       =
MOSTLYCLEANFILES =
EXTRA_DIST       =

include $(srcdir)/Makefile.inc

noinst_LTLIBRARIES =				\
	libencoders.la

BUILT_SOURCES = $(protoSOURCES)

libencoders_la_SOURCES =			\
	$(protoSOURCES)
libencoders_la_CPPFLAGS =			\
	-I$(builddir)/..			\
	-I$(srcdir)/..				\
	$(CPPFLAGS_EXTRA)

#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

bin_PROGRAMS = ipcp

ipcp_CFLAGS   =
ipcp_CPPFLAGS = $(CPPFLAGS_EXTRA) $(LIBRINA_CFLAGS) -I$(srcdir)/..
ipcp_LDADD    = $(builddir)/../common/librinad.la $(LIBRINA_LIBS)
ipcp_SOURCES  =					\
	main.cc

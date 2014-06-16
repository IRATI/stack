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
	main.cc         \
	events.cc      \
	components.cc   \
	rib-daemon.cc   \
	namespace-manager.cc \
	flow-allocator.cc   \
	enrollment-task.cc

#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

bin_PROGRAMS = ipcp

ipcp_CFLAGS   =
ipcp_CPPFLAGS =					\
	$(CPPFLAGS_EXTRA)			\
	$(LIBRINA_CFLAGS)			\
	-I$(srcdir)/..				\
	-I$(srcdir)/../common
ipcp_LDADD    = $(builddir)/../common/librinad.la $(LIBRINA_LIBS)
ipcp_SOURCES  =						\
	main.cc						\
	events.cc		events.h		\
	components.cc		components.h		\
	rib-daemon.cc		rib-daemon.h		\
	enrollment-task.cc	enrollment-task.h	\
	namespace-manager.cc	namespace-manager.h	\
	pduft-generator.cc      pduft-generator.h	\
	resource-allocator.cc	resource-allocator.h	\
	flow-allocator.cc       flow-allocator.h

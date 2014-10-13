#
# Makefile.am
#
# Written by: Vincenzo Maffione <v DOT maffione AT nextworks DOT it>
#

EXTRA_DIST   =
CLEANFILES   =

COMMONCPPFLAGS =				\
	$(CPPFLAGS_EXTRA)			\
	$(LIBRINA_CFLAGS)			\
	-I$(srcdir)/..				\
	-I$(srcdir)/../common

libps_sm_passwd_la_CFLAGS =
libps_sm_passwd_la_CPPFLAGS =			\
	$(COMMONCPPFLAGS)
libps_sm_passwd_la_LIBADD =
libps_sm_passwd_la_LDFLAGS =
libps_sm_passwd_la_SOURCES =			\
		security-manager-passwd.cc	\
		component.h

lib_LTLIBRARIES = libps-sm-passwd.la

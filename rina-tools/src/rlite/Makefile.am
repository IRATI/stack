#
# Makefile.am
#

bin_PROGRAMS                       =
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT =

rinaperf_SOURCES = rinaperf.c
rinaperf_LDADD = $(LIBRINA_API_LIBS)
rinaperf_CPPFLAGS = $(LIBRINA_API_CFLAGS)

rina_echo_SOURCES = rina-echo.c
rina_echo_LDADD = $(LIBRINA_API_LIBS)
rina_echo_CPPFLAGS = $(LIBRINA_API_CFLAGS)

bin_PROGRAMS += rinaperf rina-echo
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT += rinaperf rina-echo

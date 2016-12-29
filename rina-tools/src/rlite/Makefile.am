#
# Makefile.am
#

bin_PROGRAMS                       =
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT =

rinaperf_SOURCES = rinaperf.c
rinaperf_LDADD = $(LIBRINA_API_LIBS)
rinaperf_CPPFLAGS = $(LIBRINA_API_CFLAGS)

rina_echo_async_SOURCES = rina-echo-async.c
rina_echo_async_LDADD = $(LIBRINA_API_LIBS)
rina_echo_async_CPPFLAGS = $(LIBRINA_API_CFLAGS)

bin_PROGRAMS += rinaperf rina-echo-async
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT += rinaperf rina-echo-async

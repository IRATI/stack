#
# Makefile.am
#

bin_PROGRAMS                       =
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT =

rinaperf_SOURCES = rinaperf.c
rinaperf_LDADD = $(LIBRINA_API_LIBS)
rinaperf_CPPFLAGS = $(LIBRINA_API_CFLAGS)

rina_rr_tool_SOURCES = rina-rr-tool.c
rina_rr_tool_LDADD = $(LIBRINA_API_LIBS)
rina_rr_tool_CPPFLAGS = $(LIBRINA_API_CFLAGS)

bin_PROGRAMS += rinaperf rina-rr-tool
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT += rinaperf rina-rr-tool

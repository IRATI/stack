#
# Makefile.am
#

bin_PROGRAMS                       =
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT =

tg_server_SOURCES = tg-server.cpp
tg_server_LDADD = $(LIBRINA_API_LIBS)
tg_server_CPPFLAGS = $(LIBRINA_API_CFLAGS) -I$(srcdir)/../common -lpthread -std=c++11

tg_client_SOURCES = tg-client.cpp
tg_client_LDADD = $(LIBRINA_API_LIBS)
tg_client_CPPFLAGS = $(LIBRINA_API_CFLAGS) -I$(srcdir)/../common -lpthread -std=c++11

bin_PROGRAMS += tg-server tg-client
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT += tg-server tg-client

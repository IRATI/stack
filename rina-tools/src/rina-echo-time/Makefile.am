#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

bin_PROGRAMS                       =
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT =

rina_echo_time_SOURCES  =				\
	Application.cpp        Application.hpp		\
	ApplicationBuilder.cpp ApplicationBuilder.hpp	\
	Server.cpp             Server.hpp		\
	Client.cpp             Client.hpp		\
	Main.cpp
rina_echo_time_LDADD    = $(LIBRINA_LIBS)
rina_echo_time_CPPFLAGS = $(LIBRINA_CFLAGS) $(CPPFLAGS_EXTRA) -std=c++11
bin_PROGRAMS            += rina-echo-time
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT += rina-echo-time

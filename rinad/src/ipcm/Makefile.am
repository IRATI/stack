#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

bin_PROGRAMS = ipcm

ipcm_CFLAGS   =
ipcm_CPPFLAGS = $(LIBRINA_CFLAGS) -I$(srcdir)/..
ipcm_LDADD    = $(builddir)/../common/librinad.la $(LIBRINA_LIBS)
ipcm_SOURCES  =					\
	main.cc


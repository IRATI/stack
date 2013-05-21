#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

#SWIG_DEBUG      = -v
SWIG_DEBUG      =
SWIG_FLAGS      = $(SWIG_DEBUG) -Wall -nocpperraswarn -Werror
SWIG_JAVA_FLAGS = -java

librina.stamp: $(top_srcdir)/include/librina.i $(top_srcdir)/include/librina.h
	@rm -f librina.tmp
	@touch librina.tmp
	$(SWIG) $(SWIG_FLAGS) $(SWIG_JAVA_FLAGS)			  \
		-o librina_wrapper.c $(top_srcdir)/include/librina.i || { \
		rm -f librina.tmp ;					  \
		exit 1 ;						  \
	}
	@mv -f librina.tmp $@

CLEANFILES = librina_wrapper.c librina.stamp

librina_wrapper.c: librina.stamp

librina_wrapper_la_SOURCES  =                       \
        librina_wrapper.c
librina_wrapper_la_LDFLAGS  = -module
librina_wrapper_la_CPPFLAGS = -I$(top_srcdir)/include -I$(JNI_CPPFLAGS)

lib_LTLIBRARIES = librina_wrapper.la

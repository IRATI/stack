#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

SWIG_DEBUG      =
SWIG_FLAGS      = $(SWIG_DEBUG) -Wall -nocpperraswarn -Werror
SWIG_JAVA_FLAGS = -java -package eu.irati.librina

SWIG_DEBUG      += -v

librina.stamp: $(top_srcdir)/include/librina.i $(top_srcdir)/include/librina.h
	@rm -f librina.tmp
	@touch librina.tmp
	$(SWIG) $(SWIG_FLAGS) $(SWIG_JAVA_FLAGS)	\
		-o librina_wrapper.c			\
		$(top_srcdir)/include/librina.i || {	\
		rm -f librina.tmp ;			\
		exit 1 ;				\
	}
	$(MKDIR_P) $(builddir)/eu
	$(MKDIR_P) $(builddir)/eu/irati
	$(MKDIR_P) $(builddir)/eu/irati/librina
	@mv *.java eu/irati/librina
	@javac eu/irati/librina/*.java
	@jar -cvf librina.jar eu/irati/librina/*.class
	@mv -f librina.tmp $@

CLEANFILES = librina_wrapper.c librina.stamp *.java *.class *.jar

librina_wrapper.c: librina.stamp

librina_wrapper_la_SOURCES  =                       \
        librina_wrapper.c
librina_wrapper_la_LDFLAGS  = -module
librina_wrapper_la_CPPFLAGS = -I$(top_srcdir)/include -I$(JNI_CPPFLAGS)

lib_LTLIBRARIES = librina_wrapper.la

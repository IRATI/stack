#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

SWIG_DEBUG      =
SWIG_FLAGS      = $(SWIG_DEBUG) -Wall -nocpperraswarn -Werror
SWIG_JAVA_FLAGS = -java -package eu.irati.librina

SWIG_DEBUG      += -v

CLEANFILES =

# XXX FIXME: Prerequisites for this rule are broken ...
wrap.stamp: $(top_srcdir)/include/librina.i $(top_srcdir)/include/librina.h
	rm -f wrap.tmp
	touch wrap.tmp
	$(MKDIR_P) eu/irati/librina &&			\
	$(SWIG) $(SWIG_FLAGS) $(SWIG_JAVA_FLAGS)	\
		-o librina_java.c			\
		-outdir ./eu/irati/librina		\
		$(top_srcdir)/include/librina.i || {	\
		rm -f wrap.tmp ;			\
		exit 1 ;				\
	}
	mv -f wrap.tmp $@

CLEANFILES += wrap.stamp wrap.tmp
CLEANFILES += librina_java.c

all-local-classes: wrap.stamp
	$(JAVAC) eu/irati/librina/*.java

all-local-jar: all-local-classes
	$(JAR) -cvf librina.jar eu/irati/librina/*.class

CLEANFILES += librina.jar

javadir = $(datarootdir)/java

java_DATA = librina.jar

all-local: all-local-jar

clean-local:
	rm -r -f eu/irati/librina

librina_java.c: wrap.stamp

librina_java_la_SOURCES  =			\
        librina_java.c
librina_java_la_LDFLAGS  = -module
librina_java_la_CPPFLAGS = -I$(top_srcdir)/include -I$(JNI_CPPFLAGS)

lib_LTLIBRARIES = librina_java.la

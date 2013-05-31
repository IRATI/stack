#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

if BUILD_WRAPPERS_JAVA

SWIG_DEBUG      = -v
SWIG_FLAGS      = $(SWIG_DEBUG) -Werror -Wall
SWIG_JAVA_FLAGS = -java -package eu.irati.librina

CLEANFILES =

# XXX FIXME: Prerequisites for this rule are broken ...
wrap.stamp: $(top_srcdir)/include/librina++.i $(top_srcdir)/include/*.h $(top_srcdir)/include/*.h++
	rm -f wrap.tmp
	touch wrap.tmp
	rm -r -f eu/irati/librina   &&			\
	$(MKDIR_P) eu/irati/librina &&			\
	$(SWIG) $(SWIG_FLAGS) $(SWIG_JAVA_FLAGS)	\
		-c++					\
		-o librina_java.c++			\
		-outdir ./eu/irati/librina		\
		$(top_srcdir)/include/librina++.i || {	\
		echo "Cannot wrap input file" ;		\
		rm -f wrap.tmp ;			\
		rm -r -f eu/irati/librina ;		\
		exit 1 ;				\
	}
	mv -f wrap.tmp $@

CLEANFILES += wrap.stamp wrap.tmp
CLEANFILES += librina_java.c

librina-java-classes: wrap.stamp
	$(JAVAC) eu/irati/librina/*.java

librina.jar: librina-java-classes
	$(JAR) -cvf librina.jar eu/irati/librina/*.class

CLEANFILES += librina.jar

javadir = $(datarootdir)/java

java_DATA = librina.jar

clean-local:
	rm -r -f eu/irati/librina

librina_java.c++: wrap.stamp

librina_java_la_SOURCES  =			\
        librina_java.c++
librina_java_la_LDFLAGS  = -module
librina_java_la_LIBADD   = $(top_builddir)/src/librina.la
librina_java_la_CPPFLAGS = -I$(top_srcdir)/include -I$(JNI_CPPFLAGS)

lib_LTLIBRARIES = librina_java.la

endif

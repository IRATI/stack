#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

SWIG_DEBUG      =
SWIG_FLAGS      = $(SWIG_DEBUG) -Wall -nocpperraswarn -Werror
SWIG_JAVA_FLAGS = -java

SWIG_DEBUG      += -v

CLEANFILES =

wrap.stamp: $(top_srcdir)/include/librina.i $(top_srcdir)/include/librina.h
	@rm -f librina.tmp
	@touch librina.tmp
	$(MKDIR_P) $(builddir)/java
	$(SWIG) $(SWIG_FLAGS) $(SWIG_JAVA_FLAGS)	\
		-o librina_java.c			\
		$(top_srcdir)/include/librina.i || {	\
		rm -f librina.tmp ;			\
		exit 1 ;				\
	}
	@mv -f librina.tmp $@

CLEANFILES += wrap.stamp librina_java.c *.java

librina_java.c: wrap.stamp
CLEANFILES += librina_java.c

librina_java_la_SOURCES  =			\
        librina_java.c
librina_java_la_LDFLAGS  = -module
librina_java_la_CPPFLAGS = -I$(top_srcdir)/include -I$(JNI_CPPFLAGS)

lib_LTLIBRARIES = librina_java.la

javadir = $(datadir)/java
#nodist_java_JAVA = *.java

# To be added:
#   To compile: javac *.java
#   To jar:     jar cvf librina.jar *.class

CLEANFILES += *.class

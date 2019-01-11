#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

JAVA_HOME_DIR   = /usr/lib/jvm/default-java
JNI_CPPFLAGS    = -I$(JAVA_HOME_DIR)/include -I$(JAVA_HOME_DIR)/include/linux
JAVA_DIR	= $(top_builddir)/../java
JAVAC           = javac
JAR             = jar
SWIG            = swig
SWIG_DEBUG      = -v
SWIG_CPPFLAGS   = -I$(top_srcdir)/wrap/rina-api/include
SWIG_FLAGS      = $(SWIG_DEBUG) $(SWIG_CPPFLAGS) -Werror -Wall
SWIG_JAVA_FLAGS = -java -package eu.irati.rina.api

CLEANFILES =

# XXX FIXME: Prerequisites for this rule are broken ...
wrap.stamp: $(top_srcdir)/wrap/rina-api/rinapi.i $(top_srcdir)/wrap/rina-api/include/rina/api.h
	rm -f wrap.tmp
	touch wrap.tmp
	rm -r -f $(JAVA_DIR)/eu/irati/rina/api   &&			\
	$(MKDIR_P) $(JAVA_DIR)/eu/irati/rina/api &&			\
	$(SWIG) $(SWIG_FLAGS) $(SWIG_JAVA_FLAGS)	\
		-o rinapi_java.c			\
		-outdir $(JAVA_DIR)/eu/irati/rina/api		\
		$(top_srcdir)/wrap/rina-api/rinapi.i || {	\
		echo "Cannot wrap input file" ;		\
		rm -f wrap.tmp ;			\
		rm -r -f $(JAVA_DIR)/eu/irati/rina/api ;		\
		exit 1 ;				\
	}
	mv -f wrap.tmp $@

CLEANFILES += wrap.stamp wrap.tmp
CLEANFILES += rinapi_java.c+

rinapi-java-classes: wrap.stamp
	$(JAVAC) $(JAVA_DIR)/eu/irati/rina/api/*.java

rinapi.jar: rinapi-java-classes
	cd $(JAVA_DIR) &&	\
	$(JAR) -cvf rinapi.jar eu/irati/rina/api/*.class
	mv $(JAVA_DIR)/rinapi.jar $(builddir)
CLEANFILES += rinapi.jar

pkgdata_DATA = rinapi.jar

clean-local:
	rm -r -f $(JAVA_DIR)/eu/irati/rina/api

rinapi_java.c: wrap.stamp

rinapi_java_la_SOURCES  = rinapi_java.c
rinapi_java_la_LDFLAGS  = -module
rinapi_java_la_LIBADD   = $(top_builddir)/wrap/rina-api/src/librina-api.la
rinapi_java_la_CPPFLAGS = -I$(top_srcdir)/wrap/rina-api/include $(JNI_CPPFLAGS)

lib_LTLIBRARIES = rinapi_java.la

if BUILD_MAVEN_SUPPORT
install-data-local-maven: rinapi.jar
	$(MVN) install:install-file					\
		-Dfile=rinapi.jar					\
		-DgroupId=eu.irati					\
		-DartifactId=eu.irati.rina.api				\
		-Dversion=$(PACKAGE_VERSION)				\
		-Dpackaging=jar || {					\
		echo "Cannot install rinapi jar into maven ..." ;	\
		exit 1 ;						\
	}
else
install-data-local-maven:
endif

install-data-local: install-data-local-maven

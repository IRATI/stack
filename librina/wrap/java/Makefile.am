#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

if BUILD_BINDINGS_JAVA

JAVA_DIR		= $(top_builddir)/../java
SWIG_DEBUG      = -v
SWIG_CPPFLAGS   = -I$(top_srcdir)/include
SWIG_FLAGS      = $(SWIG_DEBUG) $(SWIG_CPPFLAGS) -Werror -Wall
SWIG_JAVA_FLAGS = -java -package eu.irati.librina

CLEANFILES =

# XXX FIXME: Prerequisites for this rule are broken ...
wrap.stamp: $(top_srcdir)/wrap/*.i $(top_srcdir)/include/librina/*.h
	rm -f wrap.tmp
	touch wrap.tmp
	rm -r -f $(JAVA_DIR)/eu/irati/librina   &&			\
	$(MKDIR_P) $(JAVA_DIR)/eu/irati/librina &&			\
	$(SWIG) $(SWIG_FLAGS) $(SWIG_JAVA_FLAGS)	\
		-c++					\
		-o librina_java.cc			\
		-outdir $(JAVA_DIR)/eu/irati/librina		\
		$(top_srcdir)/wrap/librina.i || {	\
		echo "Cannot wrap input file" ;		\
		rm -f wrap.tmp ;			\
		rm -r -f $(JAVA_DIR)/eu/irati/librina ;		\
		exit 1 ;				\
	}
	mv -f wrap.tmp $@

CLEANFILES += wrap.stamp wrap.tmp
CLEANFILES += librina_java.cc+

librina-java-classes: wrap.stamp
	$(JAVAC) $(JAVA_DIR)/eu/irati/librina/*.java

librina.jar: librina-java-classes
	cd $(JAVA_DIR) &&	\
	$(JAR) -cvf librina.jar eu/irati/librina/*.class
	mv $(JAVA_DIR)/librina.jar $(builddir)
CLEANFILES += librina.jar

pkgdata_DATA = librina.jar

clean-local:
	rm -r -f $(JAVA_DIR)/eu/irati/librina

librina_java.cc: wrap.stamp

librina_java_la_SOURCES  = librina_java.cc
librina_java_la_LDFLAGS  = -module
librina_java_la_LIBADD   = $(top_builddir)/src/librina.la
librina_java_la_CPPFLAGS = -I$(top_srcdir)/include $(JNI_CPPFLAGS)

lib_LTLIBRARIES = librina_java.la

if BUILD_MAVEN_SUPPORT
install-data-local-maven: librina.jar
	$(MVN) install:install-file					\
		-Dfile=librina.jar					\
		-DgroupId=eu.irati					\
		-DartifactId=eu.irati.librina				\
		-Dversion=$(PACKAGE_VERSION)				\
		-Dpackaging=jar || {					\
		echo "Cannot install librina jar into maven ..." ;	\
		exit 1 ;						\
	}
else
install-data-local-maven:
endif

install-data-local: install-data-local-maven

endif

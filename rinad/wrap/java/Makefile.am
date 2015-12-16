#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

if BUILD_BINDINGS_JAVA

JAVA_DIR		= $(top_builddir)/../java
SWIG_DEBUG      = -v
SWIG_CPPFLAGS   = -I$(top_srcdir)/src/common -I$(top_srcdir)/../librina/include
SWIG_FLAGS      = $(SWIG_DEBUG) $(SWIG_CPPFLAGS) -Wextra -Werror
SWIG_JAVA_FLAGS = -java -package eu.irati.librinad

CLEANFILES =

# XXX FIXME: Prerequisites for this rule are broken ...
wrap.stamp: $(top_srcdir)/wrap/*.i $(top_srcdir)/src/common/*.h
	rm -f wrap.tmp
	touch wrap.tmp
	rm -r -f $(JAVA_DIR)/eu/irati/librinad   &&			\
	$(MKDIR_P) $(JAVA_DIR)/eu/irati/librinad &&			\
	$(SWIG) $(SWIG_FLAGS) $(SWIG_JAVA_FLAGS)	\
		-c++					\
		-o librinad_java.cc			\
		-outdir $(JAVA_DIR)/eu/irati/librinad		\
		$(top_srcdir)/wrap/librinad.i || {	\
		echo "Cannot wrap input file" ;		\
		rm -f wrap.tmp ;			\
		rm -r -f $(JAVA_DIR)/eu/irati/librinad ;		\
		exit 1 ;				\
	}
	mv -f wrap.tmp $@

CLEANFILES += wrap.stamp wrap.tmp
CLEANFILES += librinad_java.cc

librinad-java-classes: wrap.stamp
	$(JAVAC) -cp $(JAVA_DIR) $(JAVA_DIR)/eu/irati/librinad/*.java

librinad.jar: librinad-java-classes
	cd $(JAVA_DIR) &&	\
	$(JAR) -cvf librinad.jar eu/irati/librinad/*.class
	mv $(JAVA_DIR)/librinad.jar $(builddir)

CLEANFILES += librinad.jar

pkgdata_DATA = librinad.jar

clean-local:
	rm -r -f $(JAVA_DIR)/eu/irati/librinad

librinad_java.cc: wrap.stamp

librinad_java_la_SOURCES  = librinad_java.cc
librinad_java_la_LDFLAGS  = -module
librinad_java_la_LIBADD   = $(top_builddir)/src/common/librinad.la
librinad_java_la_CPPFLAGS = -I$(top_srcdir)/src/common $(JNI_CPPFLAGS) \
							$(LIBRINA_CFLAGS)
lib_LTLIBRARIES = librinad_java.la

install-data-local-maven: librinad.jar
	$(MVN) install:install-file					\
		-Dfile=librinad.jar					\
		-DgroupId=eu.irati					\
		-DartifactId=eu.irati.librinad				\
		-Dversion=$(PACKAGE_VERSION)				\
		-Dpackaging=jar || {					\
		echo "Cannot install librinad jar into maven ..." ;	\
		exit 1 ;						\
	}

install-data-local: install-data-local-maven

endif
#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

EXTRA_DIST  =
CLEANFILES  =
bin_SCRIPTS =

javadir     = $(datarootdir)/java
java_DATA   =

if BUILD_WRAPPERS_JAVA

edit = $(SED)							\
        -e 's|@abs_top_builddir[@]|$(abs_top_builddir)|g'	\
        -e 's|@datarootdir[@]|$(datarootdir)|g'			\
        -e 's|@javadir[@]|$(javadir)|g'				\
        -e 's|@builddir[@]|$(builddir)|g'			\
        -e 's|@libdir[@]|$(libdir)|g'				\
        -e 's|@SHELL[@]|$(SHELL)|g'				\
        -e 's|@JAVA[@]|$(JAVA)|g'				\
        -e 's|@JAVAC[@]|$(JAVAC)|g'

#
# Rules for ipcpd
#

ipcpd: Makefile ipcpd.in
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.in >$@.tmp
	chmod +x $@.tmp
	chmod a-w $@.tmp
	mv $@.tmp $@

bin_SCRIPTS += ipcpd
CLEANFILES  += ipcpd

EXTRA_DIST  += ipcpd.in ipcpd.java ipcpd-manifest.txt

ipcpd.class: ipcpd.java
	$(JAVAC) -classpath $(builddir):$(top_builddir)/wrap/java	\
		 -d $(builddir)						\
		$(srcdir)/ipcpd.java || {				\
		echo "Cannot compile ipcpd" ;				\
		exit 1 ;						\
	}
CLEANFILES += ipcpd.class

ipcpd.jar: ipcpd.class
	$(JAR) -cvfm ipcpd.jar ipcpd-manifest.txt ipcpd.class || {	\
		echo "Cannot build the ipcpd jar" ;	\
		exit 1 ;				\
	}
CLEANFILES += ipcpd.jar

java_DATA += ipcpd.jar

#
# Rules for ipcmd
#

ipcmd: Makefile ipcmd.in
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.in >$@.tmp
	chmod +x $@.tmp
	chmod a-w $@.tmp
	mv $@.tmp $@

bin_SCRIPTS += ipcmd
CLEANFILES  += ipcmd

EXTRA_DIST  += ipcmd.in ipcmd.java ipcmd-manifest.txt

ipcmd.class: ipcmd.java
	$(JAVAC) -classpath $(builddir):$(top_builddir)/wrap/java	\
		 -d $(builddir)						\
		$(srcdir)/ipcmd.java || {				\
		echo "Cannot compile ipcmd" ;				\
		exit 1 ;						\
	}
CLEANFILES += ipcmd.class

ipcmd.jar: ipcmd.class
	$(JAR) -cvfm ipcmd.jar ipcmd-manifest.txt ipcmd.class || {	\
		echo "Cannot build the ipcmd jar" ;	\
		exit 1 ;				\
	}
CLEANFILES += ipcmd.jar

java_DATA += ipcmd.jar

#
# Rules for app
#

app: Makefile app.in
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.in >$@.tmp
	chmod +x $@.tmp
	chmod a-w $@.tmp
	mv $@.tmp $@

bin_SCRIPTS += app
CLEANFILES  += app

EXTRA_DIST  += app.in app.java app-manifest.txt

app.class: app.java
	$(JAVAC) -classpath $(builddir):$(top_builddir)/wrap/java	\
		 -d $(builddir)						\
		$(srcdir)/app.java || {					\
		echo "Cannot compile app" ;				\
		exit 1 ;						\
	}
CLEANFILES += app.class

app.jar: app.class
	$(JAR) -cvfm app.jar app-manifest.txt app.class || {		\
		echo "Cannot build the app jar" ;	\
		exit 1 ;				\
	}
CLEANFILES += app.jar

java_DATA += app.jar

installcheck-local:
	$(bindir)/ipcpd --help    &&		\
	$(bindir)/ipcpd --version &&		\
	$(bindir)/ipcpd --help    &&		\
	$(bindir)/ipcpd --version &&		\
	$(bindir)/app   --help    &&		\
	$(bindir)/app   --version

endif

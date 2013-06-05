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

EXTRA_DIST  += ipcpd.in
EXTRA_DIST  += ipcpd.java

CLEANFILES  += ipcpd

ipcpd.class: ipcpd.java
	$(JAVAC) -classpath $(builddir):$(top_builddir)/wrap/java	\
		 -d $(builddir)						\
		$(srcdir)/ipcpd.java || {				\
		echo "Cannot compile ipcpd" ;				\
		exit 1 ;						\
	}
CLEANFILES += ipcpd.class

ipcpd.jar: ipcpd.class
	$(JAR) -cvfe ipcpd.jar ipcpd ipcpd.class || {	\
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

EXTRA_DIST  += ipcmd.in
EXTRA_DIST  += ipcmd.java

CLEANFILES  += ipcmd

ipcmd.class: ipcmd.java
	$(JAVAC) -classpath $(builddir):$(top_builddir)/wrap/java	\
		 -d $(builddir)						\
		$(srcdir)/ipcmd.java || {				\
		echo "Cannot compile ipcmd" ;				\
		exit 1 ;						\
	}
CLEANFILES += ipcmd.class

ipcmd.jar: ipcmd.class
	$(JAR) -cvfe ipcmd.jar ipcmd ipcmd.class || {	\
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

EXTRA_DIST  += app.in
EXTRA_DIST  += app.java

CLEANFILES  += app

app.class: app.java
	$(JAVAC) -classpath $(builddir):$(top_builddir)/wrap/java	\
		 -d $(builddir)						\
		$(srcdir)/app.java || {					\
		echo "Cannot compile app" ;				\
		exit 1 ;						\
	}
CLEANFILES += app.class

app.jar: app.class
	$(JAR) -cvfe app.jar app app.class || {		\
		echo "Cannot build the app jar" ;	\
		exit 1 ;				\
	}
CLEANFILES += app.jar

java_DATA += app.jar

endif

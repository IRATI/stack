#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

edit = $(SED)						\
        -e 's|@PACKAGE_VERSION[@]|$(PACKAGE_VERSION)|g'	\
        -e 's|@PACKAGE_NAME[@]|$(PACKAGE_NAME)|g'	\
        -e 's|@PACKAGE_URL[@]|$(PACKAGE_URL)|g'		\
        -e 's|@JAVA[@]|$(JAVA)|g'			\
        -e 's|@pkgdatadir[@]|$(pkgdatadir)|g'		\
        -e 's|@libdir[@]|$(libdir)|g'			\
        -e 's|@bindir[@]|$(bindir)|g'			\
        -e 's|@sbindir[@]|$(bindir)|g'			\
	-e 's|@prefix[@]|$(prefix)|g'			\
        -e 's|@LIBRINA_VERSION[@]|$(LIBRINA_VERSION)|g'

CLEANFILES    =
BUILT_SOURCES =
EXTRA_DIST    =
bin_SCRIPTS   =

ipcmanager/src/main/scripts/ipcmanager: \
	ipcmanager/src/main/scripts/ipcmanager.in \
	Makefile
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.in >$@.tmp
	chmod a-w $@.tmp
	chmod a+x $@.tmp
	mv $@.tmp $@


CLEANFILES    += ipcmanager/src/main/scripts/ipcmanager
BUILT_SOURCES += ipcmanager/src/main/scripts/ipcmanager
EXTRA_DIST    += ipcmanager/src/main/scripts/ipcmanager.in
bin_SCRIPTS   += ipcmanager/src/main/scripts/ipcmanager

ipcmanager/src/main/conf/ipcmanager.conf: \
	ipcmanager/src/main/conf/ipcmanager.conf.in \
	Makefile
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.in >$@.tmp
	chmod a-w $@.tmp
	mv $@.tmp $@

CLEANFILES    += ipcmanager/src/main/conf/ipcmanager.conf
BUILT_SOURCES += ipcmanager/src/main/conf/ipcmanager.conf
EXTRA_DIST    += ipcmanager/src/main/conf/ipcmanager.conf.in

ipcprocess/src/main/scripts/ipcprocess: \
	ipcprocess/src/main/scripts/ipcprocess.in \
	Makefile
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.in >$@.tmp
	chmod a-w $@.tmp
	chmod a+x $@.tmp
	mv $@.tmp $@

CLEANFILES    += ipcprocess/src/main/scripts/ipcprocess
BUILT_SOURCES += ipcprocess/src/main/scripts/ipcprocess
EXTRA_DIST    += ipcprocess/src/main/scripts/ipcprocess.in
bin_SCRIPTS   += ipcprocess/src/main/scripts/ipcprocess

applications/rina.utils.apps.rinaband/src/main/scripts/rinaband-server: \
	applications/rina.utils.apps.rinaband/src/main/scripts/rinaband-server.in \
	Makefile
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.in >$@.tmp
	chmod a-w $@.tmp
	chmod a+x $@.tmp
	mv $@.tmp $@

CLEANFILES    += applications/rina.utils.apps.rinaband/src/main/scripts/rinaband-server
BUILT_SOURCES += applications/rina.utils.apps.rinaband/src/main/scripts/rinaband-server
EXTRA_DIST    += applications/rina.utils.apps.rinaband/src/main/scripts/rinaband-server.in
bin_SCRIPTS   += applications/rina.utils.apps.rinaband/src/main/scripts/rinaband-server

applications/rina.utils.apps.rinaband/src/main/scripts/rinaband-client: \
	applications/rina.utils.apps.rinaband/src/main/scripts/rinaband-client.in \
	Makefile
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.in >$@.tmp
	chmod a-w $@.tmp
	chmod a+x $@.tmp
	mv $@.tmp $@

CLEANFILES    += applications/rina.utils.apps.rinaband/src/main/scripts/rinaband-client
BUILT_SOURCES += applications/rina.utils.apps.rinaband/src/main/scripts/rinaband-client
EXTRA_DIST    += applications/rina.utils.apps.rinaband/src/main/scripts/rinaband-client.in
bin_SCRIPTS   += applications/rina.utils.apps.rinaband/src/main/scripts/rinaband-client

applications/rina.utils.apps.echo/src/main/scripts/echo-server: \
	applications/rina.utils.apps.echo/src/main/scripts/echo-server.in \
	Makefile
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.in >$@.tmp
	chmod a-w $@.tmp
	chmod a+x $@.tmp
	mv $@.tmp $@

CLEANFILES    += applications/rina.utils.apps.echo/src/main/scripts/echo-server
BUILT_SOURCES += applications/rina.utils.apps.echo/src/main/scripts/echo-server
EXTRA_DIST    += applications/rina.utils.apps.echo/src/main/scripts/echo-server.in
bin_SCRIPTS   += applications/rina.utils.apps.echo/src/main/scripts/echo-server

applications/rina.utils.apps.echo/src/main/scripts/echo-client: \
	applications/rina.utils.apps.echo/src/main/scripts/echo-client.in \
	Makefile
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.in >$@.tmp
	chmod a-w $@.tmp
	chmod a+x $@.tmp
	mv $@.tmp $@

CLEANFILES    += applications/rina.utils.apps.echo/src/main/scripts/echo-client
BUILT_SOURCES += applications/rina.utils.apps.echo/src/main/scripts/echo-client
EXTRA_DIST    += applications/rina.utils.apps.echo/src/main/scripts/echo-client.in
bin_SCRIPTS   += applications/rina.utils.apps.echo/src/main/scripts/echo-client

pom.xml: pom.xml.in \
	Makefile
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.in >$@.tmp
	chmod a-w $@.tmp
	mv $@.tmp $@

CLEANFILES    += pom.xml
BUILT_SOURCES += pom.xml
EXTRA_DIST    += pom.xml.in

all-local: pom.xml Makefile.am
#	$(MVN) package

install-data-local:
	$(MVN) install

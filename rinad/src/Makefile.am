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
        -e 's|@LIBRINA_VERSION[@]|$(LIBRINA_VERSION)|g'

CLEANFILES    =
BUILT_SOURCES =
EXTRA_DIST    =
bin_SCRIPTS   =

ipcmanager/src/main/scripts/ipcmanager: \
	ipcmanager/src/main/scripts/ipcmanager.in
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

ipcprocess/src/main/scripts/ipcprocess: \
	ipcprocess/src/main/scripts/ipcprocess.in
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

pom.xml: Makefile pom.xml.in
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

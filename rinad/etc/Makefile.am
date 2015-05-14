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
        -e 's|@sysconfdir[@]|$(sysconfdir)|g'		\
        -e 's|@localstatedir[@]|$(localstatedir)|g'	\
        -e 's|@libdir[@]|$(libdir)|g'			\
        -e 's|@bindir[@]|$(bindir)|g'			\
        -e 's|@sbindir[@]|$(bindir)|g'			\
	-e 's|@prefix[@]|$(prefix)|g'			\
	-e 's|@localstatedir[@]|$(localstatedir)|g'	\
        -e 's|@LIBRINA_VERSION[@]|$(LIBRINA_VERSION)|g'

CLEANFILES    =
BUILT_SOURCES =
EXTRA_DIST    =
sysconf_DATA  =

ipcmanager.conf: ipcmanager.conf.in \
	Makefile
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.in >$@.tmp
	chmod a-w $@.tmp
	mv $@.tmp $@

CLEANFILES    += ipcmanager.conf
BUILT_SOURCES += ipcmanager.conf
EXTRA_DIST    += ipcmanager.conf.in
sysconf_DATA  += ipcmanager.conf

CLEANFILES    += default.dif
BUILT_SOURCES += default.dif
EXTRA_DIST    += default.dif
sysconf_DATA  += default.dif

CLEANFILES    += vlan100.dif
BUILT_SOURCES += vlan100.dif
EXTRA_DIST    += vlan100.dif
sysconf_DATA  += vlan100.dif

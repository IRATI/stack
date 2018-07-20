#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

# --runstatedir is new to Autoconf 2.70
runstatedir ?= $(localstatedir)/run

edit = $(SED)						\
        -e 's|@PACKAGE_VERSION[@]|$(PACKAGE_VERSION)|g'	\
        -e 's|@PACKAGE_NAME[@]|$(PACKAGE_NAME)|g'	\
        -e 's|@PACKAGE_URL[@]|$(PACKAGE_URL)|g'		\
        -e 's|@JAVA[@]|$(JAVA)|g'			\
        -e 's|@pkgdatadir[@]|$(pkgdatadir)|g'		\
        -e 's|@sysconfdir[@]|$(sysconfdir)|g'		\
        -e 's|@localstatedir[@]|$(localstatedir)|g'	\
        -e 's|@runstatedir[@]|$(runstatedir)|g'		\
        -e 's|@libdir[@]|$(libdir)|g'			\
        -e 's|@bindir[@]|$(bindir)|g'			\
        -e 's|@sbindir[@]|$(bindir)|g'			\
	-e 's|@prefix[@]|$(prefix)|g'			\
        -e 's|@LIBRINA_VERSION[@]|$(LIBRINA_VERSION)|g'

CLEANFILES    =
BUILT_SOURCES =
EXTRA_DIST    =
sysconf_DATA  =

ipcmanager.conf.orig: ipcmanager.conf.orig.in \
	Makefile
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.in >$@.tmp
	chmod a-w $@.tmp
	mv $@.tmp $@

wpa_supplicant.conf.orig: wpa_supplicant.conf.orig.in \
	Makefile
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.in >$@.tmp
	chmod a-w $@.tmp
	mv $@.tmp $@

CLEANFILES    += ipcmanager.conf.orig wpa_supplicant.conf.orig
BUILT_SOURCES += ipcmanager.conf.orig wpa_supplicant.conf.orig
EXTRA_DIST    += ipcmanager.conf.orig.in default.dif.orig shim-eth-vlan.dif.orig da.map.orig wpa_supplicant.conf.orig.in
sysconf_DATA  += ipcmanager.conf.orig default.dif.orig shim-eth-vlan.dif.orig da.map.orig wpa_supplicant.conf.orig

install-data-local: install-sysconfDATA
       @FILES="ipcmanager.conf default.dif shim-eth-vlan.dif da.map wpa_supplicant.conf";\
       for FILE in $$FILES ;\
       do \  
               if test -f $(DESTDIR)$(sysconfdir)/$$FILE ; then \
                       echo "$@ will not overwrite existing $(DESTDIR)$(sysconfdir)/$$FILE" ; \
               else \
                       echo "$(INSTALL_DATA) $(DESTDIR)$(sysconfdir)/$$FILE.orig $(DESTDIR)$(sysconfdir)/$$FILE"; \
                       $(INSTALL_DATA) $(DESTDIR)$(sysconfdir)/$$FILE.orig $(DESTDIR)$(sysconfdir)/$$FILE; \
               fi ;\
       done

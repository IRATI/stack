#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

EXTRA_DIST =

EXTRA_DIST += pom.xml.in

edit = $(SED)						\
        -e 's|@PACKAGE_VERSION[@]|$(PACKAGE_VERSION)|g' \
        -e 's|@PACKAGE_NAME[@]|$(PACKAGE_NAME)|g'       \
        -e 's|@PACKAGE_URL[@]|$(PACKAGE_URL)|g'         \
        -e 's|@LIBRINA_VERSION[@]|$(LIBRINA_VERSION)|g'


pom.xml: Makefile pom.xml.in
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.in >$@.tmp
	chmod a-w $@.tmp
	mv $@.tmp $@

CLEANFILES = pom.xml

BUILT_SOURCES = pom.xml

all-local: pom.xml Makefile.am
#	$(MVN) package

install-data-local:
	$(MVN) install

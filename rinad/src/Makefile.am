#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

SUBDIRS = common ipcp ipcm

edit = $(SED)							\
	-e 's|@SHELL[@]|$(SHELL)|g'				\
        -e 's|@PACKAGE_VERSION[@]|$(PACKAGE_VERSION)|g'		\
        -e 's|@PACKAGE_NAME[@]|$(PACKAGE_NAME)|g'		\
        -e 's|@PACKAGE_URL[@]|$(PACKAGE_URL)|g'			\
        -e 's|@PACKAGE_BUGREPORT[@]|$(PACKAGE_BUGREPORT)|g'

rinaperf-client-loop: \
	rinaperf-client-loop.in \
	Makefile
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.in >$@.tmp
	chmod a-w $@.tmp
	chmod a+x $@.tmp
	mv $@.tmp $@

CLEANFILES    += rinaperf-client-loop
BUILT_SOURCES += rinaperf-client-loop
EXTRA_DIST    += rinaperf-client-loop.in
bin_SCRIPTS   += rinaperf-client-loop

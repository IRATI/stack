#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

EXTRA_DIST  =
CLEANFILES  =
bin_SCRIPTS =

if BUILD_WRAPPERS_JAVA

edit = $(SED)							\
        -e 's|@abs_top_builddir[@]|$(abs_top_builddir)|g'	\
        -e 's|@builddir[@]|$(builddir)|g'			\
        -e 's|@libdir[@]|$(libdir)|g'				\
        -e 's|@SHELL[@]|$(SHELL)|g'				\
        -e 's|@JAVA[@]|$(JAVA)|g'				\
        -e 's|@JAVAC[@]|$(JAVAC)|g'

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

endif

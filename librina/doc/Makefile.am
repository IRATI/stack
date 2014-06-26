#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

include Makefile.inc

EXTRA_DIST       =
BUILT_SOURCES    =
MOSTLYCLEANFILES =

## FIXME: Remove this workaround
##EXTRA_DIST += *.md

EXTRA_DIST += doxygen.cfg.in

if DX_COND_doc

MOSTLYCLEANFILES += $(DX_CLEANFILES)

edit = $(SED)							\
        -e 's|@abs_top_srcdir[@]|$(abs_top_srcdir)|g'		\
        -e 's|@abs_top_builddir[@]|$(abs_top_builddir)|g'

BUILT_SOURCES    += doxygen.cfg
MOSTLYCLEANFILES += doxygen.cfg

doxygen.cfg: Makefile doxygen.cfg.in
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.in >$@.tmp
	chmod a-w $@.tmp
	mv $@.tmp $@

pdf-local: doxygen.cfg doxygen-run doxygen-pdf

html-local: doxygen.cfg doxygen-run

endif

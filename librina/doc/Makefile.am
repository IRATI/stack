#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

include Makefile.inc

EXTRA_DIST	 =
BUILT_SOURCES	 =
MOSTLYCLEANFILES = $(DX_CLEANFILES)

## FIXME: Remove this workaround
##EXTRA_DIST += *.md

EXTRA_DIST += doxygen.cfg.in

if DX_COND_doc

MOSTLYCLEANFILES += $(DX_CLEANFILES)

edit = $(SED)							\
	-e 's|@abs_top_srcdir[@]|$(abs_top_srcdir)|g'		\
	-e 's|@abs_top_builddir[@]|$(abs_top_builddir)|g'	\
        -e 's|@PACKAGE_VERSION[@]|$(PACKAGE_VERSION)|g'		\
        -e 's|@PACKAGE_NAME[@]|$(PACKAGE_NAME)|g'

BUILT_SOURCES	 += doxygen.cfg
MOSTLYCLEANFILES += doxygen.cfg

doxygen.cfg: Makefile doxygen.cfg.in
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.in >$@.tmp
	chmod a-w $@.tmp
	mv $@.tmp $@

if DX_COND_pdf
pdf-local: doxygen.cfg doxygen-run doxygen-pdf
endif DX_COND_pdf

if DX_COND_html
html-local: doxygen.cfg doxygen-run
endif DX_COND_html

endif

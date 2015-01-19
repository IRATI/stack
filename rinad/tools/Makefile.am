#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

SUBDIRS                            =
EXTRA_DIST                         =
DISTCLEANFILES                     =
bin_PROGRAMS                       =
bin_SCRIPTS                        =
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT =

edit = $(SED)							\
        -e 's|@PACKAGE_VERSION[@]|$(PACKAGE_VERSION)|g'		\
        -e 's|@PACKAGE_NAME[@]|$(PACKAGE_NAME)|g'		\
        -e 's|@PACKAGE_URL[@]|$(PACKAGE_URL)|g'			\
        -e 's|@PACKAGE_BUGREPORT[@]|$(PACKAGE_BUGREPORT)|g'	\
        -e 's|@PERL[@]|$(PERL)|g'

if HAVE_PERL
backtrace-demangler: backtrace-demangler.in Makefile
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.in >$@.tmp
	chmod a-w $@.tmp
	chmod a+x $@.tmp
	mv $@.tmp $@

bin_SCRIPTS    += backtrace-demangler
DISTCLEANFILES += backtrace-demangler
EXTRA_DIST     += backtrace-demangler.in
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT += backtrace-demangler
endif

#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

SUBDIRS                            = common rina-echo-time rina-cdap-echo manager cdapc mac2ifname rlite
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
        -e 's|@PYTHON[@]|$(PYTHON)|g'				\
        -e 's|@SHELL[@]|$(SHELL)|g'

if HAVE_PYTHON
rina-mem-stats: rina-mem-stats.py Makefile
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.py >$@.tmp
	chmod a-w $@.tmp
	chmod a+x $@.tmp
	mv $@.tmp $@

bin_SCRIPTS    += rina-mem-stats
DISTCLEANFILES += rina-mem-stats
EXTRA_DIST     += rina-mem-stats.py
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT += rina-mem-stats

if HAVE_PYTHON_MATPLOT_LIB
rina-show-plots: rina-show-plots.py Makefile
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.py >$@.tmp
	chmod a-w $@.tmp
	chmod a+x $@.tmp
	mv $@.tmp $@

bin_SCRIPTS    += rina-show-plots
DISTCLEANFILES += rina-show-plots
EXTRA_DIST     += rina-show-plots.py
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT += rina-show-plots
endif
endif

rina-bug-report: rina-bug-report.in Makefile
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.in >$@.tmp
	chmod a-w $@.tmp
	chmod a+x $@.tmp
	mv $@.tmp $@

bin_SCRIPTS    += rina-bug-report
DISTCLEANFILES += rina-bug-report
EXTRA_DIST     += rina-bug-report.in
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT += rina-bug-report

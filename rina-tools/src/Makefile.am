#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

EXTRA_DIST   =
bin_PROGRAMS =

#rina_wrap_SOURCES  = rina-wrap.c
#rina_wrap_LDFLAGS  =
#rina_wrap_LDADD    =
#rina_wrap_CPPFLAGS = $(CPPFLAGS_EXTRA)
#bin_PROGRAMS       += rina-wrap

if HAVE_PYTHON
edit = $(SED)							\
        -e 's|@PACKAGE_VERSION[@]|$(PACKAGE_VERSION)|g'		\
        -e 's|@PACKAGE_NAME[@]|$(PACKAGE_NAME)|g'		\
        -e 's|@PACKAGE_URL[@]|$(PACKAGE_URL)|g'			\
        -e 's|@PACKAGE_BUGREPORT[@]|$(PACKAGE_BUGREPORT)|g'	\
        -e 's|@PYTHON[@]|$(PYTHON)|g'				\
        -e 's|@SHELL[@]|$(SHELL)|g'

rina-mem-stats: rina-mem-stats.py Makefile
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.py >$@.tmp
	chmod a-w $@.tmp
	chmod a+x $@.tmp
	mv $@.tmp $@

bin_PROGRAMS += rina-mem-stats

if HAVE_PYTHON_MATPLOT_LIB
rina-show-plots: rina-show-plots.py Makefile
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.py >$@.tmp
	chmod a-w $@.tmp
	chmod a+x $@.tmp
	mv $@.tmp $@

bin_PROGRAMS += rina-show-plots
endif
endif

#rina-bug-report: rina-bug-report.in Makefile
#	rm -f $@ $@.tmp
#	srcdir=''; \
#	  test -f ./$@.in || srcdir=$(srcdir)/; \
#	  $(edit) $${srcdir}$@.in >$@.tmp
#	chmod a-w $@.tmp
#	chmod a+x $@.tmp
#	mv $@.tmp $@
#
#bin_PROGRAMS += rina-bug-report

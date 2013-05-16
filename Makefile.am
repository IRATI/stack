#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

ACLOCAL_AMFLAGS      = -I m4
DISTCLEANFILES       =
MAINTAINERCLEANFILES =
BUILT_SOURCES        =

SUBDIRS              =				\
	src

EXTRA_DIST     =				\
	bootstrap				\
	build-aux/git-version-gen		\
	build-aux/gitlog-to-changelog
	LICENSE

.PHONY: AUTHORS
AUTHORS:
	$(GIT) log --format="%aN <%aE>" | $(SORT) | $(UNIQ) > AUTHORS || { \
		rm -f AUTHORS ;						   \
		exit 1 ;						   \
	}
EXTRA_DIST           += AUTHORS
MAINTAINERCLEANFILES += AUTHORS

.PHONY: ChangeLog
ChangeLog:
	$(srcdir)/build-aux/gitlog-to-changelog		\
		--amend=$(srcdir)/.changelog-amend |	\
		$(FMT) -u > ChangeLog || {		\
		rm -f ChangeLog ;			\
		exit 1 ;				\
	}
EXTRA_DIST           += ChangeLog
MAINTAINERCLEANFILES += ChangeLog

.PHONY: NEWS
NEWS:
	$(GIT) tag -l -n1 |						  \
	$(SORT) -V |							  \
	$(SED) -e 's,Version[ \\t]\+bump[ \\t]\+(\(.*\)),\1,' > NEWS || { \
		rm -f NEWS ;						  \
		exit 1 ;						  \
	}
EXTRA_DIST           += NEWS
MAINTAINERCLEANFILES += NEWS

$(top_srcdir)/.version:
	echo $(VERSION) > $@-t && mv $@-t $@
BUILT_SOURCES  += $(top_srcdir)/.version

dist-hook: dist-repo-cleanup
	echo $(VERSION) > $(distdir)/.tarball-version

# We are using git, remove unnecessary cruft from nsis-ka (external) svn repo
dist-repo-cleanup:
	rm -rf `find $(distdir) -name .svn`

# A workaround, to be removed ASAP
.PHONY: png
png:
	rm -f src/sigmgr/signald/sigmgr/service.dot
	$(MAKE) $(AM_FLAGS) -C  src/sigmgr/signald sigmgr/service.png

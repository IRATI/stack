#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

ACLOCAL_AMFLAGS           = -I m4
DISTCLEANFILES            =
MAINTAINERCLEANFILES      =
BUILT_SOURCES             =
DISTCHECK_CONFIGURE_FLAGS =

SUBDIRS =					\
	src					\
	doc

EXTRA_DIST =					\
	bootstrap				\
	build-aux/git-version-gen		\
	build-aux/gitlog-to-changelog		\
	LICENSE					\
	INSTALL					\
	README

.PHONY: AUTHORS
AUTHORS:
	$(GIT) log --format="%aN <%aE>" | $(SORT) | $(UNIQ) > AUTHORS || { \
		rm -f AUTHORS ;						   \
		exit 1 ;						   \
	}
EXTRA_DIST     += AUTHORS
DISTCLEANFILES += AUTHORS

.PHONY: ChangeLog
ChangeLog:
	$(srcdir)/build-aux/gitlog-to-changelog		\
		--amend=$(srcdir)/.changelog-amend |	\
		$(FMT) -u > ChangeLog || {		\
		rm -f ChangeLog ;			\
		exit 1 ;				\
	}
EXTRA_DIST     += ChangeLog .changelog-amend
DISTCLEANFILES += ChangeLog

.PHONY: NEWS
NEWS:
	$(GIT) tag -l -n1 |						  \
	$(SORT) -V |							  \
	$(SED) -e 's,Version[ \\t]\+bump[ \\t]\+(\(.*\)),\1,' > NEWS || { \
		rm -f NEWS ;						  \
		exit 1 ;						  \
	}
EXTRA_DIST     += NEWS
DISTCLEANFILES += NEWS

$(top_srcdir)/.version:
	echo $(VERSION) > $@-t && mv $@-t $@
BUILT_SOURCES  += $(top_srcdir)/.version

dist-hook:
	echo $(VERSION) > $(distdir)/.tarball-version

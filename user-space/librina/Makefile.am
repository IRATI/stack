#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

ACLOCAL_AMFLAGS           = -I m4
DISTCLEANFILES            =
MAINTAINERCLEANFILES      =
BUILT_SOURCES             =
DISTCHECK_CONFIGURE_FLAGS = --enable-java-wrappers

SUBDIRS =					\
	include					\
	src					\
	wrap					\
	apps					\
	test					\
	doc

EXTRA_DIST =					\
	bootstrap				\
	build-aux/git-version-gen		\
	build-aux/gitlog-to-changelog		\
	LICENSE					\
	README					\
	librina.pc.in

edit = $(SED)							\
        -e 's|@PACKAGE_VERSION[@]|$(PACKAGE_VERSION)|g'		\
	-e 's|@LIBNL3_LIBS[@]|$(LIBNL3_LIBS)|g'			\
	-e 's|@LIBNLGENL3_LIBS[@]|$(LIBNLGENL3_LIBS)|g'		\
	-e 's|@LIBNL3_CFLAGS[@]|$(LIBNL3_CFLAGS)|g'		\
	-e 's|@LIBNLGENL3_CFLAGS[@]|$(LIBNLGENL3_CFLAGS)|g'	\
        -e 's|@prefix[@]|$(prefix)|g'				\
        -e 's|@includedir[@]|$(includedir)|g'			\
        -e 's|@libdir[@]|$(libdir)|g'

librina.pc: Makefile librina.pc.in
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.in >$@.tmp
	chmod +x $@.tmp
	chmod a-w $@.tmp
	mv $@.tmp $@

CLEANFILES = librina.pc

pkgconfigdir   = $(libdir)
pkgconfig_DATA = librina.pc

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

check-functional:
	cd test && $(MAKE) $(AM_MAKEFLAGS) check-functional

check-all:
	cd test && $(MAKE) $(AM_MAKEFLAGS) check-all

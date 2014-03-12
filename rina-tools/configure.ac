#
# configure.ac
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

AC_INIT([RINA Tools],
        m4_esyscmd([build-aux/git-version-gen .tarball-version]),
        [f.salvestrini@nextworks.it],
        [rina-tools],
        [http://irati.eu])

AC_PREREQ([2.68])

AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_SRCDIR([src/rina-wrap.c])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE([1.11.1 tar-pax foreign -Wall])
#AM_INIT_AUTOMAKE([1.11.1 tar-pax foreign std-options -Wall])

AC_PROG_SED
AC_PROG_GREP
AC_PROG_MKDIR_P
AC_PROG_LN_S

AC_PATH_PROG([FMT],  [fmt],  [/bin/false])
AC_PATH_PROG([SORT], [sort], [/bin/false])
AC_PATH_PROG([UNIQ], [uniq], [/bin/false])
AC_PATH_PROG([GIT],  [git],  [/bin/false])

AC_PROG_CPP
AC_PROG_CC
AM_PROG_CC_C_O

CPPFLAGS_EXTRA=""
AX_CHECK_COMPILE_FLAG([ -Wall -Werror ], [
    CPPFLAGS_EXTRA="$CPPFLAGS_EXTRA -Wall -Werror"
])
AX_CHECK_COMPILE_FLAG([ -Wformat ], [
    CPPFLAGS_EXTRA="$CPPFLAGS_EXTRA -Wformat"
])
AC_SUBST(CPPFLAGS_EXTRA, $CPPFLAGS_EXTRA)

AM_PATH_PYTHON([2.0],,[:])
AM_CONDITIONAL([HAVE_PYTHON], [ test "$PYTHON" != : ])

PKG_PROG_PKG_CONFIG([0.26])

AC_CONFIG_FILES([
    Makefile

    src/Makefile
])

AC_OUTPUT

#
# configure.ac
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

AC_INIT([RINA Libraries],
        m4_esyscmd([build-aux/git-version-gen .tarball-version]),
        [f.salvestrini@nextworks.it],
        [librina],
        [http://www.irati.eu])

AC_PREREQ([2.68])

AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE([1.11.1 tar-pax foreign std-options -Wall])

AC_PROG_SED
AC_PROG_GREP
AC_PROG_MKDIR_P
AC_PROG_LN_S

AC_PATH_PROG([FMT],  [fmt],  [/bin/false])
AC_PATH_PROG([SORT], [sort], [/bin/false])
AC_PATH_PROG([UNIQ], [uniq], [/bin/false])
AC_PATH_PROG([GIT],  [git],  [/bin/false])

AC_PROG_CXXCPP
AC_PROG_CXX
AC_PROG_CXX_C_O

LT_INIT

AX_PKG_SWIG([2.0], [
    #
    # SWIG version > 2.0.4 && < 2.0.8 have a bug that prevent correct wrappers
    # generation
    #

    SWIG_VERSION=`$SWIG -version 2>&1 | grep 'SWIG Version' | sed 's/.*\([0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\).*/\1/g'`
    AX_COMPARE_VERSION($SWIG_VERSION, le, [2.0.4],,[
        AX_COMPARE_VERSION($SWIG_VERSION, ge, [2.0.8],,[
            AC_MSG_ERROR([SWIG versions in 2.0.4, ..., 2.0.8 range won't compile])
        ])
    ])
], [
    AC_MSG_ERROR([Your system lacks of SWIG (>2.0) support])
])

AC_CONFIG_FILES([
    Makefile

    include/Makefile
    src/Makefile
])

AC_OUTPUT

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
AC_CONFIG_SRCDIR([src/rina-bug-report.in])
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

AC_PATH_PROG([FMT],  [fmt],  [no])
AS_IF([test "x$FMT" = "xno"],[
	AC_MSG_ERROR([Your system lacks of fmt support])
])
AC_PATH_PROG([SORT], [sort], [no])
AS_IF([test "x$SORT" = "xno"],[
	AC_MSG_ERROR([Your system lacks of sort support])
])
AC_PATH_PROG([UNIQ], [uniq], [no])
AS_IF([test "x$UNIQ" = "xno"],[
	AC_MSG_ERROR([Your system lacks of uniq support])
])
AC_PATH_PROG([GIT],  [git],  [no])
AS_IF([test "x$GIT" = "xno"],[
	AC_MSG_ERROR([Your system lacks of git support])
])

AC_PROG_CPP
AC_PROG_CC
AM_PROG_CC_C_O

AC_PROG_CXXCPP
AC_PROG_CXX
AC_PROG_CXX_C_O

# Just to avoid warning from libtool 1.14
AM_PROG_AR
LT_INIT

CPPFLAGS_EXTRA=""
CXXFLAGS_EXTRA=""

#Set this for TCLAP... we don't support compilers with incomplete std C++ libs
#so we don't even check it.
AC_DEFINE([HAVE_SSTREAM], [1], [TCLAP needs this])

AX_CHECK_COMPILE_FLAG([ -Wall ], [
    CXXFLAGS_EXTRA="$CXXFLAGS_EXTRA -Wall"
])
AX_CHECK_COMPILE_FLAG([ -Wformat ], [
    CXXFLAGS_EXTRA="$CXXFLAGS_EXTRA -Wformat"
])
AX_CHECK_COMPILE_FLAG([ -Wnon-virtual-dtor ], [
    CXXFLAGS_EXTRA="$CXXFLAGS_EXTRA -Wnon-virtual-dtor"
])
AX_CHECK_COMPILE_FLAG([ -Woverloaded-virtual ], [
    CXXFLAGS_EXTRA="$CXXFLAGS_EXTRA -Woverloaded-virtual"
])
AX_CHECK_COMPILE_FLAG([ -Werror ], [
    CXXFLAGS_EXTRA="$CXXFLAGS_EXTRA -Werror"
])
AX_CHECK_COMPILE_FLAG([ -Wfatal-errors ], [
    CXXFLAGS_EXTRA="$CXXFLAGS_EXTRA -Wfatal-errors"
])

want_debug=no
AX_SELECTOR_ENABLE([debug],[
    want_debug=yes
],[
    want_debug=no
])
AS_IF([test "$want_debug" = "yes"],[
    AX_CHECK_COMPILE_FLAG([ -g ], [
        CXXFLAGS_EXTRA="$CXXFLAGS_EXTRA -g"
    ])
    AS_IF([`echo $CXXFLAGS|$GREP '\\\-O2'`],[
        AC_MSG_NOTICE([Patching flags])
        CXXFLAGS="`echo $CXXFLAGS | $SED -e 's|-O2||'`"
        AC_MSG_NOTICE([Flags are now '$CXXFLAGS'])
    ])
    AX_CHECK_COMPILE_FLAG([ -O0 ], [
        CXXFLAGS_EXTRA="$CXXFLAGS_EXTRA -O0"
    ])
    AX_CHECK_COMPILE_FLAG([ -fno-inline ], [
        CXXFLAGS_EXTRA="$CXXFLAGS_EXTRA -fno-inline"
    ])
])

AC_SUBST(CPPFLAGS_EXTRA, $CPPFLAGS_EXTRA)
AC_SUBST(CXXFLAGS_EXTRA, $CXXFLAGS_EXTRA)

ax_cv_have_python=yes
ax_cv_have_matplotlib=yes
AM_PATH_PYTHON([2.0],[
    AX_PYTHON_MODULE([re],,[
        AC_MSG_WARN([Python module re seems missing (???), some features will be disabled])
        ax_cv_have_python=no
    ])
    AX_PYTHON_MODULE([sys],,[
        AC_MSG_WARN([Python module sys seems missing (???), some features will be disabled])
        ax_cv_have_python=no
    ])
    AX_PYTHON_MODULE([argparse],,[
        AC_MSG_WARN([Python module arparse seems missing (???), some features will be disabled])
        ax_cv_have_python=no
    ])
    AX_PYTHON_MODULE([matplotlib],,[
        AC_MSG_WARN([Python module matplotlib seems missing, some features will be disabled])
        ax_cv_have_matplotlib=no
    ])
],[
    AC_MSG_WARN([Python (>= 2.0) seems missing, some features will be disabled])
        ax_cv_have_python=no
        ax_cv_have_matplotlib=no
])
AM_CONDITIONAL([HAVE_PYTHON], [ test "$ax_cv_have_python" != "no" ])
AM_CONDITIONAL([HAVE_PYTHON_MATPLOT_LIB], [ test "$ax_cv_have_matplotlib" != "no" ])

PKG_PROG_PKG_CONFIG([0.26])

PKG_CHECK_MODULES([LIBRINA], [librina >= 0.2.0],, [
    AC_MSG_ERROR([Your system lacks of librina support])
])
# FIXME: A cleaner solution would be better
AC_SUBST([LIBRINA_VERSION],`pkg-config --modversion librina`)

AC_CONFIG_FILES([
    Makefile

    src/Makefile
    src/common/Makefile
    src/rina-echo-time/Makefile
    src/rina-cdap-echo/Makefile
    src/manager/Makefile
    src/common/tclap/Makefile

    doc/Makefile
])

AC_OUTPUT

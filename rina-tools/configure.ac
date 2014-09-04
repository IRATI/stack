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

AM_INIT_AUTOMAKE([1.11.1 tar-pax foreign std-options -Wall])

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
AX_CHECK_COMPILE_FLAG([ -Wall ], [
    CPPFLAGS_EXTRA="$CPPFLAGS_EXTRA -Wall"
])
AX_CHECK_COMPILE_FLAG([ -Wformat ], [
    CPPFLAGS_EXTRA="$CPPFLAGS_EXTRA -Wformat"
])
AX_CHECK_COMPILE_FLAG([ -Wnon-virtual-dtor ], [
    CPPFLAGS_EXTRA="$CPPFLAGS_EXTRA -Wnon-virtual-dtor"
])
AX_CHECK_COMPILE_FLAG([ -Woverloaded-virtual ], [
    CPPFLAGS_EXTRA="$CPPFLAGS_EXTRA -Woverloaded-virtual"
])
AX_CHECK_COMPILE_FLAG([ -Wextra ], [
    CPPFLAGS_EXTRA="$CPPFLAGS_EXTRA -Wextra"
])
AX_CHECK_COMPILE_FLAG([ -Werror ], [
    CPPFLAGS_EXTRA="$CPPFLAGS_EXTRA -Werror"
])
AX_CHECK_COMPILE_FLAG([ -Wfatal-errors ], [
    CPPFLAGS_EXTRA="$CPPFLAGS_EXTRA -Wfatal-errors"
])
AC_SUBST(CPPFLAGS_EXTRA, $CPPFLAGS_EXTRA)

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

build_tools_java=no
AX_SELECTOR_ENABLE([java-support],[
    build_tools_java=yes
],[
    build_tools_java=no
])

AS_IF([test "$build_tools_java" = "yes"],[
    AX_PROG_JAVAC
    AX_JNI_INCLUDE_DIR

    # FIXME: Ugly as hell ...

    JNI_CPPFLAGS=""
    for JNI_INCLUDE_DIR in $JNI_INCLUDE_DIRS ;
    do
        JNI_CPPFLAGS="$JNI_CPPFLAGS -I$JNI_INCLUDE_DIR" ;
    done
    AC_SUBST(JNI_CPPFLAGS, $JNI_CPPFLAGS)
    AX_PROG_JAR
])
AM_CONDITIONAL(BUILD_TOOLS_JAVA, [test $build_tools_java = yes])

AC_PATH_PROG([MVN],[mvn],[])
AS_IF([test x"$MVN" != x""],[
    AC_MSG_WARN([Your system lacks of maven support, Java parts will not be built/installed])
])
AM_CONDITIONAL(BUILD_MAVEN_SUPPORT, [test x"$MVN" != x""])

PKG_PROG_PKG_CONFIG([0.26])

PKG_CHECK_MODULES([LIBRINA], [librina >= 0.2.0],, [
    AC_MSG_ERROR([Your system lacks of librina support])
])
# FIXME: A cleaner solution would be better
AC_SUBST([LIBRINA_VERSION],`pkg-config --modversion librina`)

AC_CONFIG_FILES([
    Makefile

    src/Makefile
    src/apps/Makefile
    src/rina-echo-time/Makefile

    doc/Makefile
])

AC_OUTPUT

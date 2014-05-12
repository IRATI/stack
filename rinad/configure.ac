#
# configure.ac
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

AC_INIT([RINA Daemons],
        m4_esyscmd([build-aux/git-version-gen .tarball-version]),
        [f.salvestrini@nextworks.it],
        [rinad],
        [http://irati.eu])

AC_PREREQ([2.68])

AC_CONFIG_SRCDIR([src-old/pom.xml.in])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

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

AX_PROG_JAVA

PKG_PROG_PKG_CONFIG([0.26])

PKG_CHECK_MODULES([LIBRINA], [librina >= 0.2.0],, [
    AC_MSG_ERROR([Your system lacks of librina support])
])
# FIXME: A cleaner solution would be better
AC_SUBST([LIBRINA_VERSION],`pkg-config --modversion librina`)

AC_PATH_PROG([MVN],[mvn],[])
AS_IF([test -z "$MVN"],[
    AC_MSG_ERROR([Your system lacks maven support])
])

AC_CONFIG_FILES([
    Makefile
    src-old/Makefile
    src/Makefile
])

AC_OUTPUT

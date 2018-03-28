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

AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_SRCDIR([src/ipcm/main.cc])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11.1 tar-pax foreign -Wall subdir-objects])
#AM_INIT_AUTOMAKE([1.11.1 tar-pax foreign std-options -Wall])

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

AC_PROG_CXXCPP
AC_PROG_CXX
AC_PROG_CXX_C_O

AC_CHECK_HEADERS([execinfo.h])
AC_CHECK_FUNCS([backtrace backtrace_symbols])

# Just to avoid warnings from libtool 1.14
AM_PROG_AR
# Just to avoid warnings coming from wpa_ctrl.c
AM_PROG_CC_C_O
LT_INIT

AC_DEFINE_UNQUOTED([__INSTALLATION_PREFIX], ["$prefix"],
		[The installation prefix.])

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

#AX_CXX_COMPILE_STDCXX_11([noext],[mandatory])

PKG_PROG_PKG_CONFIG([0.26])

PKG_CHECK_MODULES([LIBRINA], [librina],, [
    AC_MSG_ERROR([Your system lacks of librina support])
])
# FIXME: A cleaner solution would be better
AC_SUBST([LIBRINA_VERSION],`pkg-config --modversion librina`)

PKG_CHECK_MODULES([LIBRINA_API], [librina-api],, [
    AC_MSG_ERROR([Your system lacks of librina-api support])
])
# FIXME: A cleaner solution would be better
AC_SUBST([LIBRINA_API_VERSION],`pkg-config --modversion librina-api`)

AC_PATH_PROG([PROTOC], [protoc], [no])
AS_IF([test "x$PROTOC" = "xno"],[
    AC_MSG_ERROR([Your system lacks of protoc support])
])

LIBPROTOBUF_MIN_VERSION=2.5.0
AC_SUBST(LIBPROTOBUF_MIN_VERSION, $LIBPROTOBUF_MIN_VERSION)

PKG_CHECK_MODULES(LIBPROTOBUF, [protobuf >= $LIBPROTOBUF_MIN_VERSION],, [
    AC_MSG_ERROR([Your system lacks of libprotobuf support (>= $LIBPROTOBUF_MIN_VERSION)])
])

AC_PATH_PROG([PERL], [perl], [no])
AM_CONDITIONAL([HAVE_PERL], [ test "$PERL" != "no" ])

build_bindings=no
build_bindings_swig=no
build_bindings_java=no

AX_SELECTOR_ENABLE([java-bindings],[
    build_bindings_java=yes
],[
    build_bindings_java=no
])
AS_IF([test "$build_bindings_java" = "yes"],[
    build_bindings=yes
    build_bindings_swig=yes
])

AS_IF([test "$build_bindings_swig" = "yes"],[
    AX_PKG_SWIG([2.0],[
        #
        # SWIG version > 2.0.4 && < 2.0.8 have a bug preventing correct
        # bindings generation
        #

        SWIG_VERSION=`$SWIG -version 2>&1 | $GREP 'SWIG Version' | $SED 's/.*\([0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\).*/\1/g'`
        AX_COMPARE_VERSION($SWIG_VERSION, le, [2.0.4],,[
            AX_COMPARE_VERSION($SWIG_VERSION, ge, [2.0.8],,[
                AC_MSG_ERROR([SWIG versions in 2.0.4, ..., 2.0.8 range will not compile])
            ])
        ])
    ],[
        AC_MSG_ERROR([Your system lacks of SWIG (>2.0) support, bindings generation is not possible])
    ])
])

AS_IF([test "$build_bindings_java" = "yes"],[
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
AM_CONDITIONAL(BUILD_BINDINGS_JAVA, [test $build_bindings_java = yes])

AC_PATH_PROG([MVN],[mvn],[])
AS_IF([test x"$MVN" = x""],[
    AC_MSG_WARN([Your system lacks of maven support, Java parts will not be built/installed])
])
AM_CONDITIONAL(BUILD_MAVEN_SUPPORT, [test x"$MVN" != x""])

AC_CONFIG_FILES([
    Makefile

    src/Makefile
    src/common/Makefile
    src/common/encoders/Makefile
    src/common/tclap/Makefile
    src/ipcp/Makefile
    src/ipcp/plugins/Makefile
    src/ipcp/plugins/default/Makefile
    src/ipcp/plugins/sm-auth-psoc/Makefile
    src/ipcp/plugins/sm-passwd/Makefile
    src/ipcp/plugins/sm-cbac/Makefile
    src/ipcp/plugins/nsm-address-change/Makefile
    src/ipcm/Makefile
    src/ipcm/addons/Makefile
    src/ipcm/addons/ma/Makefile
    src/ipcm/addons/ma/ribs/Makefile
    src/ipcm/include/Makefile
    src/ipcm/include/ipcm/Makefile

    wrap/Makefile
    wrap/java/Makefile

    doc/Makefile
    tools/Makefile
    etc/Makefile
])

AC_OUTPUT

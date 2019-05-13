#
# configure.ac
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

AC_INIT([RINA Libraries],
        m4_esyscmd([build-aux/git-version-gen .tarball-version]),
        [f.salvestrini@nextworks.it],
        [librina],
        [http://irati.eu])

AC_PREREQ([2.68])

AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_SRCDIR([src/librina.cc])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

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

AC_PROG_CPP
AC_PROG_CC
AC_PROG_CC_C_O

AC_PROG_CXXCPP
AC_PROG_CXX
AC_PROG_CXX_C_O

# Just to avoid warning from libtool 1.14
AM_PROG_AR
LT_INIT

CPPFLAGS_EXTRA=""
CXXFLAGS_EXTRA=""

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

want_debug=yes
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

PKG_PROG_PKG_CONFIG([0.26])

OPENSSL_MIN_VERSION=1.0.1
AC_SUBST(OPENSSL_MIN_VERSION, $OPENSSL_MIN_VERSION)

AC_CHECK_LIB([pthread],[pthread_create],,[
	          AC_MSG_ERROR([Your system lacks of libpthread support])
		  ])

PKG_CHECK_MODULES(OPENSSL, [openssl >= $OPENSSL_MIN_VERSION],, [
			        AC_MSG_ERROR([Your system lacks of openssl support (>= $OPENSSL_MIN_VERSION)])
				])

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

AC_PATH_PROG([PROTOC], [protoc], [no])
AS_IF([test "x$PROTOC" = "xno"],[
       	AC_MSG_ERROR([Your system lacks of protoc support])
	])

LIBPROTOBUF_MIN_VERSION=2.6.1
AC_SUBST(LIBPROTOBUF_MIN_VERSION, $LIBPROTOBUF_MIN_VERSION)

PKG_CHECK_MODULES(LIBPROTOBUF, [protobuf >= $LIBPROTOBUF_MIN_VERSION],, [
				    AC_MSG_ERROR([Your system lacks of libprotobuf support (>= $LIBPROTOBUF_MIN_VERSION)])
				    ])

DX_DOXYGEN_FEATURE(ON)
DX_DOT_FEATURE(ON)
DX_HTML_FEATURE(ON)
DX_CHM_FEATURE(OFF)
DX_CHI_FEATURE(OFF)
DX_MAN_FEATURE(ON)
DX_RTF_FEATURE(OFF)
DX_XML_FEATURE(OFF)
DX_PDF_FEATURE(ON)
DX_PS_FEATURE(OFF)
DX_INIT_DOXYGEN($PACKAGE_NAME,[doxygen.cfg],[doxygen])

AC_CONFIG_FILES([
		     Makefile

		     include/Makefile
		     include/librina/Makefile

	             src/Makefile

		     wrap/Makefile
	             wrap/rina-api/Makefile
                     wrap/rina-api/src/Makefile
	             wrap/rina-api/include/Makefile
                     wrap/rina-api/include/rina/Makefile

	             test/Makefile

	             doc/Makefile
		])

AC_OUTPUT


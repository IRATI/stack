#
# SYNOPSIS
#
#   AX_SELECTOR_ENABLE(FEATURE,[IF-WANT-FEATURE],[IF-DONT-WANT-FEATURE])
#
#   The feature is disabled by default, --enable-<feature> to manually switch
#   it on
#
# WRITTEN BY:
#
#   Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

AC_DEFUN([AX_SELECTOR_ENABLE],[
    pushdef([CANONICAL_UP],  translit([$1], [a-z-], [A-Z_]))
    pushdef([CANONICAL_LOW], translit([$1], [A-Z-], [a-z_]))

    AC_MSG_CHECKING([whether $1 related features have been enabled])

    AC_ARG_ENABLE([$1],
        AS_HELP_STRING([--enable-][$1],[enable $1 related features]),[
        AS_IF([test ! $enableval = "no"],[
            ax_cv_want_[]CANONICAL_LOW=yes
        ],[
            ax_cv_want_[]CANONICAL_LOW=no
        ])
    ],[
        ax_cv_want_[]CANONICAL_LOW=no
    ])
    AC_MSG_RESULT([${ax_cv_want_[]CANONICAL_LOW}])
    AS_IF([test "${ax_cv_want_[]CANONICAL_LOW}" = "yes"],[$2],[$3])

    AM_CONDITIONAL(BUILD_[]CANONICAL_UP, [test "${ax_cv_want_[]CANONICAL_LOW}" = "yes"])

    AS_IF([test "${ax_cv_want_[]CANONICAL_LOW}" = "yes"],[
        ax_cv_define_[]CANONICAL_LOW=1
    ],[
        ax_cv_define_[]CANONICAL_LOW=0
    ])

    AC_DEFINE_UNQUOTED(WANT_[]CANONICAL_UP, [${ax_cv_define_[]CANONICAL_LOW}], [Define if you want $1])

    popdef([CANONICAL_LOW])
    popdef([CANONICAL_UP])
])

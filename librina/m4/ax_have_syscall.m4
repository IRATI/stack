#
# SYNOPSIS
#
#   AX_HAVE_SYSCALL([syscall],[ACTION-IF-FOUND],[ACTION-IF-NOT-FOUND])
#
# WRITTEN BY:
#
#   Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

AC_DEFUN([AX_HAVE_SYSCALL], [dnl
    AC_MSG_CHECKING([for $1 syscall availability])
    AC_CACHE_VAL([ax_cv_have_syscall_$1], [
        AC_LINK_IFELSE([
            AC_LANG_PROGRAM([
                #include <asm/unistd.h>
            ], [
                int i = __NR_$1;
            ])
        ],[
            ax_cv_have_syscall_$1=yes
        ],[
            ax_cv_have_syscall_$1=no
        ])
    ])
    AS_IF([test "${ax_cv_have_syscall_$1}" = "yes"],[
        AC_MSG_RESULT([yes])
        $2
    ],[
        AC_MSG_RESULT([no])
        $3
    ])
])

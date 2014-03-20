#
# SYNOPSIS
#
#   AX_PYTHON_MODULE(MODULE,[ACTION-IF-FOUND],[ACTION-IF-NOT-FOUND])
#
# LICENSE
#
#   @LICENSE_BEGIN@
#   @LICENSE_END@
#
# WRITTEN BY:
#
#   Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

AC_DEFUN([AX_PYTHON_MODULE],[
    AS_IF([test x"$PYTHON" = x""],[
        AC_MSG_WARN([No python available, cannot check $1 module presence])
        $3
    ],[
        AC_MSG_CHECKING([Python module $1])
        $PYTHON -c "import $1" >/dev/null 2>&1
        AS_IF([test $? -eq 0],[
            AC_MSG_RESULT([yes])
            $2
        ],[
            AC_MSG_RESULT([no])
            $3
        ])
    ])
])

/* vi: set sw=4 ts=4: */
/*
 * eventfd() for uClibc
 *
 * Copyright (C) 2011 Jean-Christian de Rivaz <jc@eclis.ch>
 *
 * Licensed under the LGPL v2.1, see the file COPYING.LIB in this tarball.
 */

#include <sys/syscall.h>
#include <sys/eventfd.h>

/*
 * eventfd()
 */
#ifdef __NR_eventfd
_syscall2(int, eventfd, int, count, int, flags)
#endif

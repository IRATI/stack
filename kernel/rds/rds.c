/*
 * RINA Data Structures
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#define RINA_PREFIX "rds"

#include "logs.h"
#include "debug.h"
#include "rds.h"

#ifdef CONFIG_RINA_RQUEUE_REGRESSION_TESTS
extern bool regression_tests_rqueue(void);
#endif
#ifdef CONFIG_RINA_RFIFO_REGRESSION_TESTS
extern bool regression_tests_rfifo(void);
#endif
#ifdef CONFIG_RINA_RTIMER_REGRESSION_TESTS
extern bool regression_tests_rtimer(void);
#endif
#ifdef CONFIG_RINA_RMEM_REGRESSION_TESTS
extern bool regression_tests_rmem(void);
#endif
#ifdef CONFIG_RINA_RINGQ_REGRESSION_TESTS
extern bool regression_tests_ringq(void);
#endif

bool regression_tests_rds(void)
{
#ifdef CONFIG_RINA_RQUEUE_REGRESSION_TESTS
        if (!regression_tests_rqueue())
                return false;
#endif
#ifdef CONFIG_RINA_RFIFO_REGRESSION_TESTS
        if (!regression_tests_rfifo())
                return false;
#endif
#ifdef CONFIG_RINA_RTIMER_REGRESSION_TESTS
        if (!regression_tests_rtimer())
                return false;
#endif
#ifdef CONFIG_RINA_RMEM_REGRESSION_TESTS
        if (!regression_tests_rmem())
                return false;
#endif
#ifdef CONFIG_RINA_RINGQ_REGRESSION_TESTS
        if (!regression_tests_ringq())
                return false;
#endif

        return true;
}
EXPORT_SYMBOL(regression_tests_rds);

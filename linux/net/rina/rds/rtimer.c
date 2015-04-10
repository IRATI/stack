/*
 * RINA Bitmaps
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

#include <linux/export.h>
#include <linux/types.h>
#include <linux/timer.h>

#define RINA_PREFIX "rtimer"

#include "logs.h"
#include "debug.h"
#include "rmem.h"
#include "rtimer.h"

struct rtimer {
        struct timer_list tl;
        void (* function)(void * data);
        void * data;
};

static struct rtimer * rtimer_create_gfp(gfp_t   flags,
                                         void (* function)(void * data),
                                         void *  data)
{
        struct rtimer * tmp;

        if (!function) {
                LOG_DBG("Bogus input parameter, cannot create timer");
                return NULL;
        }

        tmp = rkmalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        tmp->function = function;
        tmp->data     = data;

        init_timer(&tmp->tl);

        LOG_DBG("Timer %pK created", tmp);

        return tmp;
}

struct rtimer * rtimer_create(void (* function)(void * data), void * data)
{ return rtimer_create_gfp(GFP_KERNEL, function, data); }
EXPORT_SYMBOL(rtimer_create);

struct rtimer * rtimer_create_ni(void (* function)(void * data), void * data)
{ return rtimer_create_gfp(GFP_ATOMIC, function, data); }
EXPORT_SYMBOL(rtimer_create_ni);

static bool __rtimer_is_pending(struct rtimer * timer)
{
        ASSERT(timer);

        return timer_pending(&timer->tl) ? true : false;
}

bool rtimer_is_pending(struct rtimer * timer)
{
        if (!timer)
                return false;

        return __rtimer_is_pending(timer);
}
EXPORT_SYMBOL(rtimer_is_pending);

static int __rtimer_start(struct rtimer * timer,
                          unsigned int    millisecs)
{
        int status;

        ASSERT(timer);

        /* FIXME: Crappy, rearrange */
        timer->tl.function = (void (*)(unsigned long)) timer->function;
        timer->tl.data     = (unsigned long)           timer->data;
        timer->tl.expires  = jiffies + (millisecs * HZ) / 1000;

        status = mod_timer(&timer->tl, timer->tl.expires);


        LOG_DBG("Previously %s Timer %pK restarted (function = %pK, data = %pK, "
                "expires = %ld (%u)",
                status ? "active" : "inactive",
                timer,
                (void *) timer->tl.function,
                (void *) timer->tl.data,
                timer->tl.expires,
                millisecs);

        return 0;
}

int rtimer_start(struct rtimer * timer,
                 unsigned int    millisecs)
{
        if (!timer)
                return -1;

        if (__rtimer_is_pending(timer)) {
                LOG_DBG("Timer %pK is pending, can't start it", timer);
                return 0;
        }

        return __rtimer_start(timer, millisecs);
}
EXPORT_SYMBOL(rtimer_start);

static int __rtimer_stop(struct rtimer * timer)
{
        ASSERT(timer);

        if (!__rtimer_is_pending(timer)) {
                LOG_DBG("Timer %pK is not pending, can't stop it", timer);
                return 0;
        }

        del_timer_sync(&timer->tl);
        LOG_DBG("Timer %pK stopped", timer);

        return 0;
}

int rtimer_stop(struct rtimer * timer)
{
        if (!timer)
                return -1;

        return __rtimer_stop(timer);
}
EXPORT_SYMBOL(rtimer_stop);

int rtimer_restart(struct rtimer * timer,
                   unsigned int    millisecs)
{
        if (!timer)
                return -1;

        return __rtimer_start(timer, millisecs);
}
EXPORT_SYMBOL(rtimer_restart);

int rtimer_destroy(struct rtimer * timer)
{
        if (!timer)
                return -1;

        if (__rtimer_stop(timer))
                return -1;

        rkfree(timer);

        LOG_DBG("Timer %pK destroyed", timer);

        return 0;
}
EXPORT_SYMBOL(rtimer_destroy);

#ifdef CONFIG_RINA_RTIMER_REGRESSION_TESTS
static void timer0_function(void * data)
{ }

static int data1;

static void timer1_function(void * data)
{
        int * tmp = (int *)(data);

        *tmp *= 2;
}

static int data2;

static void timer2_function(void * data)
{
        int * tmp = (int *)(data);

        *tmp *= 4;
}

bool regression_tests_rtimer(void)
{
        struct rtimer * timer0;
        struct rtimer * timer1;
        struct rtimer * timer2;

        LOG_DBG("Timer - step 0");
        timer0 = rtimer_create(timer0_function, NULL);
        if (!timer0)
                return false;
        if (rtimer_destroy(timer0))
                return false;

        LOG_DBG("Timer - step 1");
        timer1 = rtimer_create(timer1_function, &data1);
        if (!timer1)
                return false;

        LOG_DBG("Timer - step 2");
        timer2 = rtimer_create(timer2_function, &data2);
        if (!timer2) {
                rtimer_destroy(timer1);
                return false;
        }

        data1 = 1;
        data2 = 1;

        LOG_DBG("Timer - step 3");
        if (rtimer_start(timer1, 1000)) {
                rtimer_destroy(timer1);
                rtimer_destroy(timer2);
                return false;
        }

        LOG_DBG("Timer - step 4");
        if (rtimer_start(timer2, 100)) {
                rtimer_destroy(timer1);
                rtimer_destroy(timer2);
                return false;
        }

        LOG_DBG("Timer - step 5");
        rtimer_destroy(timer1);
        rtimer_destroy(timer2);

        LOG_DBG("Timer - OK");

        return true;
}
#endif

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

        if (!data) {
                LOG_DBG("Bogus input parameter, cannot create timer");
                return NULL;
        }

        tmp = rkmalloc(sizeof(*tmp), flags);
        if (!tmp)
                return NULL;

        tmp->function = function;
        tmp->data     = data;

        return tmp;
}

struct rtimer * rtimer_create(void (* function)(void * data), void * data)
{ return rtimer_create_gfp(GFP_KERNEL, function, data); }
EXPORT_SYMBOL(rtimer_create);

struct rtimer * rtimer_create_ni(void (* function)(void * data), void * data)
{ return rtimer_create_gfp(GFP_ATOMIC, function, data); }
EXPORT_SYMBOL(rtimer_create_ni);

int rtimer_start(struct rtimer * timer,
                 unsigned int    millisec)
{
        if (!timer)
                return -1;

        init_timer(&timer->tl);

        /* FIXME: Crappy, rearrange */
        timer->tl.function = (void (*)(unsigned long)) timer->function;
        timer->tl.data     = (unsigned long)           timer->data;
        timer->tl.expires  = jiffies + (millisec * HZ) / 1000;

        add_timer(&timer->tl);

        return -1;
}
EXPORT_SYMBOL(rtimer_start);

static int _rtimer_stop(struct rtimer * timer)
{
        ASSERT(timer);

        del_timer(&timer->tl);

        return 0;
}

int rtimer_stop(struct rtimer * timer)
{
        if (!timer)
                return -1;

        return _rtimer_stop(timer);
}
EXPORT_SYMBOL(rtimer_stop);

int rtimer_destroy(struct rtimer * timer)
{
        if (!timer)
                return -1;

        if (_rtimer_stop(timer))
                return -1;

        rkfree(timer);

        return 0;
}
EXPORT_SYMBOL(rtimer_destroy);

#ifdef CONFIG_RINA_RTIMER_REGRESSION_TESTS
int data1;

static void timer1_function(void * data)
{
        int * tmp = (int *)(data);

        *tmp *= 2;
}

int data2;

static void timer2_function(void * data)
{
        int * tmp = (int *)(data);

        *tmp *= 4;
}

bool regression_tests_rtimer(void)
{
        struct rtimer * timer1;
        struct rtimer * timer2;

        timer1 = rtimer_create(timer1_function, &data1);
        if (!timer1)
                return false;

        timer2 = rtimer_create(timer2_function, &data2);
        if (!timer2) {
                rtimer_destroy(timer1);
                return false;
        }

        data1 = 1;
        data2 = 1;

        if (rtimer_start(timer1, 1000)) {
                rtimer_destroy(timer1);
                rtimer_destroy(timer2);
                return false;
        }

        if (rtimer_start(timer2, 100)) {
                rtimer_destroy(timer1);
                rtimer_destroy(timer2);
                return false;
        }

        rtimer_destroy(timer1);
        rtimer_destroy(timer2);

        return true;
}
#endif

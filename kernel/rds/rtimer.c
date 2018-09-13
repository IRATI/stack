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

int rtimer_init(
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0)
                                         void (* function)(void * data),
#else
					 void (*function)(struct timer_list * tl),
#endif
					 struct timer_list * tl, void *  data)
{
        if (!function) {
                LOG_DBG("Bogus input parameter, cannot create timer");
                return -1;
        }

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0)
        init_timer(tl);
        tl->function = (void (*)(unsigned long)) function;
        tl->data     = (unsigned long)      data;
#else
        timer_setup(tl, function, 0);
#endif

        return 0;
}
EXPORT_SYMBOL(rtimer_init);

bool rtimer_is_pending(struct timer_list * tl)
{
	return timer_pending(tl) ? true : false;
}
EXPORT_SYMBOL(rtimer_is_pending);

int rtimer_start(struct timer_list * tl,
                 unsigned int    millisecs)
{
        int status;
        unsigned long expires;

        if (rtimer_is_pending(tl)) {
                LOG_DBG("Timer %pK is pending, can't start it", tl);
                return 0;
        }

        expires = jiffies + (millisecs * HZ) / 1000;
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0)
        /* FIXME: Crappy, rearrange */
        tl->expires  = expires;
#endif

        status = mod_timer(tl, expires);

        return 0;
}
EXPORT_SYMBOL(rtimer_start);


int rtimer_stop(struct timer_list * tl)
{

        if (!rtimer_is_pending(tl)) {
                LOG_DBG("Timer %pK is not pending, can't stop it", tl);
                return 0;
        }

        del_timer_sync(tl);
        LOG_DBG("Timer %pK stopped", tl);

        return 0;
}
EXPORT_SYMBOL(rtimer_stop);

int rtimer_restart(struct timer_list * tl,
                   unsigned int    millisecs)
{

        return rtimer_start(tl, millisecs);
}
EXPORT_SYMBOL(rtimer_restart);

int rtimer_destroy(struct timer_list * tl)
{
        if (rtimer_stop(tl))
                return -1;

        LOG_DBG("Timer %pK destroyed", tl);

        return 0;
}
EXPORT_SYMBOL(rtimer_destroy);

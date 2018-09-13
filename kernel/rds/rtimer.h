/*
 * RINA Timers
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

#ifndef RINA_RTIMER_H
#define RINA_RTIMER_H

#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0)
int rtimer_init(void (* function)(void * data),
#else
int rtimer_init(void (*function)(struct timer_list * tl),
#endif
		struct timer_list * tl, void *  data);
int             rtimer_destroy(struct timer_list * tl);

int             rtimer_start(struct timer_list * tl,
                             unsigned int    millisecs);
bool            rtimer_is_pending(struct timer_list * tl);
int             rtimer_stop(struct timer_list * tl);
int             rtimer_restart(struct timer_list * tl,
                               unsigned int    millisecs);

#endif

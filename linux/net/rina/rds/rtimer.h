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

struct rtimer;

void *          rtimer_get_data(struct rtimer * timer);
struct rtimer * rtimer_create(void (* function)(void * data),
                              void *  data);
struct rtimer * rtimer_create_ni(void (* function)(void * data),
                                 void *  data);
int             rtimer_destroy(struct rtimer * timer);

int             rtimer_start(struct rtimer * timer,
                             unsigned int    millisecs);
bool            rtimer_is_pending(struct rtimer * timer);
int             rtimer_stop(struct rtimer * timer);
int             rtimer_restart(struct rtimer * timer,
                               unsigned int    millisecs);

#endif

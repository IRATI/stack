/*
 * Debugging facilities
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

#ifndef RINA_DEBUG_H
#define RINA_DEBUG_H

/* Embed assertions in the code upon user-choice */
#include <linux/bug.h>

#ifdef CONFIG_RINA_ASSERTIONS
#define ASSERT(COND) BUG_ON(!(COND))
#else
#define ASSERT(COND)
#endif

#include <linux/hardirq.h>

#define IRQ_BARRIER                                                     \
        do {                                                            \
                if (in_interrupt()) {                                   \
                        LOG_CRIT("Do not call %s in IRQ context",       \
                                 __FUNCTION__);                         \
                        BUG();                                          \
                }                                                       \
        } while (0)

#endif

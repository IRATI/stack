/*
 * Logs
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

#ifndef RINA_LOGS_H
#define RINA_LOGS_H

#ifndef RINA_PREFIX
#error You must define RINA_PREFIX before including this file
#endif

#include <linux/kernel.h>

/* The global logs prefix */
#define __GPFX "rina-"

#define __LOG(PFX, LVL, BAN, FMT, ARGS...)                                   \
        do { printk(LVL __GPFX PFX "(" BAN "): %s " FMT "\n", __func__, ##ARGS); } while (0)

/* Sorted by "urgency" (high to low) */
#define LOG_EMERG(FMT, ARGS...) __LOG(RINA_PREFIX, KERN_EMERG,      \
                                      "EMER", FMT, ##ARGS)
#define LOG_ALERT(FMT, ARGS...) __LOG(RINA_PREFIX, KERN_ALERT,      \
                                      "ALRT", FMT, ##ARGS)
#define LOG_CRIT(FMT,  ARGS...) __LOG(RINA_PREFIX, KERN_CRIT,       \
                                      "CRIT", FMT, ##ARGS)
#define LOG_ERR(FMT,   ARGS...) __LOG(RINA_PREFIX, KERN_ERR,        \
                                      "ERR",  FMT, ##ARGS)
#define LOG_WARN(FMT,  ARGS...) __LOG(RINA_PREFIX, KERN_WARNING,    \
                                      "WARN", FMT, ##ARGS)
#define LOG_NOTE(FMT,  ARGS...) __LOG(RINA_PREFIX, KERN_NOTICE,     \
                                      "NOTE", FMT, ##ARGS)
#define LOG_INFO(FMT,  ARGS...) __LOG(RINA_PREFIX, KERN_INFO,       \
                                      "INFO", FMT, ##ARGS)

#ifdef CONFIG_RINA_DEBUG
#ifdef CONFIG_RINA_SUPPRESS_DEBUG_LOGS
#warning Debugging logs WILL BE suppressed
#define LOG_DBG(FMT,   ARGS...) do { } while (0)
#else
#define LOG_DBG(FMT,   ARGS...) __LOG(RINA_PREFIX, KERN_DEBUG,      \
                                      "DBG", FMT, ##ARGS)
#endif
#else
#define LOG_DBG(FMT,   ARGS...) do { } while (0)
#endif

/* Helpers */
#define LOG_DBGF(FMT,  ARGS...) LOG_DBG("(%s: " FMT, __FUNCTION__, ##ARGS)
#define LOG_ERRF(FMT,  ARGS...) LOG_ERR("(%s: " FMT, __FUNCTION__, ##ARGS)

#ifdef CONFIG_RINA_DEBUG_HEARTBEATS
#define LOG_HBEAT LOG_DBG("I'm in %s (%s:%d)",                  \
                          __FUNCTION__, __FILE__, __LINE__)
#else
#define LOG_HBEAT do { } while (0)
#endif

#define LOG_OBSOLETE    LOG_ERR("Code in %s:%d is obsolete and "        \
                                "it will be removed soon, "             \
                                "DO NOT USE!!!",                        \
                                __FILE__, __LINE__)
#define LOG_MISSING     LOG_ERR("Missing code in %s:%d",        \
                                __FILE__, __LINE__)
#define LOG_UNSUPPORTED LOG_WARN("Unsupported feature in %s:%d",        \
                                 __FILE__, __LINE__)

#endif

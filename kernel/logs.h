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

#include <linux/interrupt.h>
#include <linux/kernel.h>

#define LOG_VERB_EMERG   1
#define LOG_VERB_ALERT   2
#define LOG_VERB_CRIT    3
#define LOG_VERB_ERR     4
#define LOG_VERB_WARN    5
#define LOG_VERB_NOTE    6
#define LOG_VERB_INFO    7
#define LOG_VERB_DBG     8

extern int irati_verbosity;

/* The global logs prefix */
#define __GPFX "rina-"

#define __LOG(PFX, LVL, BAN, FMT, ARGS...)                                   \
        do { printk(LVL __GPFX PFX "(" BAN "): " FMT "\n", ##ARGS); } while (0)

/* Sorted by "urgency" (high to low) */
#define LOG_EMERG(FMT, ARGS...) __LOG(RINA_PREFIX, KERN_EMERG,      \
                                      "EMER", FMT, ##ARGS)
#define LOG_ALERT(FMT, ARGS...) \
	if (irati_verbosity >= LOG_VERB_ALERT) \
		 __LOG(RINA_PREFIX, KERN_ALERT,      \
                       "ALRT", FMT, ##ARGS)

#define LOG_CRIT(FMT,  ARGS...) \
	if (irati_verbosity >= LOG_VERB_CRIT) \
		 __LOG(RINA_PREFIX, KERN_CRIT,       \
                       "CRIT", FMT, ##ARGS)

#define LOG_ERR(FMT,   ARGS...) \
	if (irati_verbosity >= LOG_VERB_ERR) \
		__LOG(RINA_PREFIX, KERN_ERR,        \
                      "ERR",  FMT, ##ARGS)

#define LOG_WARN(FMT,  ARGS...) \
	if (irati_verbosity >= LOG_VERB_WARN) \
		__LOG(RINA_PREFIX, KERN_WARNING,    \
                      "WARN", FMT, ##ARGS)

#define LOG_NOTE(FMT,  ARGS...) \
	if (irati_verbosity >= LOG_VERB_NOTE) \
		 __LOG(RINA_PREFIX, KERN_NOTICE,     \
                       "NOTE", FMT, ##ARGS)

#define LOG_INFO(FMT,  ARGS...) \
	if (irati_verbosity >= LOG_VERB_INFO) \
		__LOG(RINA_PREFIX, KERN_INFO,       \
                      "INFO", FMT, ##ARGS)

#define LOG_DBG(FMT,   ARGS...) \
	if (irati_verbosity >= LOG_VERB_DBG) \
		__LOG(RINA_PREFIX, KERN_DEBUG,      \
                      "DBG", FMT, ##ARGS)

/* Helpers */
#define LOG_DBGF(FMT,  ARGS...) LOG_DBG("(%s: " FMT, __FUNCTION__, ##ARGS)
#define LOG_ERRF(FMT,  ARGS...) LOG_ERR("(%s: " FMT, __FUNCTION__, ##ARGS)

#define LOG_OBSOLETE    LOG_ERR("Code in %s:%d is obsolete and "        \
                                "it will be removed soon, "             \
                                "DO NOT USE!!!",                        \
                                __FILE__, __LINE__)
#define LOG_MISSING     LOG_ERR("Missing code in %s:%d",        \
                                __FILE__, __LINE__)
#define LOG_UNSUPPORTED LOG_WARN("Unsupported feature in %s:%d",        \
                                 __FILE__, __LINE__)

#endif

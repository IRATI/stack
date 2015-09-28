/*
 * TCP-like RED Congestion avoidance policy set
 *
 *    Leonardo Bergesio <leonardo.bergesio@i2cat.net>
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
#include <linux/module.h>
#include <linux/string.h>

#define RINA_PREFIX "red-plugin"
#define RINA_PS_RED_NAME "red-ps"

#include "logs.h"
#include "rds/rmem.h"
#include "rmt-ps.h"
#include "dtcp-ps.h"
#include "dtcp-ps-debug.h"
#include "rmt-ps-debug.h"

extern struct ps_factory rmt_factory;
extern struct ps_factory dtcp_factory;

static int __init mod_init(void)
{
        int ret;

        strcpy(rmt_factory.name,  RINA_PS_RED_NAME);
        strcpy(dtcp_factory.name, RINA_PS_RED_NAME);

        ret = rmt_ps_publish(&rmt_factory);
        if (ret) {
                LOG_ERR("Failed to publish RMT policy set factory");
                return -1;
        }

        LOG_INFO("RMT TCP-RED policy set loaded successfully");

        ret = dtcp_ps_publish(&dtcp_factory);
        if (ret) {
                LOG_ERR("Failed to publish DTCP policy set factory");
                return -1;
        }
#if DTCP_DEBUG
	red_dtcp_debug_proc_init();
#endif
#if RMT_DEBUG
	red_rmt_debug_proc_init();
#endif
        LOG_INFO("DTCP TCP-RED policy set loaded successfully");

        return 0;
}

static void __exit mod_exit(void)
{
        int ret;

#if DTCP_DEBUG
	red_dtcp_debug_proc_exit();
#endif
#if RMT_DEBUG
	red_rmt_debug_proc_exit();
#endif

        ret = rmt_ps_unpublish(RINA_PS_RED_NAME);
        if (ret) {
                LOG_ERR("Failed to unpublish RMT policy set factory");
                return;
        }

        LOG_INFO("RMT TCP-RED policy set unloaded successfully");

        ret = dtcp_ps_unpublish(RINA_PS_RED_NAME);
        if (ret) {
                LOG_ERR("Failed to unpublish DTCP policy set factory");
                return;
        }

        LOG_INFO("DTCP TCP-RED policy set unloaded successfully");

}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("TCP-like RED policy sets");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Leonardo Bergesio <leonardo.bergesio@i2cat.net>");

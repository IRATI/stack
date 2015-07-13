/*
 * Dummy Kernel plugin with whole set of policy sets (RMT, PFT, DTP, DTCP)
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

#define RINA_PREFIX "dummy-plugin"
#define RINA_DUMMY_PS_NAME "dummy-ps"

#include "logs.h"
#include "rds/rmem.h"
#include "rmt-ps.h"
#include "dtp-ps.h"
#include "dtcp-ps.h"
#include "pff-ps.h"

extern struct ps_factory rmt_factory;
extern struct ps_factory dtp_factory;
extern struct ps_factory dtcp_factory;
extern struct ps_factory pff_factory;

static int __init mod_init(void)
{
        int ret;

        strcpy(rmt_factory.name,  RINA_DUMMY_PS_NAME);
        strcpy(dtp_factory.name,  RINA_DUMMY_PS_NAME);
        strcpy(dtcp_factory.name, RINA_DUMMY_PS_NAME);
        strcpy(pff_factory.name,  RINA_DUMMY_PS_NAME);

        ret = rmt_ps_publish(&rmt_factory);
        if (ret) {
                LOG_ERR("Failed to publish RMT policy set factory");
                return -1;
        }

        LOG_INFO("RMT dummy policy set loaded successfully");

        ret = dtp_ps_publish(&dtp_factory);
        if (ret) {
                LOG_ERR("Failed to publish DTP policy set factory");
                return -1;
        }

        LOG_INFO("DTP dummy policy set loaded successfully");

        ret = dtcp_ps_publish(&dtcp_factory);
        if (ret) {
                LOG_ERR("Failed to publish DTCP policy set factory");
                return -1;
        }

        LOG_INFO("DTCP dummy policy set loaded successfully");

        ret = pff_ps_publish(&pff_factory);
        if (ret) {
                LOG_ERR("Failed to publish PFT policy set factory");
                return -1;
        }

        LOG_INFO("PFT dummy policy set loaded successfully");

        return 0;
}

static void __exit mod_exit(void)
{
        int ret;

        ret = rmt_ps_unpublish(RINA_DUMMY_PS_NAME);
        if (ret) {
                LOG_ERR("Failed to unpublish Dummy RMT policy set factory");
                return;
        }

        LOG_INFO("Dummy RMT dummy policy set unloaded successfully");

        ret = dtp_ps_unpublish(RINA_DUMMY_PS_NAME);
        if (ret) {
                LOG_ERR("Failed to unpublish Dummy DTP policy set factory");
                return;
        }

        LOG_INFO("Dummy DTP dummy policy set unloaded successfully");

        ret = dtcp_ps_unpublish(RINA_DUMMY_PS_NAME);
        if (ret) {
                LOG_ERR("Failed to unpublish Dummy DTCP policy set factory");
                return;
        }

        LOG_INFO("Dummy DTCP dummy policy set unloaded successfully");

        ret = pff_ps_unpublish(RINA_DUMMY_PS_NAME);
        if (ret) {
                LOG_ERR("Failed to unpublish Dummy PFT policy set factory");
                return;
        }

        LOG_INFO("Dummy PFT dummy policy set unloaded successfully");
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_AUTHOR("Leonardo Bergesio <leonardo.bergesio@i2cat.net>");
MODULE_LICENSE ("GPL");
MODULE_DESCRIPTION("Dummy policy sets");


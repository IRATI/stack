/*
 * Default plugins for kernel components
 *
 *    Vincenzo Maffione <v.maffione@nextworks.it>
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
#include <linux/module.h>
#include <linux/string.h>

#define RINA_PREFIX "rina-default-plugin"

#include "logs.h"
#include "rds/rmem.h"
#include "rmt-ps.h"
#include "dtp-ps.h"
#include "dtcp-ps.h"
#include "pff-ps.h"
#include "sdup-ps.h"

extern struct ps_factory default_rmt_ps_factory;
extern struct ps_factory default_dtp_ps_factory;
extern struct ps_factory default_dtcp_ps_factory;
extern struct ps_factory default_pff_ps_factory;
extern struct ps_factory default_sdup_ps_factory;

static int __init mod_init(void)
{
        int ret;

        strcpy(default_rmt_ps_factory.name, RINA_PS_DEFAULT_NAME);
        strcpy(default_dtp_ps_factory.name, RINA_PS_DEFAULT_NAME);
        strcpy(default_dtcp_ps_factory.name, RINA_PS_DEFAULT_NAME);
        strcpy(default_pff_ps_factory.name, RINA_PS_DEFAULT_NAME);
        strcpy(default_sdup_ps_factory.name, RINA_PS_DEFAULT_NAME);

        ret = rmt_ps_publish(&default_rmt_ps_factory);
        if (ret) {
                LOG_ERR("Failed to publish RMT policy set factory");
                return -1;
        }

        LOG_INFO("RMT default policy set loaded successfully");

        ret = dtp_ps_publish(&default_dtp_ps_factory);
        if (ret) {
                LOG_ERR("Failed to publish DTP policy set factory");
                return -1;
        }

        LOG_INFO("DTP default policy set loaded successfully");

        ret = dtcp_ps_publish(&default_dtcp_ps_factory);
        if (ret) {
                LOG_ERR("Failed to publish DTCP policy set factory");
                return -1;
        }

        LOG_INFO("DTCP default policy set loaded successfully");

        ret = pff_ps_publish(&default_pff_ps_factory);
        if (ret) {
                LOG_ERR("Failed to publish PFF policy set factory");
                return -1;
        }

        LOG_INFO("PFF default policy set loaded successfully");

        ret = sdup_ps_publish(&default_sdup_ps_factory);
        if (ret) {
                LOG_ERR("Failed to publish SDU Protection policy set factory");
                return -1;
        }

        LOG_INFO("SDU Protection default policy set loaded successfully");

        return 0;
}

static void __exit mod_exit(void)
{
        int ret;

        ret = sdup_ps_unpublish(RINA_PS_DEFAULT_NAME);
        if (ret) {
                LOG_ERR("Failed to unpublish SDU Protection policy set factory");
                return;
        }

        ret = rmt_ps_unpublish(RINA_PS_DEFAULT_NAME);
        if (ret) {
                LOG_ERR("Failed to unpublish RMT policy set factory");
                return;
        }

        LOG_INFO("RMT default policy set unloaded successfully");

        ret = dtp_ps_unpublish(RINA_PS_DEFAULT_NAME);
        if (ret) {
                LOG_ERR("Failed to unpublish DTP policy set factory");
                return;
        }

        LOG_INFO("DTP default policy set unloaded successfully");

        ret = dtcp_ps_unpublish(RINA_PS_DEFAULT_NAME);
        if (ret) {
                LOG_ERR("Failed to unpublish DTCP policy set factory");
                return;
        }

        LOG_INFO("DTCP default policy set unloaded successfully");

        ret = pff_ps_unpublish(RINA_PS_DEFAULT_NAME);
        if (ret) {
                LOG_ERR("Failed to unpublish PFF policy set factory");
                return;
        }

        LOG_INFO("PFF default policy set unloaded successfully");
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("Default policy sets");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Vincenzo Maffione <v.maffione@nextworks.it>");
MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini@nextworks.it>");

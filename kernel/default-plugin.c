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
#include "sdup-crypto-ps.h"
#include "sdup-errc-ps.h"
#include "sdup-ttl-ps.h"
#include "rmt-ps-default.h"
#include "dtp-ps-default.h"
#include "dtcp-ps-default.h"
#include "pff-ps-default.h"
#include "sdup-crypto-ps-default.h"
#include "sdup-errc-ps-default.h"
#include "sdup-ttl-ps-default.h"
#include "delim-ps-default.h"

struct ps_factory default_rmt_ps_factory = {
	.owner = THIS_MODULE,
	.create = rmt_ps_default_create,
	.destroy = rmt_ps_default_destroy,
};

struct ps_factory default_dtp_ps_factory = {
	.owner = THIS_MODULE,
	.create = dtp_ps_default_create,
	.destroy = dtp_ps_default_destroy,
};

struct ps_factory default_dtcp_ps_factory = {
	.owner = THIS_MODULE,
	.create = dtcp_ps_default_create,
	.destroy = dtcp_ps_default_destroy,
};

struct ps_factory default_delim_ps_factory = {
	.owner = THIS_MODULE,
	.create = delim_ps_default_create,
	.destroy = delim_ps_default_destroy,
};

struct ps_factory default_pff_ps_factory = {
	.owner = THIS_MODULE,
	.create = pff_ps_default_create,
	.destroy = pff_ps_default_destroy,
};

struct ps_factory default_sdup_crypto_ps_factory = {
	.owner   = THIS_MODULE,
	.create  = sdup_crypto_ps_default_create,
	.destroy = sdup_crypto_ps_default_destroy,
};

struct ps_factory default_sdup_errc_ps_factory = {
	.owner   = THIS_MODULE,
	.create  = sdup_errc_ps_default_create,
	.destroy = sdup_errc_ps_default_destroy,
};

struct ps_factory default_sdup_ttl_ps_factory = {
	.owner   = THIS_MODULE,
	.create  = sdup_ttl_ps_default_create,
	.destroy = sdup_ttl_ps_default_destroy,
};

static int __init mod_init(void)
{
        int ret;

        strcpy(default_rmt_ps_factory.name, RINA_PS_DEFAULT_NAME);
        strcpy(default_dtp_ps_factory.name, RINA_PS_DEFAULT_NAME);
        strcpy(default_dtcp_ps_factory.name, RINA_PS_DEFAULT_NAME);
        strcpy(default_delim_ps_factory.name, RINA_PS_DEFAULT_NAME);
        strcpy(default_pff_ps_factory.name, RINA_PS_DEFAULT_NAME);
        strcpy(default_sdup_crypto_ps_factory.name, RINA_PS_DEFAULT_NAME);
        strcpy(default_sdup_errc_ps_factory.name, CRC32);
        strcpy(default_sdup_ttl_ps_factory.name, RINA_PS_DEFAULT_NAME);

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

        ret = delim_ps_publish(&default_delim_ps_factory);
        if (ret) {
                LOG_ERR("Failed to publish Delimiting policy set factory");
                return -1;
        }

        LOG_INFO("Delimiting default policy set loaded successfully");

        ret = pff_ps_publish(&default_pff_ps_factory);
        if (ret) {
                LOG_ERR("Failed to publish PFF policy set factory");
                return -1;
        }

        LOG_INFO("PFF default policy set loaded successfully");

        ret = sdup_crypto_ps_publish(&default_sdup_crypto_ps_factory);
        if (ret) {
                LOG_ERR("Failed to publish SDU Protection Crypto policy set factory");
                return -1;
        }

        LOG_INFO("SDU Protection default Crypto policy set loaded successfully");

        ret = sdup_errc_ps_publish(&default_sdup_errc_ps_factory);
        if (ret) {
                LOG_ERR("Failed to publish SDU Protection error check policy set factory");
                return -1;
        }

        LOG_INFO("SDU Protection default error check policy set loaded successfully");

        ret = sdup_ttl_ps_publish(&default_sdup_ttl_ps_factory);
        if (ret) {
                LOG_ERR("Failed to publish SDU Protection TTL policy set factory");
                return -1;
        }

        LOG_INFO("SDU Protection default TTL policy set loaded successfully");

        return 0;
}

static void __exit mod_exit(void)
{
        int ret;

        ret = sdup_crypto_ps_unpublish(RINA_PS_DEFAULT_NAME);
        if (ret) {
                LOG_ERR("Failed to unpublish SDU Protection Crypto policy set factory");
                return;
        }

        ret = sdup_errc_ps_unpublish(CRC32);
        if (ret) {
                LOG_ERR("Failed to unpublish SDU Protection error check policy set factory");
                return;
        }

        ret = sdup_ttl_ps_unpublish(RINA_PS_DEFAULT_NAME);
        if (ret) {
                LOG_ERR("Failed to unpublish SDU Protection TTL policy set factory");
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

        ret = delim_ps_unpublish(RINA_PS_DEFAULT_NAME);
        if (ret) {
                LOG_ERR("Failed to unpublish Delimiting policy set factory");
                return;
        }

        LOG_INFO("Delimiting default policy set unloaded successfully");

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
MODULE_AUTHOR("Miquel Tarzan <miquel.tarzan@i2cat.net>");
MODULE_AUTHOR("Leonardo Begesio <leonardo.bergesio@i2cat.net>");
MODULE_AUTHOR("Ondrej Lichtner <ilichtner@fit.vutbr.cz>");
MODULE_AUTHOR("Sander Vrijders <sander.vrijders@intec.ugent.be>");

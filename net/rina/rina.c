/*
 * RINA personality
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Miquel Tarzan <miquel.tarzan@i2cat.net>
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

#include <linux/module.h>

#define RINA_PREFIX "personality-default"

#include "logs.h"
#include "kipcm.h"
#include "efcp.h"
#include "rmt.h"
#include "shim-eth.h"
#include "shim-tcp-udp.h"

static int __init mod_init(void)
{
        LOG_FBEGN;

        LOG_DBG("Rina personality initializing");

        if (kipcm_init())
                return -1;

        if (efcp_init()) {
                kipcm_exit();
                return -1;
        }

        if (rmt_init()) {
                efcp_exit();
                kipcm_exit();
        }

#ifdef CONFIG_SHIM_ETH
        if (shim_eth_init()) {
                rmt_exit();
                efcp_exit();
                kipcm_exit();
        }
#endif
#ifdef CONFIG_SHIM_TCP_UDP
        if (shim_tcp_udp_init()) {
                shim_eth_exit();
                rmt_exit();
                efcp_exit();
                kipcm_exit();
        }
#endif

        LOG_DBG("Rina personality loaded successfully");

        LOG_FEXIT;
        return 0;
}

static void __exit mod_exit(void)
{
        LOG_FBEGN;

        LOG_DBG("Rina personality exiting");

#ifdef CONFIG_SHIM_TCP_UDP
        shim_tcp_udp_exit();
#endif
#ifdef CONFIG_SHIM_ETH
        shim_eth_exit();
#endif
        rmt_exit();
        efcp_exit();
        kipcm_exit();

        LOG_DBG("Rina personality unloaded successfully");

        LOG_FEXIT;
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("RINA default personality");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini@nextworks.it>");
MODULE_AUTHOR("Leonardo Bergesio <leonardo.bergesio@i2cat.net>");
MODULE_AUTHOR("Miquel Tarzan <miquel.tarzan@i2cat.net>");
MODULE_AUTHOR("Sander Vrijders <sander.vrijders@intec.ugent.be>");

/*
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

#define RINA_PREFIX "rina"

#include "logs.h"
#include "rina.h"
#include "ipcm.h"
#include "efcp.h"
#include "rmt.h"
#include "shim-eth.h"
#include "shim-tcp-udp.h"

MODULE_DESCRIPTION("RINA stack");
MODULE_LICENSE("GPLv2");
MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini>");

static __init int rina_init(void)
{
        LOG_INFO("RINA stack v" RINA_VERSION_STRING " initializing");

        ipcm_init();
        efcp_init();
        rmt_init();
#ifdef CONFIG_SHIM_ETH
        shim_eth_init();
#endif
#ifdef CONFIG_SHIM_TCP_UDP
        shim_tcp_udp_init();
#endif
        return 0;
}

static __exit void rina_exit(void)
{
#ifdef CONFIG_SHIM_TCP_UDP
        shim_tcp_udp_exit();
#endif
#ifdef CONFIG_SHIM_ETH
        shim_eth_exit();
#endif
        rmt_exit();
        efcp_exit();
        ipcm_exit();

        LOG_INFO("exit");
}

module_init(rina_init);
module_exit(rina_exit);

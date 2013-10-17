/*
 * ARP 826 (wonnabe) core
 *
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
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

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/if_ether.h>

/* FIXME: The following dependencies have to be removed */
#define RINA_PREFIX "arp826-core"

#include "logs.h"
#include "debug.h"
#include "utils.h"
/* FIXME: End of dependencies ... */

#include "arp826.h"
#include "arp826-utils.h"
#include "arp826-rxtx.h"
#include "arp826-arm.h"
#include "arp826-tables.h"

static struct packet_type arp826_packet_type __read_mostly = {
        .type = cpu_to_be16(ETH_P_ARP),
        .func = arp_receive,
};

static int protocol_add(uint16_t ptype,
                        size_t   hlen)
{
        LOG_DBG("Adding protocol 0x%02x, hlen = %zd", ptype, hlen);
      
        if (tbls_create(ptype, hlen)) {
                LOG_ERR("Cannot add protocol 0x%02x, hlen = %zd", ptype, hlen);
                return -1;
        }

        LOG_DBG("Protocol type 0x%02x added successfully", ptype);

        return 0;
}

static void protocol_remove(uint16_t ptype)
{
        LOG_DBG("Removing protocol 0x%02x", ptype);

        tbls_destroy(ptype);
}

#define CONFIG_ARP826_REGRESSION_TESTS

static int regression_tests(void)
{
        struct gpa * a;
        uint8_t      name_tmp[] = { 0x01, 0x02, 0x03, 0x04 };
        struct gpa * b;
        size_t       len_a_1, len_a_2;
        size_t       len_b_1, len_b_2;

        LOG_DBG("Regression test #1.1");
        a = gpa_create(name_tmp, sizeof(name_tmp));
        if (!a)
                return -1;
        len_a_1 = gpa_address_length(a);

        LOG_DBG("Regression test #1.2");
        b = gpa_create(name_tmp, sizeof(name_tmp));
        if (!b)
                return -1;
        len_b_1 = gpa_address_length(b);

        LOG_DBG("Regression test #2");
        if (!gpa_is_equal(a, b))
                return -1;
        if (len_a_1 != len_b_1)
                return -1;

        LOG_DBG("Regression test #3.1");
        if (gpa_address_grow(a, sizeof(name_tmp) * 2, 0xff))
                return -1;
        len_a_2 = gpa_address_length(a);

        if (gpa_address_grow(b, sizeof(name_tmp) * 2, 0xff))
                return -1;
        len_b_2 = gpa_address_length(b);

        if (!gpa_is_equal(a, b))
                return -1;

        LOG_DBG("Regression test #3.2");
        if (len_a_2 != len_b_2)
                return -1;

        LOG_DBG("Regression test #3.3");
        if ((len_a_1 == len_a_2) ||
            (len_b_1 == len_b_2))
                return -1;

        LOG_DBG("Regression test #4.1");
        if (gpa_address_shrink(a, 0xff))
                return -1;
        if (gpa_address_shrink(b, 0xff))
                return -1;

        LOG_DBG("Regression test #4.2");
        if (gpa_address_length(a) != len_a_1)
                return -1;
        LOG_DBG("Regression test #4.3");
        if (gpa_address_length(b) != len_b_1)
                return -1;

        gpa_destroy(b);
        gpa_destroy(a);

        return 0;
}

static int __init mod_init(void)
{
        LOG_DBG("Initializing");

        if (tbls_init())
                return -1;

        if (arm_init()) {
                tbls_fini();
                return -1;
        }

#ifdef CONFIG_ARP826_REGRESSION_TESTS
        if (regression_tests()) {
                LOG_ERR("Regression tests do not pass, bailing out");
                return -1;
        }
#endif
        if (protocol_add(ETH_P_RINA, 6)) {
                tbls_fini();
                arm_fini();
                return -1;
        }

	dev_add_pack(&arp826_packet_type);

        LOG_DBG("Initialized successfully");

        return 0;
}

static void __exit mod_exit(void)
{
        LOG_DBG("Finalizing");

	dev_remove_pack(&arp826_packet_type);

        protocol_remove(ETH_P_RINA);

        arm_fini();
        tbls_fini();

        LOG_DBG("Finalized successfully");
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("Basic RFC 826 compliant ARP implementation");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini@nextworks.it>");
MODULE_AUTHOR("Sander Vrijders <sander.vrijders@intec.ugent.be>");

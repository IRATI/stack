/*
 * Common policy set for PFT
 *
 *    Miquel Tarzan <miquel.tarzan@i2cat.net>
 *    Leonardo Bergesio <leonardo.bergesio@i2cat.net>
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
#include <linux/random.h>

#define RINA_PREFIX "pft-ps-common"

#include "logs.h"
#include "rds/rmem.h"
#include "rds/rtimer.h"
#include "pft-ps-common.h"
#include "debug.h"

int common_next_hop(struct pft_ps * ps,
                    struct pci *    pci,
                    port_id_t **    ports,
                    size_t *        count)
{
        struct pft *       pft = ps->dm;
        address_t          destination;
        qos_id_t           qos_id;
        struct pft_entry * tmp;

        if (!pft_is_ok(pft))
                return -1;

        destination = pci_destination(pci);
        if (!is_address_ok(destination)) {
                LOG_ERR("Bogus destination address, cannot get NHOP");
                return -1;
        }
        qos_id = pci_qos_id(pci);
        if (!is_qos_id_ok(qos_id)) {
                LOG_ERR("Bogus qos-id, cannot get NHOP");
                return -1;
        }
        if (!ports || !count) {
                LOG_ERR("Bogus output parameters, won't get NHOP");
                return -1;
        }

        /*
         * Taking the lock here since otherwise instance might be deleted when
         * copying the ports
         */
        rcu_read_lock();

        tmp = pft_find(pft, destination, qos_id);
        if (!tmp) {
                LOG_ERR("Could not find any entry for dest address: %u and "
                        "qos_id %d", destination, qos_id);
                rcu_read_unlock();
                return -1;
        }

        if (pfte_ports_copy(tmp, ports, count)) {
                rcu_read_unlock();
                return -1;
        }

        rcu_read_unlock();

        return 0;
}
EXPORT_SYMBOL(common_next_hop);

int pft_ps_common_set_policy_set_param(struct ps_base * bps,
                                       const char *     name,
                                       const char *     value)
{
        struct pft_ps *ps = container_of(bps, struct pft_ps, base);

        (void) ps;

        if (!name) {
                LOG_ERR("Null parameter name");
                return -1;
        }

        if (!value) {
                LOG_ERR("Null parameter value");
                return -1;
        }

        LOG_ERR("No such parameter to set");

        return -1;
}
EXPORT_SYMBOL(pft_ps_common_set_policy_set_param);

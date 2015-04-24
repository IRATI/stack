/*
 * Default policy set for PFT
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
#include <linux/random.h>

#define RINA_PREFIX "pft-ps-default"

#include "logs.h"
#include "rds/rmem.h"
#include "rds/rtimer.h"
#include "pft-ps.h"
#include "debug.h"

static int default_next_hop(struct pft_ps * ps,
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

static int pft_ps_set_policy_set_param(struct ps_base * bps,
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

static struct ps_base *
pft_ps_default_create(struct rina_component * component)
{
        struct pft * dtp = pft_from_component(component);
        struct pft_ps * ps = rkzalloc(sizeof(*ps), GFP_KERNEL);

        if (!ps) {
                return NULL;
        }

        ps->base.set_policy_set_param = pft_ps_set_policy_set_param;
        ps->dm              = dtp;
        ps->priv            = NULL;

        ps->next_hop = default_next_hop;

        return &ps->base;
}

static void pft_ps_default_destroy(struct ps_base * bps)
{
        struct pft_ps *ps = container_of(bps, struct pft_ps, base);

        if (bps) {
                rkfree(ps);
        }
}

struct ps_factory pft_factory = {
        .owner          = THIS_MODULE,
        .create  = pft_ps_default_create,
        .destroy = pft_ps_default_destroy,
};

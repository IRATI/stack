/*
 * Default policy set for RMT
 *
 *    Vincenzo Maffione <v.maffione@nextworks.it>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
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

#define RINA_PREFIX "rmt-ps-default"

#include "logs.h"
#include "rds/rmem.h"
#include "rmt-ps.h"
#include "rmt.h"
#include <linux/interrupt.h>

/* Used for the old round roubin implementation */
struct sched_substate {
        int last_bucket;
        struct rfifo * last_queue;
};

struct sched_state {
        struct sched_substate rx;
        struct sched_substate tx;
};
/* */

struct rmt_ps_data {
        struct sched_state *   ss;
        struct rmt_queue_set * inqs;
        struct rmt_queue_set * outqs;
        struct tasklet_struct  ingress_tasklet;
        struct tasklet_struct  egress_tasklet;
};

static void
default_max_q_policy_tx(struct rmt_ps * ps,
                        struct pdu *    pdu,
                        struct rfifo *  queue)
{ }

static void
default_max_q_policy_rx(struct rmt_ps * ps,
                        struct sdu *    sdu,
                        struct rfifo *  queue)
{ }

static void
default_rmt_q_monitor_policy_tx(struct rmt_ps *      ps,
                                struct pdu *         pdu,
                                struct rmt_n1_port * n1_port)
{ }

static void
default_rmt_q_monitor_policy_rx(struct rmt_ps *      ps,
                                struct sdu *         sdu,
                                struct rmt_n1_port * n1_port)
{ }

static int
default_rmt_scheduling_create_policy_common(struct rmt_ps *        ps,
                                            struct rmt_n1_port *   n1_port,
                                            struct rmt_queue_set * qs)
{
        struct rmt_qgroup * qgroup;
        struct rmt_kqueue * kqueue;

        qgroup = rmt_qgroup_create();
        if (!qgroup) {
                LOG_ERR("Could not create queues group struct for n1_port %u",
                        n1_port->port_id);
                return -1;
        }
        hash_add(qs->qgroups, &qgroup->hlist, n1_port->port_id);

        kqueue = rmt_kqueue_create(0);
        if (!kqueue) {
                LOG_ERR("Could not create key-queue struct for n1_port %u",
                        n1_port->port_id);
                return -1;
        }
        hash_add(qgroup->queues, &kqueue->hlist, 0);

        return 0;
}

static int
default_rmt_scheduling_create_policy_tx(struct rmt_ps * ps,
                                        struct rmt_n1_port * n1_port)
{
        struct rmt_ps_data * data;
        data = ps->priv;
        return default_rmt_scheduling_create_policy_common(ps,
                                                           n1_port,
                                                           data->outqs);
}

static int
default_rmt_scheduling_create_policy_rx(struct rmt_ps * ps,
                                        struct rmt_n1_port * n1_port)
{
        struct rmt_ps_data * data;
        data = ps->priv;
        return default_rmt_scheduling_create_policy_common(ps,
                                                           n1_port,
                                                           data->inqs);
}

/* NOTE: To be used when rmt_n1_port struct has several queues */
/* Round robin scheduler. */
/*
static struct rmt_queue *
rmt_scheduling_policy_common(struct sched_substate * sss,
                             struct rmt_qmap * qmap, bool restart)
{
        struct rmt_queue *queue;
        struct rmt_queue *selected_queue = NULL;
        int bkt;

        if (restart) {
                sss->last_bucket = 0;
                sss->last_queue = NULL;
        }

        LOG_DBG("SCHED-START: lb = %d, lq = %p",
                sss->last_bucket, sss->last_queue);

        for (bkt = sss->last_bucket, queue = NULL;
                        queue == NULL && bkt < HASH_SIZE(qmap->queues); bkt++) {
                if (sss->last_queue) {
                        queue = sss->last_queue;
                        hlist_for_each_entry_continue(queue, hlist) {
                                selected_queue = queue;
                                break;
                        }
                } else {
                        hlist_for_each_entry(queue, &qmap->queues[bkt], hlist) {
                                selected_queue = queue;
                                break;
                        }
                }
                if (selected_queue) {
                        break;
                } else {
                        // When the bucket changes we must invalidate
                        // sss->last_queue.
                        sss->last_queue = NULL;
                }
        }

        sss->last_bucket = bkt;
        sss->last_queue = selected_queue;

        LOG_DBG("SCHED-END: lb = %d, lq = %p",
                sss->last_bucket, sss->last_queue);

        return selected_queue;
}
*/

static void send_worker(unsigned long o)
{ }
/*
{
        struct rmt *         rmt;
        struct rmt_n1_port * entry;
        struct hlist_node *  ntmp;
        int                  bucket;
        int                  pending = 0;
        struct rfifo *       queue;
        struct rmt_ps *      ps;
        bool                 sched_restart = true;

        LOG_DBG("Send worker called");

        ps = (struct rmt_ps *) o;
        if (!tmp) {
                LOG_ERR("No instance passed to send worker !!!");
                return;
        }

        rmt = ps->dm;
        ASSERT(rmt);

        spin_lock(&ps->data->egress.n1_ports->lock);
        hash_for_each_safe(tmp->egress.n1_ports->n1_ports,
                           bucket,
                           ntmp,
                           entry,
                           hlist) {
                struct pdu * pdu;

                ASSERT(entry);

                spin_unlock(&tmp->egress.n1_ports->lock);

                spin_lock(&entry->lock);
                if (entry->state == N1_PORT_STATE_DISABLED ||
                    atomic_read(&entry->n_sdus) == 0) {
                        if (atomic_read(&entry->n_sdus) > 0)
                                pending++;
                        spin_unlock(&entry->lock);
                        spin_lock(&tmp->egress.n1_ports->lock);
                        LOG_DBG("Port state is DISABLED or empty");
                        continue;
                }

                for (;;) {

                        queue = ps->rmt_scheduling_policy_tx(ps,
                                                             entry,
                                                             sched_restart);
                        if (!queue) {
                                break;
                        }
                        sched_restart = false;

                        pdu = (struct pdu *) rfifo_pop(queue);
                        if (!pdu) {
                                spin_unlock(&entry->lock);
                                LOG_DBG("No PDU to work in this queue ...");
                                break;
                        }
                        entry->state = N1_PORT_STATE_BUSY;
                        spin_unlock(&entry->lock);

                        n1_port_write(tmp->serdes, entry, pdu);

                        spin_lock(&entry->lock);
                        if (atomic_read(&entry->n_sdus) <= 0) {
                                atomic_set(&entry->n_sdus, 0);
                                if (entry->state != N1_PORT_STATE_DISABLED)
                                        entry->state = N1_PORT_STATE_ENABLED;
                        } else {
                                pending++;
                        }
                }
                spin_unlock(&entry->lock);
                spin_lock(&tmp->egress.n1_ports->lock);
        }
        spin_unlock(&tmp->egress.n1_ports->lock);

        if (pending) {
                LOG_DBG("RMT worker scheduling tasklet...");
                tasklet_hi_schedule(&tmp->egress.egress_tasklet);
        }

        return;
}
*/

static int
default_rmt_scheduling_policy_tx(struct rmt_ps *      ps,
                                 struct rmt_n1_port * n1_port,
                                 struct pdu *         pdu)
{
        unsigned long flags;
        struct rmt_kqueue * q;
        struct rmt_qgroup * qg;
        struct rmt_ps_data * data = ps->priv;

        /*FIXME: add checks */
        spin_lock_irqsave(&n1_port->lock, flags);
        qg = rmt_queue_set_find(data->outqs, n1_port->port_id);
        if (!qg) {
                LOG_ERR("Could not find queue group for n1_port %u",
                        n1_port->port_id);
                pdu_destroy(pdu);
                return -1;
        }
        q = rmt_qgroup_find(qg, 0);
        if (!qg) {
                LOG_ERR("Could not find queue in the group for n1_port %u",
                        n1_port->port_id);
                pdu_destroy(pdu);
                return -1;
        }
        atomic_inc(&n1_port->n_sdus);
        if (n1_port->state == N1_PORT_STATE_ENABLED &&
            /*FIXME check if queue length can be used instead */
            atomic_read(&n1_port->n_sdus) == 1) {
                int ret = 0;
                n1_port->state = N1_PORT_STATE_BUSY;
                spin_unlock_irqrestore(&n1_port->lock, flags);
                if (rmt_n1_port_write(ps->dm, n1_port, pdu))
                        ret = -1;

                spin_lock_irqsave(&n1_port->lock, flags);
                if (atomic_read(&n1_port->n_sdus) <= 0) {
                        atomic_set(&n1_port->n_sdus, 0);
                        if (n1_port->state != N1_PORT_STATE_DISABLED) {
                                LOG_DBG("Sent and enabling port");
                                n1_port->state = N1_PORT_STATE_ENABLED;
                        }
                } else {
                        LOG_DBG("Sent and scheduling cause there are more"
                                " pdus in the port");
                        tasklet_hi_schedule(&data->egress_tasklet);
                }
                spin_unlock_irqrestore(&n1_port->lock, flags);
                return ret;
        } else if (n1_port->state == N1_PORT_STATE_BUSY) {
                if (rfifo_push_ni(q->queue, pdu)) {
                        spin_unlock_irqrestore(&n1_port->lock, flags);
                        pdu_destroy(pdu);
                        return -1;
                }
                LOG_DBG("Port was busy, enqueuing PDU..");
        } else {
                LOG_ERR("Port state deallocated, discarding PDU");
                pdu_destroy(pdu);
                return -1;
        }

        spin_unlock_irqrestore(&n1_port->lock, flags);

        return 0;
}

static void receive_worker(unsigned long o)
{ }
/*
{
        struct rmt *         tmp;
        struct rfifo *       queue;
        struct rmt_ps *      ps;
        bool                 sched_restart = true;
        struct rmt_n1_port * entry;
        int                  bucket;
        struct hlist_node *  ntmp;

        port_id_t            port_id;
        pdu_type_t           pdu_type;
        const struct pci *   pci;
        struct pdu_ser *     pdu_ser;
        struct pdu *         pdu;
        address_t            dst_addr;
        qos_id_t             qos_id;
        struct serdes *      serdes;
        struct buffer *      buf;
        struct sdu *         sdu;

        LOG_DBG("RMT tasklet receive worker called");

        tmp = (struct rmt *) o;
        if (!tmp) {
                LOG_ERR("No instance passed to receive worker !!!");
                return;
        }

        rcu_read_lock();
        ps = container_of(rcu_dereference(tmp->base.ps),
                          struct rmt_ps,
                          base);
        if (!ps) {
                LOG_ERR("No ps in the receive_worker");
                rcu_read_unlock();
                return;
        }
        rcu_read_unlock();

        spin_lock(&tmp->ingress.n1_ports->lock);
        hash_for_each_safe(tmp->ingress.n1_ports->n1_ports,
                           bucket,
                           ntmp,
                           entry,
                           hlist) {
                ASSERT(entry);

                for (;;) {
                        queue = ps->rmt_scheduling_policy_rx(ps,
                                        entry, sched_restart);
                        if (!queue) {
                                // No more queues to serve.
                                break;
                        }
                        sched_restart = false;

                        sdu = (struct sdu *) rfifo_pop(queue);
                        if (!sdu) {
                                LOG_DBG("No SDU to work with in this queue");
                                break;
                        }

                        atomic_dec(&entry->n_sdus);

                        port_id = entry->port_id;
                        spin_unlock(&tmp->ingress.n1_ports->lock);

                        buf = sdu_buffer_rw(sdu);
                        if (!buf) {
                                LOG_DBG("No buffer present");
                                sdu_destroy(sdu);
                                spin_lock(&tmp->ingress.n1_ports->lock);
                                break;
                        }

                        if (sdu_buffer_disown(sdu)) {
                                LOG_DBG("Could not disown SDU");
                                sdu_destroy(sdu);
                                spin_lock(&tmp->ingress.n1_ports->lock);
                                break;
                        }

                        sdu_destroy(sdu);

                        pdu_ser = pdu_ser_create_buffer_with_ni(buf);
                        if (!pdu_ser) {
                                LOG_DBG("No ser PDU to work with");
                                buffer_destroy(buf);
                                spin_lock(&tmp->ingress.n1_ports->lock);
                                break;
                        }

                        serdes = tmp->serdes;
                        ASSERT(serdes);

                        pdu = pdu_deserialize_ni(serdes, pdu_ser);
                        if (!pdu) {
                                LOG_ERR("Failed to deserialize PDU!");
                                pdu_ser_destroy(pdu_ser);
                                spin_lock(&tmp->ingress.n1_ports->lock);
                                break;
                        }

                        pci = pdu_pci_get_ro(pdu);
                        if (!pci) {
                                LOG_ERR("No PCI to work with, dropping SDU!");
                                pdu_destroy(pdu);
                                spin_lock(&tmp->ingress.n1_ports->lock);
                                break;
                        }

                        ASSERT(pdu_is_ok(pdu));

                        pdu_type = pci_type(pci);
                        dst_addr = pci_destination(pci);
                        qos_id   = pci_qos_id(pci);
                        if (!pdu_type_is_ok(pdu_type) ||
                            !is_address_ok(dst_addr)  ||
                            !is_qos_id_ok(qos_id)) {
                                LOG_ERR("Wrong PDU type, dst address or qos_id,"
                                        " dropping SDU! %u, %u, %u",
                                        pdu_type, dst_addr, qos_id);
                                pdu_destroy(pdu);
                                spin_lock(&tmp->ingress.n1_ports->lock);
                                break;
                        }

                        // pdu is not for me
                        if (tmp->address != dst_addr) {
                                if (!dst_addr) {
                                        process_mgmt_pdu_ni(tmp, port_id, pdu);
                                } else {
                                        forward_pdu(tmp,
                                                    port_id,
                                                    dst_addr,
                                                    qos_id,
                                                    pdu);
                                }
                        } else {
                                // pdu is for me
                                switch (pdu_type) {
                                case PDU_TYPE_MGMT:
                                        process_mgmt_pdu_ni(tmp, port_id, pdu);
                                        break;

                                case PDU_TYPE_CACK:
                                case PDU_TYPE_SACK:
                                case PDU_TYPE_NACK:
                                case PDU_TYPE_FC:
                                case PDU_TYPE_ACK:
                                case PDU_TYPE_ACK_AND_FC:
                                case PDU_TYPE_DT:
                                        process_dt_pdu(tmp, port_id, pdu);
                                        LOG_DBG("Finishing  process_dt_sdu");
                                        break;

                                default:
                                        LOG_ERR("Unknown PDU type %d", pdu_type);
                                        pdu_destroy(pdu);
                                        break;
                                }
                        }
                        if (atomic_read(&entry->n_sdus) <= 0)
                                atomic_set(&entry->n_sdus, 0);
                        spin_lock(&tmp->ingress.n1_ports->lock);
                }
        }

        spin_unlock(&tmp->ingress.n1_ports->lock);
        tasklet_hi_schedule(&tmp->ingress.ingress_tasklet);

        return;
}
*/

static int
default_rmt_scheduling_policy_rx(struct rmt_ps *      ps,
                                 struct rmt_n1_port * n1_port,
                                 struct sdu *         sdu)
{
        struct rmt_ps_data * data = ps->priv;
/*
        atomic_inc(&in_n1_port->n_sdus);

        tasklet_hi_schedule(&instance->ingress.ingress_tasklet);

        LOG_DBG("RMT tasklet scheduled");
*/
        return 0;
}

static int
rmt_ps_set_policy_set_param(struct ps_base * bps,
                            const char    * name,
                            const char    * value)
{
        struct rmt_ps *ps = container_of(bps, struct rmt_ps, base);

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
rmt_ps_default_create(struct rina_component * component)
{
        struct rmt * rmt = rmt_from_component(component);
        struct rmt_ps * ps = rkzalloc(sizeof(*ps), GFP_KERNEL);
        struct rmt_ps_data * data = rkzalloc(sizeof(*data), GFP_KERNEL);

        if (!ps || !data) {
                return NULL;
        }

        /* Allocate policy-set private data. */
        data->ss = rkzalloc(sizeof(*data->ss), GFP_KERNEL);
        if (!data->ss) {
                rkfree(ps);
                rkfree(data);
                return NULL;
        }
        data->inqs = rmt_queue_set_create();
        if (!data->inqs) {
                rkfree(data->ss);
                rkfree(ps);
                rkfree(data);
                return NULL;
        }
        data->outqs = rmt_queue_set_create();
        if (!data->outqs) {
                rkfree(data->ss);
                rmt_queue_set_destroy(data->inqs);
                rkfree(ps);
                rkfree(data);
                return NULL;
        }
        ps->priv = data;

        ps->base.set_policy_set_param = rmt_ps_set_policy_set_param;
        ps->dm          = rmt;


        tasklet_init(&data->ingress_tasklet,
                     receive_worker,
                     (unsigned long) ps);
        tasklet_init(&data->egress_tasklet,
                     send_worker,
                     (unsigned long) ps);

        ps->max_q_policy_tx                 = default_max_q_policy_tx;
        ps->max_q_policy_rx                 = default_max_q_policy_rx;
        ps->rmt_q_monitor_policy_tx         = default_rmt_q_monitor_policy_tx;
        ps->rmt_q_monitor_policy_rx         = default_rmt_q_monitor_policy_rx;
        ps->rmt_scheduling_policy_tx        = default_rmt_scheduling_policy_tx;
        ps->rmt_scheduling_policy_rx        = default_rmt_scheduling_policy_rx;
        ps->rmt_scheduling_create_policy_tx = default_rmt_scheduling_create_policy_tx;
        ps->rmt_scheduling_create_policy_rx = default_rmt_scheduling_create_policy_rx;

        ps->max_q       = 256;

        return &ps->base;
}

static void
rmt_ps_default_destroy(struct ps_base * bps)
{
        struct rmt_ps *ps = container_of(bps, struct rmt_ps, base);
        struct rmt_ps_data * data = ps->priv;

        if (bps) {
                if (data) {
                        if (data->ss)    rkfree(data->ss);
                        if (data->inqs)  rmt_queue_set_destroy(data->inqs);
                        if (data->outqs) rmt_queue_set_destroy(data->outqs);
                        rkfree(data);
                }
                tasklet_kill(&data->ingress_tasklet);
                tasklet_kill(&data->egress_tasklet);
                rkfree(ps);
        }
}

struct ps_factory rmt_factory = {
        .owner   = THIS_MODULE,
        .create  = rmt_ps_default_create,
        .destroy = rmt_ps_default_destroy,
};

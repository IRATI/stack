/*
 *  Normal IPC Process
 *
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
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
#include <linux/list.h>
#include <linux/string.h>

#define IPCP_NAME   "normal-ipc"

#define RINA_PREFIX IPCP_NAME

#include "logs.h"
#include "common.h"
#include "debug.h"
#include "utils.h"
#include "kipcm.h"
#include "ipcp-utils.h"
#include "ipcp-factories.h"
#include "du.h"
#include "kfa.h"
#include "efcp.h"
#include "rmt.h"
#include "efcp-utils.h"
#include "connection.h"

/*  FIXME: To be removed ABSOLUTELY */
extern struct kipcm * default_kipcm;

struct normal_info {
        struct name * name;
        struct name * dif_name;
};

enum mgmt_state {
        MGMT_DATA_READY,
        MGMT_DATA_DESTROYED
};

struct mgmt_data {
        struct rfifo *    sdu_ready;
        wait_queue_head_t wait_q;
        spinlock_t        lock;
        atomic_t          readers;
        enum mgmt_state   state;
};

struct ipcp_instance_data {
        /* FIXME: add missing needed attributes */
        ipc_process_id_t        id;
        u32                     nl_port;
        struct list_head        flows;
        struct list_head        list;
        struct normal_info *    info;
        /*  FIXME: Remove it as soon as the kipcm_kfa gets removed*/
        struct kfa *            kfa;
        struct efcp_container * efcpc;
        struct rmt *            rmt;
        address_t               address;
        struct mgmt_data *      mgmt_data;
        spinlock_t              lock;
};

enum normal_flow_state {
        PORT_STATE_NULL = 1,
        PORT_STATE_PENDING,
        PORT_STATE_ALLOCATED,
        PORT_STATE_DEALLOCATED,
        PORT_STATE_DISABLED
};

struct cep_ids_entry {
        struct list_head list;
        cep_id_t         cep_id;
};

struct normal_flow {
        port_id_t              port_id;
        cep_id_t               active;
        struct list_head       cep_ids_list;
        struct list_head       list;
        enum normal_flow_state state;
};

static struct normal_flow * find_flow(struct ipcp_instance_data * data,
                                      port_id_t                   port_id)
{
        struct normal_flow * flow;

        list_for_each_entry(flow, &(data->flows), list) {
                if (flow->port_id == port_id)
                        return flow;
        }

        return NULL;
}

struct ipcp_factory_data {
        u32    nl_port;
        struct list_head instances;
};

static struct ipcp_factory_data normal_data;

static int normal_init(struct ipcp_factory_data * data)
{
        ASSERT(data);

        bzero(&normal_data, sizeof(normal_data));
        INIT_LIST_HEAD(&data->instances);

        return 0;
}

static int normal_fini(struct ipcp_factory_data * data)
{
        ASSERT(data);
        ASSERT(list_empty(&data->instances));
        return 0;
}

static int normal_sdu_enqueue(struct ipcp_instance_data * data,
                              port_id_t                   id,
                              struct sdu *                sdu)
{
        if (rmt_receive(data->rmt, sdu, id)) {
                LOG_ERR("Could not enqueue SDU into the RMT");
                return -1;
        }

        return 0;
}

static int normal_sdu_write(struct ipcp_instance_data * data,
                            port_id_t                   id,
                            struct sdu *                sdu)
{
        struct normal_flow * flow;

        spin_lock(&data->lock);
        flow = find_flow(data, id);
        if (!flow || flow->state != PORT_STATE_ALLOCATED) {
                spin_unlock(&data->lock);
                LOG_ERR("Write: There is no flow bound to this port_id: %d",
                        id);
                sdu_destroy(sdu);
                return -1;
        }
        spin_unlock(&data->lock);

        if (efcp_container_write(data->efcpc, flow->active, sdu)) {
                LOG_ERR("Could not send sdu to EFCP Container");
                return -1;
        }

        return 0;
}

static struct ipcp_factory * normal = NULL;

static struct ipcp_instance_data *
find_instance(struct ipcp_factory_data * data,
              ipc_process_id_t           id)
{
        struct ipcp_instance_data * pos;

        list_for_each_entry(pos, &(data->instances), list) {
                if (pos->id == id) {
                        return pos;
                }
        }

        return NULL;
}

static
cep_id_t connection_create_request(struct ipcp_instance_data * data,
                                   port_id_t                   port_id,
                                   address_t                   source,
                                   address_t                   dest,
                                   qos_id_t                    qos_id,
                                   struct conn_policies *      cp_params)
{
        cep_id_t               cep_id;
        struct connection *    conn;
        struct normal_flow *   flow;
        struct cep_ids_entry * cep_entry;

        conn = connection_create();
        if (!conn)
                return -1;

        conn->destination_address = dest;
        conn->source_address      = source;
        conn->port_id             = port_id;
        conn->qos_id              = qos_id;
        conn->source_cep_id       = cep_id_bad(); /* init value */
        conn->destination_cep_id  = cep_id_bad(); /* init velue */
        conn->policies_params     = cp_params;  /* Take the ownership. */

        cep_id = efcp_connection_create(data->efcpc, NULL, conn);
        if (!is_cep_id_ok(cep_id)) {
                LOG_ERR("Failed EFCP connection creation");
                return cep_id_bad();
        }

        cep_entry = rkzalloc(sizeof(*cep_entry), GFP_KERNEL);
        if (!cep_entry) {
                LOG_ERR("Could not create a cep_id entry, bailing out");
                efcp_connection_destroy(data->efcpc, cep_id);
                return cep_id_bad();
        }
        INIT_LIST_HEAD(&cep_entry->list);
        cep_entry->cep_id = cep_id;

        spin_lock(&data->lock);
        flow = find_flow(data, port_id);
        if (!flow) {
                spin_unlock(&data->lock);
                flow = rkzalloc(sizeof(*flow), GFP_KERNEL);
                if (!flow) {
                        LOG_ERR("Could not create a flow in normal-ipcp");
                        efcp_connection_destroy(data->efcpc, cep_id);
                        return cep_id_bad();
                }
                flow->port_id = port_id;
                INIT_LIST_HEAD(&flow->list);
                INIT_LIST_HEAD(&flow->cep_ids_list);
                spin_lock(&data->lock);
                list_add(&flow->list, &data->flows);
        }

        list_add(&cep_entry->list, &flow->cep_ids_list);
        flow->active = cep_id;
        flow->state = PORT_STATE_PENDING;

        spin_unlock(&data->lock);

        return cep_id;
}

static int connection_update_request(struct ipcp_instance_data * data,
                                     struct ipcp_instance *      user_ipcp,
                                     port_id_t                   port_id,
                                     cep_id_t                    src_cep_id,
                                     cep_id_t                    dst_cep_id)
{
        struct normal_flow *   flow;
        struct ipcp_instance * n1_ipcp;

        if (!user_ipcp)
                return cep_id_bad();

        n1_ipcp = kipcm_find_ipcp(default_kipcm, data->id);
        if (!n1_ipcp) {
                LOG_ERR("KIPCM cannot retrieve this IPCP");
                efcp_connection_destroy(data->efcpc, src_cep_id);
                return -1;
        }
        ASSERT(user_ipcp->ops);
        ASSERT(user_ipcp->ops->flow_binding_ipcp);
        if (user_ipcp->ops->flow_binding_ipcp(user_ipcp->data,
                                              port_id,
                                              n1_ipcp)) {
                LOG_ERR("Cannot bind flow with user ipcp");
                efcp_connection_destroy(data->efcpc, src_cep_id);
                return -1;
        }

        if (efcp_connection_update(data->efcpc,
                                   user_ipcp,
                                   src_cep_id,
                                   dst_cep_id))
                return -1;

        spin_lock(&data->lock);

        flow = find_flow(data, port_id);
        if (!flow) {
                spin_unlock(&data->lock);
                LOG_ERR("The flow with port-id %d is not pending, "
                        "cannot commit it", port_id);
                return -1;
        }

        if (flow->state != PORT_STATE_PENDING) {
                spin_unlock(&data->lock);
                LOG_ERR("Flow on port-id %d already committed", port_id);
                return -1;
        }

        flow->state = PORT_STATE_ALLOCATED;

        spin_unlock(&data->lock);

        LOG_DBG("Flow bound to port-id %d", port_id);
        return 0;
}

static struct normal_flow * find_flow_cepid(struct ipcp_instance_data * data,
                                            cep_id_t                    id)
{
        struct normal_flow * pos;

        list_for_each_entry(pos, &(data->flows), list) {
                if (pos->active == id) {
                        return pos;
                }
        }
        return NULL;
}

static int remove_cep_id_from_flow(struct normal_flow * flow,
                                   cep_id_t             id)
{
        struct cep_ids_entry *pos, *next;

        list_for_each_entry_safe(pos, next, &(flow->cep_ids_list), list) {
                if (pos->cep_id == id) {
                        list_del(&pos->list);
                        rkfree(pos);
                        return 0;
                }
        }
        return -1;
}

static int ipcp_flow_binding(struct ipcp_instance_data * user_data,
                             port_id_t                   pid,
                             struct ipcp_instance *      n1_ipcp)
{ return rmt_n1port_bind(user_data->rmt, pid, n1_ipcp); }

static int normal_flow_unbinding_ipcp(struct ipcp_instance_data * user_data,
                                      port_id_t                   pid)
{
        if (rmt_n1port_unbind(user_data->rmt, pid))
                return -1;

        return 0;
}

static int normal_flow_unbinding_user_ipcp(struct ipcp_instance_data * data,
                                           port_id_t                   pid)
{
        struct normal_flow * flow;

        ASSERT(data);

        spin_lock(&data->lock);
        flow = find_flow(data, pid);
        if (!flow || !flow->active) {
                spin_unlock(&data->lock);
                LOG_ERR("Could not find flow with port %d to unbind user IPCP",
                        pid);
                return -1;
        }
        spin_unlock(&data->lock);

        if (efcp_container_unbind_user_ipcp(data->efcpc, flow->active)){
                spin_unlock(&data->lock);
                return -1;
        }

        return 0;
}

static int connection_destroy_request(struct ipcp_instance_data * data,
                                      cep_id_t                    src_cep_id)
{
        struct normal_flow * flow;

        if (!data) {
                LOG_ERR("Bogus instance passed");
                return -1;
        }
        if (efcp_connection_destroy(data->efcpc, src_cep_id))
                LOG_ERR("Could not destroy EFCP instance: %d", src_cep_id);

        spin_lock(&data->lock);
        if (!(&data->flows)) {
                spin_unlock(&data->lock);
                LOG_ERR("Could not destroy EFCP instance: %d", src_cep_id);
                return -1;
        }
        flow = find_flow_cepid(data, src_cep_id);
        if (!flow) {
                spin_unlock(&data->lock);
                LOG_ERR("Could not retrieve flow by cep_id :%d", src_cep_id);
                return -1;
        }
        if (remove_cep_id_from_flow(flow, src_cep_id))
                LOG_ERR("Could not remove cep_id: %d", src_cep_id);

        if (list_empty(&flow->cep_ids_list))
                rkfree(flow);
        spin_unlock(&data->lock);

        return 0;
}

static cep_id_t
connection_create_arrived(struct ipcp_instance_data * data,
                          struct ipcp_instance *      user_ipcp,
                          port_id_t                   port_id,
                          address_t                   source,
                          address_t                   dest,
                          qos_id_t                    qos_id,
                          cep_id_t                    dst_cep_id,
                          struct conn_policies *      cp_params)
{
        struct connection *    conn;
        cep_id_t               cep_id;
        struct normal_flow *   flow;
        struct cep_ids_entry * cep_entry;
        struct ipcp_instance * ipcp;

        if (!user_ipcp)
                return cep_id_bad();

        conn = rkzalloc(sizeof(*conn), GFP_KERNEL);
        if (!conn) {
                LOG_ERR("Failed connection creation");
                return cep_id_bad();
        }
        conn->destination_address = dest;
        conn->source_address      = source;
        conn->port_id             = port_id;
        conn->qos_id              = qos_id;
        conn->source_cep_id       = cep_id_bad(); /* init values */
        conn->destination_cep_id  = dst_cep_id;
        conn->policies_params     = cp_params;  /* Take the ownership. */

        cep_id = efcp_connection_create(data->efcpc, user_ipcp, conn);
        if (!is_cep_id_ok(cep_id)) {
                LOG_ERR("Failed EFCP connection creation");
                return cep_id_bad();
        }
        LOG_DBG("Cep_id allocated for the arrived connection request: %d",
                cep_id);

        cep_entry = rkzalloc(sizeof(*cep_entry), GFP_KERNEL);
        if (!cep_entry) {
                LOG_ERR("Could not create a cep_id entry, bailing out");
                efcp_connection_destroy(data->efcpc, cep_id);
                return cep_id_bad();
        }
        INIT_LIST_HEAD(&cep_entry->list);
        cep_entry->cep_id = cep_id;

        spin_lock(&data->lock);
        flow = find_flow(data, port_id);
        if (!flow) {
                spin_unlock(&data->lock);
                flow = rkzalloc(sizeof(*flow), GFP_KERNEL);
                if (!flow) {
                        LOG_ERR("Could not create a flow in normal-ipcp");
                        efcp_connection_destroy(data->efcpc, cep_id);
                        return cep_id_bad();
                }
                flow->port_id = port_id;
                INIT_LIST_HEAD(&flow->list);
                INIT_LIST_HEAD(&flow->cep_ids_list);

                ipcp = kipcm_find_ipcp(default_kipcm, data->id);
                if (!ipcp) {
                        LOG_ERR("KIPCM could not retrieve this IPCP");
                        efcp_connection_destroy(data->efcpc, cep_id);
                        return cep_id_bad();
                }

                ASSERT(user_ipcp->ops);
                ASSERT(user_ipcp->ops->flow_binding_ipcp);
                if (user_ipcp->ops->flow_binding_ipcp(user_ipcp->data,
                                                      conn->port_id,
                                                      ipcp)) {
                        LOG_ERR("Could not bind flow with user_ipcp");
                        efcp_connection_destroy(data->efcpc, cep_id);
                        return cep_id_bad();
                }

                spin_lock(&data->lock);
                list_add(&flow->list, &data->flows);

        }

        list_add(&cep_entry->list, &flow->cep_ids_list);
        flow->active = cep_id;
        flow->state = PORT_STATE_ALLOCATED;
        spin_unlock(&data->lock);

        return cep_id;
}

static int remove_all_cepid(struct ipcp_instance_data * data,
                            struct normal_flow *        flow)
{
        struct cep_ids_entry *pos, *next;

        ASSERT(data);

        list_for_each_entry_safe(pos, next, &flow->cep_ids_list, list) {
                efcp_connection_destroy(data->efcpc, pos->cep_id);
                list_del(&pos->list);
                rkfree(pos);
        }

        return 0;
}

static int normal_deallocate(struct ipcp_instance_data * data,
                             port_id_t                   port_id)
{
        struct normal_flow * flow;
        unsigned long        flags;

        if (!data) {
                LOG_ERR("Bogus instance passed");
                return -1;
        }

        spin_lock_irqsave(&data->lock, flags);
        flow = find_flow(data, port_id);
        if (!flow) {
                spin_unlock_irqrestore(&data->lock, flags);
                LOG_ERR("Could not find flow %d to deallocate", port_id);
                return -1;
        }
        flow->state = PORT_STATE_DEALLOCATED;
        spin_unlock_irqrestore(&data->lock, flags);

        remove_all_cepid(data, flow);

        list_del(&flow->list);

        rkfree(flow);

        return 0;
}

static int enable_write(struct ipcp_instance_data * data,
                        port_id_t                   port_id)
{
        if (rmt_enable_port_id(data->rmt, port_id))
                return -1;

        return 0;
}

static int disable_write(struct ipcp_instance_data * data,
                         port_id_t                   port_id) {
        if (rmt_disable_port_id(data->rmt, port_id))
                return -1;

        return 0;
}

static int normal_assign_to_dif(struct ipcp_instance_data * data,
                                const struct dif_info *     dif_information)
{
        struct efcp_config * efcp_config;

        data->info->dif_name = name_dup(dif_information->dif_name);
        data->address        = dif_information->configuration->address;

        efcp_config = dif_information->configuration->efcp_config;

        if (!efcp_config) {
                LOG_ERR("No EFCP configuration in the dif_info");
                return -1;
        }

        if (!efcp_config->dt_cons) {
                LOG_ERR("Configuration constants for the DIF are bogus...");
                efcp_config_destroy(efcp_config);
                return -1;
        }

        efcp_container_set_config(efcp_config, data->efcpc);

        if (rmt_address_set(data->rmt, data->address))
                return -1;

        if (rmt_dt_cons_set(data->rmt, dt_cons_dup(efcp_config->dt_cons))) {
                LOG_ERR("Could not set dt_cons in RMT");
                return -1;
        }

        return 0;
}

static bool queue_ready(struct mgmt_data * mgmt_data)
{
        if (!mgmt_data                                ||
            (mgmt_data->state == MGMT_DATA_DESTROYED) ||
            !mgmt_data->sdu_ready                     ||
            !rfifo_is_empty(mgmt_data->sdu_ready))
                return true;
        return false;
}

static int mgmt_remove(struct mgmt_data * data)
{
        if (!data)
                return -1;

        if (data->sdu_ready)
                rfifo_destroy(data->sdu_ready,
                              (void (*)(void *)) sdu_wpi_destroy);

        rkfree(data);

        return 0;
}

static int normal_mgmt_sdu_read(struct ipcp_instance_data * data,
                                struct sdu_wpi **           sdu_wpi)
{
        struct mgmt_data * mgmt_data;
        int                retval;
        unsigned long      flags;

        if (!data) {
                LOG_ERR("Bogus instance data");
                return -1;
        }

        LOG_DBG("Trying to read mgmt SDU from IPC Process %d", data->id);

        IRQ_BARRIER;

        mgmt_data = data->mgmt_data;
        if (!mgmt_data) {
                LOG_ERR("No normal_mgmt data in IPC Process %d", data->id);
                return -1;
        }

        spin_lock_irqsave(&mgmt_data->lock, flags);
        if (mgmt_data->state == MGMT_DATA_DESTROYED) {
                LOG_DBG("IPCP %d is being destroyed", data->id);
                spin_unlock_irqrestore(&mgmt_data->lock, flags);
                return -1;
        }

        atomic_inc(&mgmt_data->readers);

        while (rfifo_is_empty(mgmt_data->sdu_ready)) {
                spin_unlock_irqrestore(&mgmt_data->lock, flags);

                LOG_DBG("Mgmt read going to sleep...");
                retval = wait_event_interruptible(mgmt_data->wait_q,
                                                  queue_ready(mgmt_data));

                if (!mgmt_data  || !mgmt_data->sdu_ready) {
                        LOG_ERR("No mgmt data anymore, waitqueue "
                                "return code was %d", retval);
                        return -1;
                }

                spin_lock_irqsave(&mgmt_data->lock, flags);
                if (retval) {
                        LOG_DBG("Mgmt queue waken up by interruption, "
                                "returned error %d", retval);
                        goto finish;
                }

                if (mgmt_data->state == MGMT_DATA_DESTROYED) {
                        LOG_DBG("Mgmt data destroyed, waitqueue "
                                "return code %d", retval);
                        break;
                }
        }

        if (rfifo_is_empty(mgmt_data->sdu_ready)) {
                retval = -1;
                goto finish;
        }

        *sdu_wpi = rfifo_pop(mgmt_data->sdu_ready);
        if (!sdu_wpi_is_ok(*sdu_wpi)) {
                LOG_ERR("There is not enough data in the management queue");
                retval = -1;
        }

 finish:
        if (atomic_dec_and_test(&mgmt_data->readers) &&
            (mgmt_data->state == MGMT_DATA_DESTROYED)) {
                spin_unlock_irqrestore(&mgmt_data->lock, flags);
                if (mgmt_remove(mgmt_data))
                        LOG_ERR("Could not destroy mgmt_data");
                return retval;
        }

        spin_unlock_irqrestore(&mgmt_data->lock, flags);
        return retval;
}

static int normal_mgmt_sdu_write(struct ipcp_instance_data * data,
                                 address_t                   dst_addr,
                                 port_id_t                   port_id,
                                 struct sdu *                sdu)
{
        struct pci *  pci;
        struct pdu *  pdu;

        LOG_DBG("Passing SDU to be written to N-1 port %d "
                "from IPC Process %d", port_id, data->id);

        if (!sdu) {
                LOG_ERR("No data passed, bailing out");
                return -1;
        }

        pci = pci_create();
        if (!pci)
                return -1;

        /* FIXME: qos_id is set to 1 since 0 is QOS_ID_WRONG */
        if (pci_format(pci,
                       0,
                       0,
                       data->address,
                       dst_addr,
                       0,
                       1,
                       PDU_TYPE_MGMT)) {
                pci_destroy(pci);
                return -1;
        }

        pdu = pdu_create();
        if (!pdu) {
                pci_destroy(pci);
                return -1;
        }

        if (pdu_buffer_set(pdu, sdu_buffer_rw(sdu))) {
                pci_destroy(pci);
                return -1;
        }

        if (pdu_pci_set(pdu, pci)) {
                pci_destroy(pci);
                return -1;
        }

        /*
         * NOTE:
         *   Decide on how to deliver to the RMT depending on
         *   port_id or dst_addr
         */
        if (dst_addr) {
                if (rmt_send(data->rmt,
                             pci_destination(pdu_pci_get_ro(pdu)),
                             pci_qos_id(pdu_pci_get_ro(pdu)),
                             pdu)) {
                        LOG_ERR("Could not send to RMT (using dst_addr");
                        return -1;
                }
        } else if (port_id) {
                if (rmt_send_port_id(data->rmt,
                                     port_id,
                                     pdu)) {
                        LOG_ERR("Could not send to RMT (using port_id)");
                        return -1;
                }
        } else {
                LOG_ERR("Could not send to RMT: no port_id nor dst_addr");
                pdu_destroy(pdu);
                return -1;
        }

        return 0;
}

static int normal_mgmt_sdu_post(struct ipcp_instance_data * data,
                                port_id_t                   port_id,
                                struct sdu *                sdu)
{
        /* FIXME: We should get rid of sdu_wpi ASAP */
        struct sdu_wpi * tmp;

        if (!data) {
                LOG_ERR("Bogus instance passed");
                sdu_destroy(sdu);
                return -1;
        }

        if (!is_port_id_ok(port_id)) {
                LOG_ERR("Wrong port id");
                sdu_destroy(sdu);
                return -1;
        }
        if (!sdu_is_ok(sdu)) {
                LOG_ERR("Bogus management SDU");
                sdu_destroy(sdu);
                return -1;
        }

        tmp = rkzalloc(sizeof(*tmp), GFP_ATOMIC);
        if (!tmp) {
                sdu_destroy(sdu);
                return -1;
        }

        if (!data->mgmt_data) {
                LOG_ERR("No mgmt data for IPCP %d", data->id);
                sdu_destroy(sdu);
                rkfree(tmp);
                return -1;
        }

        if (data->mgmt_data->state == MGMT_DATA_DESTROYED) {
                LOG_ERR("IPCP %d is being destroyed", data->id);
                sdu_destroy(sdu);
                rkfree(tmp);
                return -1;
        }

        tmp->port_id = port_id;
        tmp->sdu     = sdu;
        spin_lock(&data->mgmt_data->lock);
        if (rfifo_push_ni(data->mgmt_data->sdu_ready,
                          tmp)) {
                sdu_destroy(sdu);
                rkfree(tmp);
                spin_unlock(&data->mgmt_data->lock);
                return -1;
        }
        spin_unlock(&data->mgmt_data->lock);

        LOG_DBG("Gonna wake up waitqueue: %d", port_id);
        wake_up_interruptible(&data->mgmt_data->wait_q);

        return 0;
}

static int normal_pft_add(struct ipcp_instance_data * data,
                          address_t                   address,
                          qos_id_t                    qos_id,
                          port_id_t *                 ports,
                          size_t                      size)

{
        ASSERT(data);

        return rmt_pft_add(data->rmt,
                           address,
                           qos_id,
                           ports,
                           size);
}

static int normal_pft_remove(struct ipcp_instance_data * data,
                             address_t                   address,
                             qos_id_t                    qos_id,
                             port_id_t *                 ports,
                             size_t                      size)
{
        ASSERT(data);

        return rmt_pft_remove(data->rmt,
                              address,
                              qos_id,
                              ports,
                              size);
}

static int normal_pft_dump(struct ipcp_instance_data * data,
                           struct list_head *          entries)
{
        ASSERT(data);

        return rmt_pft_dump(data->rmt,
                            entries);
}

static int normal_pft_flush(struct ipcp_instance_data * data)
{
        ASSERT(data);

        return rmt_pft_flush(data->rmt);
}

static const struct name * normal_ipcp_name(struct ipcp_instance_data * data)
{
        ASSERT(data);
        ASSERT(data->info);
        ASSERT(name_is_ok(data->info->name));

        return data->info->name;
}

static int normal_set_policy_set_param(struct ipcp_instance_data * data,
                                       const string_t *path,
                                       const string_t *param_name,
                                       const string_t *param_value)
{
        size_t cmplen;
        size_t offset;

        parse_component_id(path, &cmplen, &offset);

        if (strncmp(path, "rmt", cmplen) == 0) {
                return rmt_set_policy_set_param(data->rmt, path + offset,
                                                param_name, param_value);
        } else if (strncmp(path, "efcp", cmplen) == 0) {
                return efcp_container_set_policy_set_param(data->efcpc,
                                path + offset, param_name, param_value);
        } else {
                LOG_ERR("The selected component does not exist");
                return -1;
        }

        return -1;
}

static int normal_select_policy_set(struct ipcp_instance_data *data,
                                    const string_t *path,
                                    const string_t *ps_name)
{
        size_t cmplen;
        size_t offset;

        parse_component_id(path, &cmplen, &offset);

        if (strncmp(path, "rmt", cmplen) == 0) {
                return rmt_select_policy_set(data->rmt, path + offset,
                                             ps_name);
        } else if (strncmp(path, "efcp", cmplen) == 0) {
                return efcp_container_select_policy_set(data->efcpc,
                                                path + offset, ps_name);
        } else {
                LOG_ERR("The selected component does not exist");
                return -1;
        }

        return -1;
}

static struct ipcp_instance_ops normal_instance_ops = {
        .flow_allocate_request     = NULL,
        .flow_allocate_response    = NULL,
        .flow_deallocate           = normal_deallocate,
        .flow_binding_ipcp         = ipcp_flow_binding,
        .flow_unbinding_ipcp       = normal_flow_unbinding_ipcp,
        .flow_unbinding_user_ipcp  = normal_flow_unbinding_user_ipcp,

        .application_register      = NULL,
        .application_unregister    = NULL,

        .assign_to_dif             = normal_assign_to_dif,
        .update_dif_config         = NULL,

        .connection_create         = connection_create_request,
        .connection_update         = connection_update_request,
        .connection_destroy        = connection_destroy_request,
        .connection_create_arrived = connection_create_arrived,

        .sdu_enqueue               = normal_sdu_enqueue,
        .sdu_write                 = normal_sdu_write,

        .mgmt_sdu_read             = normal_mgmt_sdu_read,
        .mgmt_sdu_write            = normal_mgmt_sdu_write,
        .mgmt_sdu_post             = normal_mgmt_sdu_post,

        .pft_add                   = normal_pft_add,
        .pft_remove                = normal_pft_remove,
        .pft_dump                  = normal_pft_dump,
        .pft_flush                 = normal_pft_flush,

        .query_rib		   = NULL,

        .ipcp_name                 = normal_ipcp_name,

        .set_policy_set_param      = normal_set_policy_set_param,
        .select_policy_set         = normal_select_policy_set,

        .enable_write              = enable_write,
        .disable_write             = disable_write
};

static struct mgmt_data * normal_mgmt_data_create(void)
{
        struct mgmt_data * data;

        data = rkzalloc(sizeof(*data), GFP_KERNEL);
        if (!data) {
                LOG_ERR("Could not allocate memory for RMT mgmt struct");
                return NULL;
        }

        data->state     = MGMT_DATA_READY;
        data->sdu_ready = rfifo_create();
        if (!data->sdu_ready) {
                LOG_ERR("Could not create MGMT SDUs queue");
                rkfree(data);
                return NULL;
        }

        init_waitqueue_head(&data->wait_q);
        spin_lock_init(&data->lock);

        return data;
}

static int mgmt_data_destroy(struct mgmt_data * data)
{
        if (!data)
                return -1;

        spin_lock(&data->lock);
        data->state = MGMT_DATA_DESTROYED;
        if ((atomic_read(&data->readers) == 0)) {
                spin_unlock(&data->lock);
                mgmt_remove(data);
                return 0;
        }
        spin_unlock(&data->lock);

        wake_up_interruptible_all(&data->wait_q);

        return 0;
}

static struct ipcp_instance * normal_create(struct ipcp_factory_data * data,
                                            const struct name *        name,
                                            ipc_process_id_t           id)
{
        struct ipcp_instance * instance;

        ASSERT(data);

        if (find_instance(data, id)) {
                LOG_ERR("There is already a normal ipcp instance with id %d",
                        id);
                return NULL;
        }

        LOG_DBG("Creating normal IPC process...");
        instance = rkzalloc(sizeof(*instance), GFP_KERNEL);
        if (!instance) {
                LOG_ERR("Could not allocate memory for normal ipc process "
                        "with id %d", id);
                return NULL;
        }

        instance->ops  = &normal_instance_ops;

        /* FIXME: Rearrange the mess creating the data */
        instance->data = rkzalloc(sizeof(struct ipcp_instance_data),
                                  GFP_KERNEL);
        if (!instance->data) {
                LOG_ERR("Could not allocate memory for normal ipcp "
                        "internal data");
                rkfree(instance);
                return NULL;
        }

        instance->data->id      = id;
        instance->data->nl_port = data->nl_port;
        instance->data->info    = rkzalloc(sizeof(struct normal_info),
                                           GFP_KERNEL);
        if (!instance->data->info) {
                LOG_ERR("Could not allocate memory for normal ipcp info");
                rkfree(instance->data);
                rkfree(instance);
                return NULL;
        }

        /* FIXME: This (whole) function has to be rehersed HEAVILY */

        instance->data->info->name = name_dup(name);
        if (!instance->data->info->name) {
                LOG_ERR("Failed creation of ipc name");
                rkfree(instance->data->info);
                rkfree(instance->data);
                rkfree(instance);
                return NULL;
        }

        /*  FIXME: Remove as soon as the kipcm_kfa gets removed */
        instance->data->kfa = kipcm_kfa(default_kipcm);

        instance->data->efcpc = efcp_container_create(instance->data->kfa);
        if (!instance->data->efcpc) {
                name_destroy(instance->data->info->name);
                rkfree(instance->data->info);
                rkfree(instance->data);
                rkfree(instance);
                return NULL;
        }
        instance->data->rmt = rmt_create(instance,
                                         instance->data->kfa,
                                         instance->data->efcpc);
        if (!instance->data->rmt) {
                LOG_ERR("Failed creation of RMT instance");
                efcp_container_destroy(instance->data->efcpc);
                name_destroy(instance->data->info->name);
                rkfree(instance->data->info);
                rkfree(instance->data);
                rkfree(instance);
                return NULL;
        }

        if (efcp_bind_rmt(instance->data->efcpc, instance->data->rmt)) {
                LOG_ERR("Failed binding of RMT and EFCPC");
                rmt_destroy(instance->data->rmt);
                efcp_container_destroy(instance->data->efcpc);
                name_destroy(instance->data->info->name);
                rkfree(instance->data->info);
                rkfree(instance->data);
                rkfree(instance);
                return NULL;
        }

        instance->data->mgmt_data = normal_mgmt_data_create();
        if (!instance->data->mgmt_data) {
                LOG_ERR("Failed creation of management data");
                rmt_destroy(instance->data->rmt);
                efcp_container_destroy(instance->data->efcpc);
                name_destroy(instance->data->info->name);
                rkfree(instance->data->info);
                rkfree(instance->data);
                rkfree(instance);
                return NULL;
        }

        INIT_LIST_HEAD(&instance->data->flows);
        INIT_LIST_HEAD(&instance->data->list);
        spin_lock_init(&instance->data->lock);
        list_add(&(instance->data->list), &(data->instances));
        LOG_DBG("Normal IPC process instance created and added to the list");

        return instance;
}

static int normal_deallocate_all(struct ipcp_instance_data * data)
{
        struct normal_flow *flow, *next;
        unsigned long       flags;

        spin_lock_irqsave(&data->lock, flags);
        list_for_each_entry_safe(flow, next, &(data->flows), list) {
                if (remove_all_cepid(data, flow))
                        LOG_ERR("Some efcp structures could not be destroyed"
                                "in flow %d", flow->port_id);

                list_del(&flow->list);
                rkfree(flow);
        }
        spin_unlock_irqrestore(&data->lock, flags);

        return 0;
}

static int normal_destroy(struct ipcp_factory_data * data,
                          struct ipcp_instance *     instance)
{

        struct ipcp_instance_data * tmp;

        ASSERT(data);
        ASSERT(instance);

        tmp = find_instance(data, instance->data->id);
        if (!tmp) {
                LOG_ERR("Could not find normal ipcp instance to destroy");
                return -1;
        }

        list_del(&tmp->list);

        if (normal_deallocate_all(tmp)) {
                LOG_ERR("Could not deallocate normal ipcp flows");
                return -1;
        }

        if (tmp->info) {
                if (tmp->info->dif_name)
                        name_destroy(tmp->info->dif_name);
                if (tmp->info->name)
                        name_destroy(tmp->info->name);
                rkfree(tmp->info);
        }

        efcp_container_destroy(tmp->efcpc);
        rmt_destroy(tmp->rmt);
        mgmt_data_destroy(tmp->mgmt_data);
        rkfree(tmp);
        rkfree(instance);

        return 0;
}

static struct ipcp_factory_ops normal_ops = {
        .init    = normal_init,
        .fini    = normal_fini,
        .create  = normal_create,
        .destroy = normal_destroy,
};

static int __init mod_init(void)
{
        normal = kipcm_ipcp_factory_register(default_kipcm,
                                             IPCP_NAME,
                                             &normal_data,
                                             &normal_ops);
        if (!normal)
                return -1;

        return 0;
}

static void __exit mod_exit(void)
{
        ASSERT(normal);

        kipcm_ipcp_factory_unregister(default_kipcm, normal);
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("RINA normal IPC Process");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini@nextworks.it>");
MODULE_AUTHOR("Miquel Tarzan <miquel.tarzan@i2cat.net>");
MODULE_AUTHOR("Sander Vrijders <sander.vrijders@intec.ugent.be>");
MODULE_AUTHOR("Leonardo Bergesio <leonardo.bergesio@i2cat.net>");

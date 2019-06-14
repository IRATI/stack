/*
 * EFCP (Error and Flow Control Protocol)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
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
/* For wait_queue */
#include <linux/sched.h>
#include <linux/wait.h>


#define RINA_PREFIX "efcp"

#include "logs.h"
#include "utils.h"
#include "connection.h"
#include "debug.h"
#include "delim-ps.h"
#include "efcp-str.h"
#include "efcp.h"
#include "efcp-utils.h"
#include "ipcp-utils.h"
#include "cidm.h"
#include "dtp.h"
#include "dtcp.h"
#include "dtcp-ps.h"
#include "dtp-conf-utils.h"
#include "dtcp-conf-utils.h"
#include "rmt.h"
#include "dtp-utils.h"
#include "dtp-ps.h"
#include "policies.h"

static ssize_t efcp_attr_show(struct robject *		     robj,
                         	     struct robj_attribute * attr,
                                     char *		     buf)
{
	struct efcp * instance;

	instance = container_of(robj, struct efcp, robj);
	if (!instance || !instance->connection || !instance->dtp)
		return 0;

	if (strcmp(robject_attr_name(attr), "src_address") == 0)
		return sprintf(buf, "%u\n",
			       instance->connection->source_address);
	if (strcmp(robject_attr_name(attr), "dst_address") == 0)
		return sprintf(buf, "%u\n",
			       instance->connection->destination_address);
	if (strcmp(robject_attr_name(attr), "src_cep_id") == 0)
		return sprintf(buf, "%d\n",
			       instance->connection->source_cep_id);
	if (strcmp(robject_attr_name(attr), "dst_cep_id") == 0)
		return sprintf(buf, "%d\n",
			       instance->connection->destination_cep_id);
	if (strcmp(robject_attr_name(attr), "qos_id") == 0)
		return sprintf(buf, "%u\n",
			       instance->connection->qos_id);
	if (strcmp(robject_attr_name(attr), "port_id") == 0)
		return sprintf(buf, "%u\n",
			       instance->connection->port_id);
	if (strcmp(robject_attr_name(attr), "a_timer") == 0)
		return sprintf(buf, "%u\n", instance->dtp->sv->A);
	if (strcmp(robject_attr_name(attr), "r_timer") == 0)
		return sprintf(buf, "%u\n", instance->dtp->sv->R);
	if (strcmp(robject_attr_name(attr), "tr_timeout") == 0)
		return sprintf(buf, "%u\n", instance->dtp->sv->tr);
	if (strcmp(robject_attr_name(attr), "max_flow_pdu_size") == 0)
		return sprintf(buf, "%u\n",
			       instance->dtp->sv->max_flow_pdu_size);
	if (strcmp(robject_attr_name(attr), "max_flow_sdu_size") == 0)
		return sprintf(buf, "%u\n",
			       instance->dtp->sv->max_flow_sdu_size);
	if (strcmp(robject_attr_name(attr), "max_packet_life") == 0)
		return sprintf(buf, "%u\n", instance->dtp->sv->MPL);
	return 0;
}
RINA_SYSFS_OPS(efcp);
RINA_ATTRS(efcp, src_address, dst_address, src_cep_id, dst_cep_id,
	qos_id, port_id, a_timer, r_timer, tr_timeout, max_flow_pdu_size,
	max_flow_sdu_size, max_packet_life);
RINA_KTYPE(efcp);

static struct efcp * efcp_create(void)
{
        struct efcp * instance;

        instance = rkzalloc(sizeof(*instance), GFP_KERNEL);
        if (!instance)
                return NULL;

        instance->state = EFCP_ALLOCATED;
        atomic_set(&instance->pending_ops, 0);
        instance->delim = NULL;

        LOG_DBG("Instance %pK initialized successfully", instance);

        return instance;
}

int efcp_container_unbind_user_ipcp(struct efcp_container * efcpc,
                                    cep_id_t                cep_id)
{
        struct efcp * efcp;

        ASSERT(efcpc);

        if (!is_cep_id_ok(cep_id)) {
                LOG_ERR("Bad cep-id, cannot retrieve efcp instance");
                return -1;
        }

        spin_lock_bh(&efcpc->lock);
        efcp = efcp_imap_find(efcpc->instances, cep_id);
        if (!efcp) {
                spin_unlock_bh(&efcpc->lock);
                LOG_ERR("There is no EFCP bound to this cep-id %d", cep_id);
                return -1;
        }
        efcp->user_ipcp = NULL;
        spin_unlock_bh(&efcpc->lock);

        return 0;
}
EXPORT_SYMBOL(efcp_container_unbind_user_ipcp);

static int efcp_destroy(struct efcp * instance)
{
        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }

        if (instance->user_ipcp) {
                instance->user_ipcp->ops->flow_unbinding_ipcp(
                                instance->user_ipcp->data,
                                instance->connection->port_id);
        }

        if (instance->dtp) {
                /*
                 * FIXME:
                 *   Shouldn't we check for flows running, before
                 *   unbinding dtp, dtcp, cwq and rtxw ???
                 */
                dtp_destroy(instance->dtp);
        } else
                LOG_WARN("No DT instance present");


        if (instance->connection) {
                /* FIXME: Connection should release the cep id */
                if (is_cep_id_ok(instance->connection->source_cep_id)) {
                        ASSERT(instance->container);
                        ASSERT(instance->container->cidm);

                        cidm_release(instance->container->cidm,
                        	     instance->connection->source_cep_id);
                }
                /* FIXME: Should we release (actually the connection) release
                 * the destination cep id? */

                connection_destroy(instance->connection);
        }

        if (instance->delim) {
        	delim_destroy(instance->delim);
        }

	robject_del(&instance->robj);
        rkfree(instance);

        LOG_DBG("EFCP instance %pK finalized successfully", instance);

        return 0;
}

struct efcp_container * efcp_container_create(struct kfa * kfa, struct robject * parent)
{
        struct efcp_container * container;

        if (!kfa) {
                LOG_ERR("Bogus KFA instances passed, bailing out");
                return NULL;
        }

        container = rkzalloc(sizeof(*container), GFP_KERNEL);
        if (!container)
                return NULL;

	container->rset        = NULL;
        container->instances   = efcp_imap_create();
        container->cidm        = cidm_create();
        if (!container->instances ||
            !container->cidm) {
                LOG_ERR("Failed to create EFCP container instance");
                efcp_container_destroy(container);
                return NULL;
        }

        container->kfa = kfa;
        spin_lock_init(&container->lock);
	init_waitqueue_head(&container->del_wq);

	container->rset = rset_create_and_add("connections", parent);
	if (!container->rset) {
                LOG_ERR("Failed to create EFCP container sysfs entrance");
                efcp_container_destroy(container);
                return NULL;
	}

        return container;
}
EXPORT_SYMBOL(efcp_container_create);

int efcp_container_destroy(struct efcp_container * container)
{
        if (!container) {
                LOG_ERR("Bogus container passed, bailing out");
                return -1;
        }

        if (container->instances)  efcp_imap_destroy(container->instances,
                                                     efcp_destroy);
        if (container->cidm)       cidm_destroy(container->cidm);

        if (container->config)     efcp_config_free(container->config);

	if (container->rset)       rset_unregister(container->rset);
        rkfree(container);

        return 0;
}
EXPORT_SYMBOL(efcp_container_destroy);

struct efcp * efcp_container_find(struct efcp_container * container,
                                  cep_id_t                id)
{
        struct efcp * efcp = NULL;

        if (!container) {
                LOG_ERR("Bogus container passed, bailing out");
                return NULL;
        }
        if (!is_cep_id_ok(id)) {
                LOG_ERR("Bad cep-id, cannot find instance");
                return NULL;
        }

        spin_lock_bh(&container->lock);
        efcp = efcp_imap_find(container->instances, id);
        spin_unlock_bh(&container->lock);

        return efcp;
}
EXPORT_SYMBOL(efcp_container_find);

int efcp_container_config_set(struct efcp_container * container,
			      struct efcp_config *    efcp_cfg)
{
        if (!efcp_cfg || !efcp_cfg->dt_cons || !container) {
                LOG_ERR("Bogus input parameters, bailing out");
                return -1;
        }

	efcp_cfg->pci_offset_table = pci_offset_table_create(efcp_cfg->dt_cons);
        container->config = efcp_cfg;
        if (container->config->dt_cons->max_sdu_size == 0) {
        	container->config->dt_cons->max_sdu_size =
        			container->config->dt_cons->max_pdu_size -
        			pci_calculate_size(container->config, PDU_TYPE_DT);
        }

        LOG_DBG("Succesfully set EFCP config to EFCP container");

        return 0;
}
EXPORT_SYMBOL(efcp_container_config_set);

static int efcp_write(struct efcp * efcp,
                      struct du *  du)
{
	struct delim_ps * delim_ps = NULL;
	struct du_list_item * next_du = NULL;

	/* Handle fragmentation here */
	if (efcp->delim) {
		delim_ps = container_of(rcu_dereference(efcp->delim->base.ps),
						        struct delim_ps,
						        base);

		if (delim_ps->delim_fragment(delim_ps, du,
					     efcp->delim->tx_dus)) {
			LOG_ERR("Error performing SDU fragmentation");
			du_list_clear(efcp->delim->tx_dus, true);
			return -1;
		}

	        list_for_each_entry(next_du, &(efcp->delim->tx_dus->dus),
	        		    next) {
	                if (dtp_write(efcp->dtp, next_du->du)) {
	                	LOG_ERR("Could not write SDU fragment to DTP");
	                	/* TODO, what to do here? */
	                }
	        }

	        du_list_clear(efcp->delim->tx_dus, false);

		return 0;
	}

	/* No fragmentation */
        if (dtp_write(efcp->dtp, du)) {
                LOG_ERR("Could not write SDU to DTP");
                return -1;
        }

        return 0;
}

int efcp_container_write(struct efcp_container * container,
                         cep_id_t                cep_id,
                         struct du *             du)
{
        struct efcp * efcp;
        int           ret;

        if (!is_cep_id_ok(cep_id)) {
                LOG_ERR("Bad cep-id, cannot write into container");
                du_destroy(du);
                return -1;
        }

        du->cfg = container->config;

        spin_lock_bh(&container->lock);
        efcp = efcp_imap_find(container->instances, cep_id);
        if (!efcp) {
                spin_unlock_bh(&container->lock);
                LOG_ERR("There is no EFCP bound to this cep-id %d", cep_id);
                du_destroy(du);
                return -1;
        }
        if (efcp->state == EFCP_DEALLOCATED) {
                spin_unlock_bh(&container->lock);
                du_destroy(du);
                LOG_DBG("EFCP already deallocated");
                return 0;
        }
        atomic_inc(&efcp->pending_ops);
        spin_unlock_bh(&container->lock);

        ret = efcp_write(efcp, du);

        spin_lock_bh(&container->lock);
        if (atomic_dec_and_test(&efcp->pending_ops) &&
        		efcp->state == EFCP_DEALLOCATED) {
                spin_unlock_bh(&container->lock);
		wake_up_interruptible(&container->del_wq);
                return ret;
        }
        spin_unlock_bh(&container->lock);

        return ret;
}
EXPORT_SYMBOL(efcp_container_write);

static int efcp_receive(struct efcp * efcp,
                        struct du *  du)
{
        pdu_type_t    pdu_type;

        pdu_type = pci_type(&du->pci);
        if (pdu_type_is_control(pdu_type)) {
                if (!efcp->dtp || !efcp->dtp->dtcp) {
                        LOG_ERR("No DTCP instance available");
                        du_destroy(du);
                        return -1;
                }

                if (dtcp_common_rcv_control(efcp->dtp->dtcp, du))
                        return -1;

                return 0;
        }

        if (dtp_receive(efcp->dtp, du)) {
                LOG_ERR("DTP cannot receive this PDU");
                return -1;
        }

        return 0;
}

int efcp_container_receive(struct efcp_container * container,
                           cep_id_t                cep_id,
                           struct du *             du)
{
        struct efcp *      efcp;
        int                ret = 0;
        pdu_type_t         pdu_type;

        if (!is_cep_id_ok(cep_id)) {
                LOG_ERR("Bad cep-id, cannot write into container");
                du_destroy(du);
                return -1;
        }

        spin_lock_bh(&container->lock);
        efcp = efcp_imap_find(container->instances, cep_id);
        if (!efcp) {
                spin_unlock_bh(&container->lock);
                LOG_ERR("Cannot find the requested instance cep-id: %d",
                        cep_id);
                /* FIXME: It should call unknown_flow policy of EFCP */
                du_destroy(du);
                return -1;
        }
        if (efcp->state == EFCP_DEALLOCATED) {
                spin_unlock_bh(&container->lock);
                du_destroy(du);
                LOG_DBG("EFCP already deallocated");
                return 0;
        }
        atomic_inc(&efcp->pending_ops);

        pdu_type = pci_type(&du->pci);
        if (pdu_type == PDU_TYPE_DT &&
        		efcp->connection->destination_cep_id < 0) {
        	/* Check that the destination cep-id is set to avoid races,
        	 * otherwise set it*/
        	efcp->connection->destination_cep_id = pci_cep_source(&du->pci);
        }
        spin_unlock_bh(&container->lock);

        ret = efcp_receive(efcp, du);

        spin_lock_bh(&container->lock);
        if (atomic_dec_and_test(&efcp->pending_ops) &&
        		efcp->state == EFCP_DEALLOCATED) {
                spin_unlock_bh(&container->lock);
		wake_up_interruptible(&container->del_wq);
                return ret;
        }
        spin_unlock_bh(&container->lock);

        return ret;
}
EXPORT_SYMBOL(efcp_container_receive);

static bool is_candidate_connection_ok(const struct connection * connection)
{
        /* FIXME: Add checks for policy params */

        if (!connection                                      ||
            !is_cep_id_ok(connection->source_cep_id) ||
            !is_port_id_ok(connection->port_id))
                return false;

        return true;
}

int efcp_enable_write(struct efcp * efcp)
{
        if (!efcp                 ||
            !efcp->user_ipcp      ||
            !efcp->user_ipcp->ops ||
            !efcp->user_ipcp->data) {
                LOG_ERR("No EFCP or user IPCP provided");
                return -1;
        }

        if (efcp->user_ipcp->ops->enable_write(efcp->user_ipcp->data,
                                               efcp->connection->port_id)) {
                return -1;
        }

        return 0;
}

int efcp_disable_write(struct efcp * efcp)
{
        if (!efcp                 ||
            !efcp->user_ipcp      ||
            !efcp->user_ipcp->ops ||
            !efcp->user_ipcp->data) {
                LOG_ERR("No user IPCP provided");
                return -1;
        }

        if (efcp->user_ipcp->ops->disable_write(efcp->user_ipcp->data,
                                                efcp->connection->port_id)) {
                return -1;
        }

        return 0;
}
EXPORT_SYMBOL(efcp_disable_write);

/* FIXME This function has not been ported yet to use the DTCP policy set
 * parameters in place of struct dtcp_config. This because the code
 * tries to access "connection parameters" that are defined as DTCP
 * policies (in RINA) even if DTCP is not present. This has to
 * be fixed, because the DTCP policy set can exist only if DTCP
 * is present.
 */
cep_id_t efcp_connection_create(struct efcp_container * container,
                                struct ipcp_instance *  user_ipcp,
                                address_t               src_addr,
                                address_t               dst_addr,
                                port_id_t               port_id,
                                qos_id_t                qos_id,
                                cep_id_t                src_cep_id,
                                cep_id_t                dst_cep_id,
                                struct dtp_config *     dtp_cfg,
                                struct dtcp_config *    dtcp_cfg)
{
        struct efcp *       efcp;
        struct connection * connection;
        cep_id_t            cep_id;
        struct dtcp *       dtcp;
        struct cwq *        cwq;
        struct rtxq *       rtxq;
        uint_t              mfps, mfss;
        timeout_t           mpl, a, r = 0, tr = 0;
        struct dtp_ps       * dtp_ps;
        bool                dtcp_present;
        struct rttq *       rttq;
        struct delim * delim;

        if (!container) {
                LOG_ERR("Bogus container passed, bailing out");
                return cep_id_bad();
        }

        connection = connection_create();
        if (!connection)
                return cep_id_bad();

        ASSERT(dtp_cfg);

        connection->destination_address = dst_addr;
        connection->source_address = src_addr;
        connection->port_id = port_id;
        connection->qos_id = qos_id;
        connection->source_cep_id = src_cep_id;
        connection->destination_cep_id = dst_cep_id;

        efcp = efcp_create();
        if (!efcp) {
                connection_destroy(connection);
                return cep_id_bad();
        }

        efcp->user_ipcp = user_ipcp;

        cep_id = cidm_allocate(container->cidm);
        if (!is_cep_id_ok(cep_id)) {
                LOG_ERR("CIDM generated wrong CEP ID");
                efcp_destroy(efcp);
                return cep_id_bad();
        }

        /* We must ensure that the DTP is instantiated, at least ... */
        efcp->container = container;
        connection->source_cep_id = cep_id;
        if (!is_candidate_connection_ok((const struct connection *) connection)) {
                LOG_ERR("Bogus connection passed, bailing out");
                efcp_destroy(efcp);
                return cep_id_bad();
        }

        efcp->connection = connection;

	if (robject_rset_init_and_add(&efcp->robj,
				      &efcp_rtype,
				      container->rset,
				      "%d",
				      cep_id)) {
		LOG_ERR("Could not add connection tp sysfs");
                efcp_destroy(efcp);
                return cep_id_bad();
	}

        if (container->config->dt_cons->dif_frag) {
        	delim = delim_create(efcp, &efcp->robj);
        	if (!delim){
        		LOG_ERR("Problems creating delimiting module");
                        efcp_destroy(efcp);
                        return cep_id_bad();
        	}

        	delim->max_fragment_size =
        			container->config->dt_cons->max_pdu_size -
        			pci_calculate_size(container->config, PDU_TYPE_DT)
        			- 1;

        	/* TODO, allow selection of delimiting policy set name */
                if (delim_select_policy_set(delim, "", RINA_PS_DEFAULT_NAME)) {
                        LOG_ERR("Could not load delimiting PS %s",
                        	RINA_PS_DEFAULT_NAME);
                        delim_destroy(delim);
                        efcp_destroy(efcp);
                        return cep_id_bad();
                }

        	efcp->delim = delim;
        }

        /* FIXME: dtp_create() takes ownership of the connection parameter */
	efcp->dtp = dtp_create(efcp, container->rmt, dtp_cfg, &efcp->robj);
        if (!efcp->dtp) {
                efcp_destroy(efcp);
                return cep_id_bad();
        }

        dtcp = NULL;
        rcu_read_lock();
        dtp_ps = dtp_ps_get(efcp->dtp);
        a = dtp_ps->initial_a_timer;
        dtcp_present = dtp_ps->dtcp_present;
        rcu_read_unlock();
        if (dtcp_present) {
                dtcp = dtcp_create(efcp->dtp,
                                   container->rmt,
                                   dtcp_cfg,
				   &efcp->robj);
                if (!dtcp) {
                        efcp_destroy(efcp);
                        return cep_id_bad();
                }

                efcp->dtp->dtcp = dtcp;
        }

        if (dtcp_window_based_fctrl(dtcp_cfg) ||
            dtcp_rate_based_fctrl(dtcp_cfg)) {
                cwq = cwq_create();
                if (!cwq) {
                        LOG_ERR("Failed to create closed window queue");
                        efcp_destroy(efcp);
                        return cep_id_bad();
                }
                efcp->dtp->cwq = cwq;
        }

        if (dtcp_rtx_ctrl(dtcp_cfg)) {
                rtxq = rtxq_create(efcp->dtp, container->rmt, container,
                		   dtcp_cfg, cep_id);
                if (!rtxq) {
                        LOG_ERR("Failed to create rexmsn queue");
                        efcp_destroy(efcp);
                        return cep_id_bad();
                }
                efcp->dtp->rtxq = rtxq;
                efcp->dtp->rttq = NULL;
        } else {
        	rttq = rttq_create();
        	if (!rttq) {
        		LOG_ERR("Failed to create RTT queue");
        		efcp_destroy(efcp);
        		return cep_id_bad();
        	}
        	efcp->dtp->rttq = rttq;
        	efcp->dtp->rtxq = NULL;
        }

        efcp->dtp->efcp = efcp;

        /* FIXME: This is crap and have to be rethinked */

        /* FIXME: max pdu and sdu sizes are not stored anywhere. Maybe add them
         * in connection... For the time being are equal to max_pdu_size in
         * dif configuration
         */
        mfps = container->config->dt_cons->max_pdu_size;
        mfss = container->config->dt_cons->max_pdu_size;
        mpl  = container->config->dt_cons->max_pdu_life;
        if (dtcp && dtcp_rtx_ctrl(dtcp_cfg)) {
                tr = dtcp_initial_tr(dtcp_cfg);
                /* tr = msecs_to_jiffies(tr); */
                /* FIXME: r should be passed and must be a bound */
                r  = dtcp_data_retransmit_max(dtcp_cfg) * tr;
        }

        LOG_DBG("DT SV initialized with:");
        LOG_DBG("  MFPS: %d, MFSS: %d",   mfps, mfss);
        LOG_DBG("  A: %d, R: %d, TR: %d", a, r, tr);

        if (dtp_sv_init(efcp->dtp, dtcp_rtx_ctrl(dtcp_cfg),
                        dtcp_window_based_fctrl(dtcp_cfg),
                        dtcp_rate_based_fctrl(dtcp_cfg),
			mfps, mfss, mpl, a, r, tr)) {
                LOG_ERR("Could not init dtp_sv");
                efcp_destroy(efcp);
                return cep_id_bad();
        }

        /***/

        spin_lock_bh(&container->lock);
        if (efcp_imap_add(container->instances,
                          cep_id,
                          efcp)) {
                spin_unlock_bh(&container->lock);
                LOG_ERR("Cannot add a new instance into container %pK",
                        container);

                efcp_destroy(efcp);
                return cep_id_bad();
        }
        spin_unlock_bh(&container->lock);

        LOG_DBG("Connection created ("
                "Source address %d,"
                "Destination address %d, "
                "Destination cep-id %d, "
                "Source cep-id %d)",
                connection->source_address,
                connection->destination_address,
                connection->destination_cep_id,
                connection->source_cep_id);

        return cep_id;
}
EXPORT_SYMBOL(efcp_connection_create);

int efcp_connection_destroy(struct efcp_container * container,
                            cep_id_t                id)
{
        struct efcp * efcp;
	int retval;

        LOG_DBG("EFCP connection destroy called");

        /* FIXME: should wait 3*delta-t before destroying the connection */

        if (!container) {
                LOG_ERR("Bogus container passed, bailing out");
                return -1;
        }
        if (!is_cep_id_ok(id)) {
                LOG_ERR("Bad cep-id, cannot destroy connection");
                return -1;
        }

        spin_lock_bh(&container->lock);
        efcp = efcp_imap_find(container->instances, id);
        if (!efcp) {
                spin_unlock_bh(&container->lock);
                LOG_ERR("Cannot find instance %d in container %pK",
                        id, container);
                return -1;
        }

        if (efcp_imap_remove(container->instances, id)) {
                spin_unlock_bh(&container->lock);
                LOG_ERR("Cannot remove instance %d from container %pK",
                        id, container);
                return -1;
        }
        efcp->state = EFCP_DEALLOCATED;
	if (atomic_read(&efcp->pending_ops) != 0) {
		spin_unlock_bh(&container->lock);
		retval = wait_event_interruptible(container->del_wq,
						  atomic_read(&efcp->pending_ops) == 0 &&
						  efcp->state == EFCP_DEALLOCATED);
		if (retval != 0)
			LOG_ERR("EFCP destroy even interrupted (%d)", retval);
               	if (efcp_destroy(efcp)) {
               	        LOG_ERR("Cannot destroy instance %d, instance lost", id);
               	        return -1;
               	}
		return 0;
	}
        spin_unlock_bh(&container->lock);
        if (efcp_destroy(efcp)) {
        	LOG_ERR("Cannot destroy instance %d, instance lost", id);
        	return -1;
        }
        return 0;
}
EXPORT_SYMBOL(efcp_connection_destroy);

int efcp_connection_update(struct efcp_container * container,
                           cep_id_t                from,
                           cep_id_t                to)
{
        struct efcp * efcp;

        if (!container) {
                LOG_ERR("Bogus container passed, bailing out");
                return -1;
        }
        if (!is_cep_id_ok(from)) {
                LOG_ERR("Bad from cep-id, cannot update connection");
                return -1;
        }
        if (!is_cep_id_ok(to)) {
                LOG_ERR("Bad to cep-id, cannot update connection");
                return -1;
        }

        spin_lock_bh(&container->lock);
        efcp = efcp_imap_find(container->instances, from);
        if (!efcp) {
                spin_unlock_bh(&container->lock);
                LOG_ERR("Cannot get instance %d from container %pK",
                        from, container);
                return -1;
        }
        if (atomic_read(&efcp->pending_ops) == 0 &&
        		efcp->state == EFCP_DEALLOCATED) {
                spin_unlock_bh(&container->lock);
                if (efcp_destroy(efcp)) {
                        LOG_ERR("Cannot destroy instance %d, instance lost", from);
                        return -1;
                }
                return 0;
        }
        efcp->connection->destination_cep_id = to;
        spin_unlock_bh(&container->lock);

        LOG_DBG("Connection updated");
        LOG_DBG("  Source address:     %d",
        	efcp->connection->source_address);
        LOG_DBG("  Destination address %d",
        	efcp->connection->destination_address);
        LOG_DBG("  Destination cep id: %d",
        	efcp->connection->destination_cep_id);
        LOG_DBG("  Source cep id:      %d",
        	efcp->connection->source_cep_id);

        return 0;
}
EXPORT_SYMBOL(efcp_connection_update);

int efcp_connection_modify(struct efcp_container * cont,
			   cep_id_t		   cep_id,
			   address_t               src,
			   address_t               dst)
{
	struct efcp * efcp;

	if (!cont) {
		LOG_ERR("Bogus container passed, bailing out");
		return -1;
	}

	spin_lock_bh(&cont->lock);
	efcp = efcp_imap_find(cont->instances, cep_id);
	if (!efcp) {
		spin_unlock_bh(&cont->lock);
		LOG_ERR("Cannot get instance %d from container %pK",
				cep_id, cont);
		return -1;
	}
	if (atomic_read(&efcp->pending_ops) == 0 &&
			efcp->state == EFCP_DEALLOCATED) {
		spin_unlock_bh(&cont->lock);
		if (efcp_destroy(efcp)) {
			LOG_ERR("Cannot destroy instance %d, instance lost",
				cep_id);
			return -1;
		}
		return 0;
	}
	efcp->connection->source_address = src;
	efcp->connection->destination_address = dst;
	spin_unlock_bh(&cont->lock);

	return 0;
}
EXPORT_SYMBOL(efcp_connection_modify);

int efcp_enqueue(struct efcp * efcp,
                 port_id_t     port,
                 struct du *   du)
{
	struct delim_ps * delim_ps = NULL;
	struct du_list_item * next_du = NULL;

        if (!efcp->user_ipcp) {
        	LOG_ERR("Flow is being deallocated, dropping SDU");
        	du_destroy(du);
        	return -1;
        }

        /* Reassembly goes here */
        if (efcp->delim) {
		delim_ps = container_of(rcu_dereference(efcp->delim->base.ps),
						        struct delim_ps,
						        base);

		if (delim_ps->delim_process_udf(delim_ps, du,
						efcp->delim->rx_dus)) {
			LOG_ERR("Error processing EFCP UDF by delimiting");
			du_list_clear(efcp->delim->rx_dus, true);
			return -1;
		}

	        list_for_each_entry(next_du, &(efcp->delim->rx_dus->dus),
	        		    next) {
	                if (efcp->user_ipcp->ops->du_enqueue(efcp->user_ipcp->data,
	                                                     port,
	                                                     next_du->du)) {
	                        LOG_ERR("Upper ipcp could not enqueue sdu to port: %d", port);
	                        return -1;
	                }
	        }

	        du_list_clear(efcp->delim->rx_dus, false);

		return 0;
        }

        if (efcp->user_ipcp->ops->du_enqueue(efcp->user_ipcp->data,
                                             port,
                                             du)) {
                LOG_ERR("Upper ipcp could not enqueue sdu to port: %d", port);
                return -1;
        }

        return 0;
}

struct efcp_imap *
efcp_container_get_instances(struct efcp_container *efcpc)
{
	return efcpc->instances;
}
EXPORT_SYMBOL(efcp_container_get_instances);

int efcp_address_change(struct efcp_container * efcpc,
			address_t new_address)
{
        if (!efcpc) {
                LOG_ERR("Bogus efcp passed, bailing out");
                return -1;
        }

        spin_lock_bh(&efcpc->lock);
        efcp_imap_address_change(efcpc->instances, new_address);
        spin_unlock_bh(&efcpc->lock);

	return 0;
}
EXPORT_SYMBOL(efcp_address_change);

/*
 * KFA (Kernel Flow Allocator)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
 *        
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
#include <linux/kfifo.h>

#define RINA_PREFIX "kfa"

#include "logs.h"
#include "debug.h"
#include "utils.h"
#include "fidm.h"
#include "kfa.h"
#include "kipcm-utils.h"


struct kfa {
        spinlock_t    lock;
        struct fidm * fidm;

        struct {
		struct ipcp_fmap * pending;
		struct ipcp_pmap * committed;
	} flows;
};

struct ipcp_flow {
        port_id_t              port_id;
        struct ipcp_instance * ipc_process;
        struct kfifo           sdu_ready;
        wait_queue_head_t      wait_queue;
        struct efcp *          efcp;
};

struct kfa * kfa_create(void)
{
        struct kfa * instance;

        instance = rkzalloc(sizeof(*instance), GFP_KERNEL);
        if (!instance)
                return NULL;
        
        instance->fidm = fidm_create();
        if (!instance->fidm) {
                rkfree(instance);
                return NULL;
        }

        instance->flows.pending = ipcp_fmap_create();
        if (!instance->flows.pending) {
		if (fidm_destroy(instance->fidm)) {
			/* FIXME: What could we do here ? */
		}
		rkfree(instance);
		return NULL;
	}

        instance->flows.committed = ipcp_pmap_create();
	if (!instance->flows.committed) {
		if (fidm_destroy(instance->fidm)) {
			/* FIXME: What could we do here ? */
		}
		if (ipcp_fmap_destroy(instance->flows.pending)) {
			/* FIXME> What could we do here? */
		}
		rkfree(instance);
		return NULL;
	}

        spin_lock_init(&instance->lock);

        return instance;
}

int kfa_destroy(struct kfa * instance)
{
        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }

        fidm_destroy(instance->fidm);

        /* FIXME: Destroy all the pending flows */
	ASSERT(ipcp_fmap_empty(instance->flows.pending));
        ipcp_fmap_destroy(instance->flows.pending);

        /* FIXME: Destroy all the committed flows */
	ASSERT(ipcp_pmap_empty(instance->flows.committed));
        ipcp_pmap_destroy(instance->flows.committed);
        rkfree(instance);

        return 0;
}

flow_id_t kfa_flow_create(struct kfa * instance)
{

	struct ipcp_flow * flow;
	flow_id_t          fid;

        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return flow_id_bad();
        }

        ASSERT(instance->fidm);

        spin_lock(&instance->lock);
	
	fid = fidm_allocate(instance->fidm);
	if (!is_flow_id_ok(fid)){
                LOG_ERR("Cannot get a flow-id");
		spin_unlock(&instance->lock);
		return flow_id_bad();
	}
	
	flow = rkzalloc(sizeof(*flow), GFP_KERNEL);
	if (!flow) {
		spin_unlock(&instance->lock);
		return flow_id_bad();
	}

	if (!ipcp_fmap_add(instance->flows.pending, fid, flow)) {
                LOG_ERR("Could not map Flow and Flow ID");
		rkfree(flow);
		spin_unlock(&instance->lock);
		return flow_id_bad();
	}

        spin_unlock(&instance->lock);

	return fid;

}
EXPORT_SYMBOL(kfa_flow_create);

int kfa_flow_bind(struct kfa * 		 instance,
                  flow_id_t    		 fid,
                  port_id_t    		 pid,
                  struct ipcp_instance * ipc_process,
                  ipc_process_id_t       ipc_id)
{
	struct ipcp_flow * flow;

        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }
        if (!is_flow_id_ok(fid)) {
                LOG_ERR("Bogus flow-id, bailing out");
                return -1;
        }
        if (!is_port_id_ok(pid)) {
                LOG_ERR("Bogus port-id, bailing out");
                return -1;
        }
        if (!ipc_process) {
		LOG_ERR("Bogus ipc process instance passed, bailing out");
		return -1;
	}

        spin_lock(&instance->lock);

	/* FIXME: What about pending flows ? */
	if (ipcp_pmap_find(instance->flows.committed, pid)) {
		LOG_ERR("Flow on port-id %d already exists", pid);
		spin_unlock(&instance->lock);
		return -1;
	}

	flow = rkzalloc(sizeof(*flow), GFP_KERNEL);
	if (!flow) {
		spin_unlock(&instance->lock);
		return -1;
	}

	init_waitqueue_head(&flow->wait_queue);

	flow->port_id     = pid;
	flow->ipc_process = ipc_process;

	if (kfifo_alloc(&flow->sdu_ready, PAGE_SIZE, GFP_KERNEL)) {
		LOG_ERR("Couldn't create the sdu-ready queue for "
			"flow on port-id %d", pid);
		rkfree(flow);
		spin_unlock(&instance->lock);
		return -1;
	}

	/* FIXME: What about pending flows ? */
	if (ipcp_pmap_add(instance->flows.committed, pid, flow, ipc_id)) {
		kfifo_free(&flow->sdu_ready);
		rkfree(flow);
		spin_unlock(&instance->lock);
		return -1;
	}

        spin_unlock(&instance->lock);

        return 0;
}
EXPORT_SYMBOL(kfa_flow_bind);

flow_id_t kfa_flow_unbind(struct kfa * instance,
                          port_id_t    id)
{
        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }
        if (!is_port_id_ok(id)) {
                LOG_ERR("Bogus port-id, bailing out");
                return -1;
        }

        spin_lock(&instance->lock);
        LOG_MISSING;
        spin_unlock(&instance->lock);

        return flow_id_bad();
}
EXPORT_SYMBOL(kfa_flow_unbind);

int kfa_flow_destroy(struct kfa * instance,
                     flow_id_t    id)
{
        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }
        if (!is_flow_id_ok(id)) {
                LOG_ERR("Bogus flow-id, bailing out");
                return -1;
        }

        spin_lock(&instance->lock);
        LOG_MISSING;
        spin_unlock(&instance->lock);

        return -1;
}
EXPORT_SYMBOL(kfa_flow_destroy);

int kfa_remove_all_for_id(struct kfa * instance, ipc_process_id_t id)
{
	if (!instance) {
		LOG_ERR("Bogus instance passed, bailing out");
		return -1;
	}

	if (ipcp_pmap_remove_all_for_id(instance->flows.committed, id)) {
		return -1;
	}
	return 0;
}
EXPORT_SYMBOL(kfa_remove_all_for_id);

int kfa_flow_sdu_write(struct kfa *  instance,
                       port_id_t     id,
                       struct sdu *  sdu)
{
	struct ipcp_flow *     flow;
	struct ipcp_instance * ipcp;

        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }
        if (!is_port_id_ok(id)) {
                LOG_ERR("Bogus port-id, bailing out");
                return -1;
        }
        if (!is_sdu_ok(sdu)) {
                LOG_ERR("Bogus port-id, bailing out");
                return -1;
        }

        //spin_lock(&instance->lock);
        flow = ipcp_pmap_find(instance->flows.committed, id);
	if (!flow) {
		LOG_ERR("There is no flow bound to port-id %d", id);
		//spin_unlock(&instance->lock);
		return -1;
	}

	ipcp = flow->ipc_process;
	ASSERT(instance);
	if (ipcp->ops->sdu_write(ipcp->data, id, sdu)) {
		LOG_ERR("Couldn't write SDU on port-id %d", id);
		//spin_unlock(&instance->lock);
		return -1;
	}
        //spin_unlock(&instance->lock);

        return 0;
}

struct sdu * kfa_flow_sdu_read(struct kfa *  instance,
                               port_id_t     id)
{
        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return NULL;
        }
        if (!is_port_id_ok(id)) {
                LOG_ERR("Bogus port-id, bailing out");
                return NULL;
        }

        spin_lock(&instance->lock);
        LOG_MISSING;
        spin_unlock(&instance->lock);

        return NULL;
}
int kfa_sdu_post(struct kfa * instance,
                 port_id_t    id,
                 struct sdu * sdu)
{
	struct ipcp_flow * flow;
	unsigned int       avail;

	if (!instance) {
		LOG_ERR("Bogus kfa instance passed, cannot post SDU");
		return -1;
	}
	if (!is_port_id_ok(id)) {
		LOG_ERR("Bogus port-id, bailing out");
		return -1;
	}
	if (!sdu || !is_sdu_ok(sdu)) {
		LOG_ERR("Bogus parameters passed, bailing out");
		return -1;
	}

	LOG_DBG("Posting SDU of size %zd to port-id %d ",
		sdu->buffer->size, id);


	spin_lock(&instance->lock);
	flow = ipcp_pmap_find(instance->flows.committed, id);
	if (!flow) {
		LOG_ERR("There is no flow bound to port-id %d", id);
		spin_unlock(&instance->lock);
		return -1;
	}

	avail = kfifo_avail(&flow->sdu_ready);
	if (avail < (sdu->buffer->size + sizeof(size_t))) {
		LOG_ERR("There is no space in the port-id %d fifo", id);
		spin_unlock(&instance->lock);
		return -1;
	}

	if (kfifo_in(&flow->sdu_ready,
		     &sdu->buffer->size,
		     sizeof(size_t)) != sizeof(size_t)) {
		LOG_ERR("Could not write %zd bytes into port-id %d fifo",
			sizeof(size_t), id);
		spin_unlock(&instance->lock);
		return -1;
	}
	if (kfifo_in(&flow->sdu_ready,
		     sdu->buffer->data,
		     sdu->buffer->size) != sdu->buffer->size) {
		LOG_ERR("Could not write %zd bytes into port-id %d fifo",
			sdu->buffer->size, id);
		spin_unlock(&instance->lock);
		return -1;
	}

	LOG_DBG("SDU posted");

	wake_up_interruptible(&flow->wait_queue);

	LOG_DBG("Sleeping read syscall should be working now");

	return 0;
}
EXPORT_SYMBOL(kfa_sdu_post);

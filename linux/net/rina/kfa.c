/*
 * KFA (Kernel Flow Allocator)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
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

/* For sdu_ready */
#include <linux/kfifo.h>

/* For wait_queue */
#include <linux/sched.h>
#include <linux/wait.h>

#define RINA_PREFIX "kfa"

#include "logs.h"
#include "debug.h"
#include "utils.h"
#include "pidm.h"
#include "kfa.h"
#include "kfa-utils.h"

struct kfa {
        spinlock_t    lock;
        struct pidm * pidm;
        struct kfa_pmap * flows;
};

enum flow_state {
        PORT_STATE_NULL = 1,
        PORT_STATE_PENDING,
        PORT_STATE_ALLOCATED
};

struct ipcp_flow {
        port_id_t              port_id;

        enum flow_state         state;

        struct ipcp_instance *  ipc_process;

        /* FIXME: To be wiped out */
        struct kfifo           sdu_ready;
        wait_queue_head_t      wait_queue;
};

struct kfa * kfa_create(void)
{
        struct kfa * instance;

        instance = rkzalloc(sizeof(*instance), GFP_KERNEL);
        if (!instance)
                return NULL;

        instance->pidm = pidm_create();
        if (!instance->pidm) {
                rkfree(instance);
                return NULL;
        }

        instance->flows = kfa_pmap_create();

        if (!instance->flows) {
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

        /* FIXME: Destroy all the committed flows */
        ASSERT(kfa_pmap_empty(instance->flows));
        kfa_pmap_destroy(instance->flows);

        pidm_destroy(instance->pidm);
        rkfree(instance);

        return 0;
}

port_id_t kfa_flow_create(struct kfa *     instance,
                          ipc_process_id_t id)
{
        struct ipcp_flow * flow;
        port_id_t          pid;

        IRQ_BARRIER;

        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return port_id_bad();
        }

        spin_lock(&instance->lock);

        if (!instance->pidm) {
                LOG_ERR("This instance doesn't have a PIDM");
                spin_unlock(&instance->lock);
                return port_id_bad();
        }

        pid = pidm_allocate(instance->pidm);
        if (!is_port_id_ok(pid)) {
                LOG_ERR("Cannot get a port-id");
                spin_unlock(&instance->lock);
                return port_id_bad();
        }

        flow = rkzalloc(sizeof(*flow), GFP_ATOMIC);
        if (!flow) {
                pidm_release(instance->pidm, pid);
                spin_unlock(&instance->lock);
                return port_id_bad();
        }

        flow->state = PORT_STATE_PENDING;

        init_waitqueue_head(&flow->wait_queue);

        if (kfa_pmap_add_gfp(GFP_ATOMIC, instance->flows, pid, flow, id)) {
                LOG_ERR("Could not map Flow and Port ID");
                pidm_release(instance->pidm, pid);
                rkfree(flow);
                spin_unlock(&instance->lock);
                return port_id_bad();
        }

        spin_unlock(&instance->lock);

        return pid;

}
EXPORT_SYMBOL(kfa_flow_create);

int kfa_flow_bind(struct kfa *           instance,
                  port_id_t              pid,
                  struct ipcp_instance * ipc_process,
                  ipc_process_id_t       ipc_id)
{
        struct ipcp_flow * flow;

        IRQ_BARRIER;

        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }
        if (!is_port_id_ok(pid)) {
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
        /* FIXME: Check the ipc-id */

        spin_lock(&instance->lock);

        flow = kfa_pmap_find(instance->flows, pid);
        if (!flow) {
                LOG_ERR("The flow with port-id %d is not pending, "
                        "cannot commit it", pid);
                spin_unlock(&instance->lock);
                return -1;
        }
        if (flow->state != PORT_STATE_PENDING) {
                LOG_ERR("Flow on port-id %d already committed", pid);
                spin_unlock(&instance->lock);
                return -1;
        }

        flow->state = PORT_STATE_ALLOCATED;
        flow->ipc_process = ipc_process;

        if (kfifo_alloc(&flow->sdu_ready, PAGE_SIZE, GFP_ATOMIC)) {
                LOG_ERR("Couldn't create the sdu-ready queue for "
                        "flow on port-id %d", pid);
                rkfree(flow);
                spin_unlock(&instance->lock);
                return -1;
        }

        LOG_DBG("Flow bound to port id %d with waitqueue %pK",
                pid, &flow->wait_queue);

        spin_unlock(&instance->lock);

        return 0;
}
EXPORT_SYMBOL(kfa_flow_bind);

int kfa_flow_destroy(struct kfa * instance,
                     port_id_t    id)
{
        struct ipcp_flow * flow;

        IRQ_BARRIER;

        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }
        if (!is_port_id_ok(id)) {
                LOG_ERR("Bogus flow-id, bailing out");
                return -1;
        }

        spin_lock(&instance->lock);

        flow = kfa_pmap_find(instance->flows, id);
        if (!flow) {
                LOG_ERR("There is no flow created with port_id %d", id);
                spin_unlock(&instance->lock);
                return -1;
        }

        kfifo_free(&flow->sdu_ready);
        rkfree(flow);

        if (kfa_pmap_remove(instance->flows, id)) {
                LOG_ERR("Could not remove pending flow with port_id %d", id);
                spin_unlock(&instance->lock);
                return -1;
        }

        if (pidm_release(instance->pidm, id)) {
                LOG_ERR("Could not release pid %d from the map", id);
                spin_unlock(&instance->lock);
                return -1;
        }

        spin_unlock(&instance->lock);

        return 0;
}
EXPORT_SYMBOL(kfa_flow_destroy);

/* FIXME: To be removed ASAP */
int kfa_remove_all_for_id(struct kfa *     instance,
                          ipc_process_id_t id)
{
        IRQ_BARRIER;

        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }

        if (kfa_pmap_remove_all_for_id(instance->flows, id))
                return -1;

        return 0;
}
EXPORT_SYMBOL(kfa_remove_all_for_id);

int kfa_flow_sdu_write(struct kfa * instance,
                       port_id_t    id,
                       struct sdu * sdu)
{
        struct ipcp_flow *     flow;
        struct ipcp_instance * ipcp;

        IRQ_BARRIER;

        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }
        if (!is_sdu_ok(sdu)) {
                LOG_ERR("Bogus port-id, bailing out");
                return -1;
        }

        spin_lock(&instance->lock);

        if (!is_port_id_ok(id)) {
                LOG_ERR("Bogus port-id, bailing out");
                spin_unlock(&instance->lock);
                return -1;
        }

        flow = kfa_pmap_find(instance->flows, id);
        if (!flow) {
                LOG_ERR("There is no flow bound to port-id %d", id);
                spin_unlock(&instance->lock);
                return -1;
        }

        while (flow->state != PORT_STATE_ALLOCATED) {
                LOG_DBG("Write is going to sleep on wait queue %pK",
                        &flow->wait_queue);
                spin_unlock(&instance->lock);
                wait_event_interruptible(flow->wait_queue,
                                         (flow->state ==
                                          PORT_STATE_ALLOCATED));

                spin_lock(&instance->lock);
                LOG_DBG("Write woken up");

                flow = kfa_pmap_find(instance->flows, id);
                if (!flow) {
                        LOG_ERR("There is no flow bound to port-id %d anymore",
                                id);
                        spin_unlock(&instance->lock);
                        return -1;
                }
        }

        ipcp = flow->ipc_process;
        if (!ipcp) {
                spin_unlock(&instance->lock);
                return -1;
        }
        if (ipcp->ops->sdu_write(ipcp->data, id, sdu)) {
                LOG_ERR("Couldn't write SDU on port-id %d", id);
                spin_unlock(&instance->lock);
                return -1;
        }

        spin_unlock(&instance->lock);

        return 0;
}

static int ready_queue_not_empty(struct kfifo * sdu_ready)
{
        return (!kfifo_is_empty(sdu_ready));
}

int kfa_flow_sdu_read(struct kfa *  instance,
                      port_id_t     id,
                      struct sdu ** sdu)
{
        struct ipcp_flow * flow;

        IRQ_BARRIER;

        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }
        if (!is_port_id_ok(id)) {
                LOG_ERR("Bogus port-id, bailing out");
                return -1;
        }

        LOG_DBG("Trying to read SDU from port-id %d", id);
        spin_lock(&instance->lock);
        flow = kfa_pmap_find(instance->flows, id);
        if (!flow) {
                LOG_ERR("There is no flow bound to port-id %d", id);
                spin_unlock(&instance->lock);
                return -1;
        }
        while (kfifo_is_empty(&flow->sdu_ready)) {
                LOG_DBG("Going to sleep on wait queue %pK", &flow->wait_queue);
                spin_unlock(&instance->lock);
                wait_event_interruptible(flow->wait_queue,
                                         ready_queue_not_empty(&flow->sdu_ready));

                spin_lock(&instance->lock);
                LOG_DBG("Woken up");

                flow = kfa_pmap_find(instance->flows, id);
                if (!flow) {
                        LOG_ERR("There is no flow bound to port-id %d anymore",
                                id);
                        spin_unlock(&instance->lock);
                        return -1;
                }
        }

        if (kfifo_out(&flow->sdu_ready, sdu, sizeof(struct sdu *)) <
            sizeof(struct sdu *)) {
                LOG_ERR("There is not enough data in port-id %d fifo",
                        id);
                spin_unlock(&instance->lock);
                return -1;
        }

        spin_unlock(&instance->lock);

        return 0;
}

int kfa_sdu_post(struct kfa * instance,
                 port_id_t    id,
                 struct sdu * sdu)
{
        struct ipcp_flow *  flow;
        wait_queue_head_t * wq;

        IRQ_BARRIER;

        /*
         * FIXME: kfa_sdu_post copies the contents of the SDU in the kfifo,
         * which forces the reader of the port id to create a new SDU from
         * this data. This is too much of a burden for the two operations and
         * should change, probably through a fifo of pointers.
         */
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
        flow = kfa_pmap_find(instance->flows, id);
        if (!flow) {
                LOG_ERR("There is no flow bound to port-id %d", id);
                spin_unlock(&instance->lock);
                return -1;
        }

        if (kfifo_avail(&flow->sdu_ready) < (sizeof(struct sdu *))) {
                LOG_ERR("There is no space in the port-id %d fifo", id);
                spin_unlock(&instance->lock);
                return -1;
        }
        if (kfifo_in(&flow->sdu_ready,
                     &sdu,
                     sizeof(struct sdu *)) != sizeof(struct sdu *)) {
                LOG_ERR("Could not write %zd bytes into port-id %d fifo",
                        sizeof(struct sdu *), id);
                spin_unlock(&instance->lock);
                return -1;
        }

        wq = &flow->wait_queue;

        LOG_DBG("Wait queue %pK, next: %pK, prev: %pK",
                wq, wq->task_list.next, wq->task_list.prev);

        spin_unlock(&instance->lock);

        LOG_DBG("SDU posted");

        wake_up(wq);

        LOG_DBG("Sleeping read syscall should be working now");

        return 0;
}
EXPORT_SYMBOL(kfa_sdu_post);

struct ipcp_flow * kfa_find_flow_by_pid(struct kfa * instance, port_id_t pid)
{ return kfa_pmap_find(instance->flows, pid); }
EXPORT_SYMBOL(kfa_find_flow_by_pid);

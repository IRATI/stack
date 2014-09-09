/*
 * KFA (Kernel Flow Allocator)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
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
#include "rmt.h"

struct kfa {
        struct mutex     lock;
        struct pidm *     pidm;
        struct kfa_pmap * flows;
};

enum flow_state {
        PORT_STATE_NULL        = 1,
        PORT_STATE_PENDING,
        PORT_STATE_ALLOCATED,
        PORT_STATE_DEALLOCATED
};

struct ipcp_flow {
        port_id_t              port_id;

        enum flow_state        state;
        struct ipcp_instance * ipc_process;
        struct rfifo *         sdu_ready;
        wait_queue_head_t      wait_queue;
        atomic_t               readers;
        atomic_t               writers;
        atomic_t               posters;
        struct rmt *           rmt;
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

        mutex_init(&instance->lock);

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

int kfa_flow_create(struct kfa *     instance,
                    ipc_process_id_t id,
                    port_id_t        pid)
{
        struct ipcp_flow * flow;

        IRQ_BARRIER;

        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }
        if (!is_port_id_ok(pid)) {
                LOG_ERR("Bogus PID passed, bailing out");
                return-1;
        }

        flow = rkzalloc(sizeof(*flow), GFP_KERNEL);
        if (!flow)
                return -1;

        flow->state = PORT_STATE_PENDING;
        atomic_set(&flow->readers, 0);
        atomic_set(&flow->writers, 0);
        atomic_set(&flow->posters, 0);

        init_waitqueue_head(&flow->wait_queue);

        mutex_lock(&instance->lock);

        if (kfa_pmap_add_ni(instance->flows, pid, flow, id)) {
                LOG_ERR("Could not map flow and port-id %d", pid);
                rkfree(flow);

                mutex_unlock(&instance->lock);
                return -1;
        }

        mutex_unlock(&instance->lock);

        return 0;
}
EXPORT_SYMBOL(kfa_flow_create);

port_id_t kfa_port_id_reserve(struct kfa *     instance,
                              ipc_process_id_t id)
{
        port_id_t pid;

        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return port_id_bad();
        }

        mutex_lock(&instance->lock);

        if (!instance->pidm) {
                mutex_unlock(&instance->lock);
                LOG_ERR("This KFA instance doesn't have a PIDM");
                return port_id_bad();
        }

        pid = pidm_allocate(instance->pidm);
        if (!is_port_id_ok(pid)) {
                mutex_unlock(&instance->lock);
                LOG_ERR("Cannot get a port-id");
                return port_id_bad();
        }

        mutex_unlock(&instance->lock);

        return pid;
}
EXPORT_SYMBOL(kfa_port_id_reserve);

int  kfa_port_id_release(struct kfa * instance,
                         port_id_t    port_id)
{
        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }

        if (!is_port_id_ok(port_id)) {
                LOG_ERR("Bogus port-id, bailing out");
                return -1;
        }

        mutex_lock(&instance->lock);

        if (!instance->pidm) {
                LOG_ERR("This KFA instance doesn't have a PIDM");
                mutex_unlock(&instance->lock);
                return -1;
        }

        if (pidm_release(instance->pidm, port_id)) {
                LOG_ERR("Could not release pid %d from the map", port_id);
                mutex_unlock(&instance->lock);
                return -1;
        }

        mutex_unlock(&instance->lock);

        return 0;
}
EXPORT_SYMBOL(kfa_port_id_release);

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
                LOG_ERR("Bogus port-id, bailing out");
                return -1;
        }
        if (!ipc_process) {
                LOG_ERR("Bogus ipc process instance passed, bailing out");
                return -1;
        }
        /* FIXME: Check the ipc-id */

        mutex_lock(&instance->lock);

        flow = kfa_pmap_find(instance->flows, pid);
        if (!flow) {
                LOG_ERR("The flow with port-id %d is not pending, "
                        "cannot commit it", pid);
                mutex_unlock(&instance->lock);
                return -1;
        }

        if (flow->state != PORT_STATE_PENDING) {
                LOG_ERR("Flow on port-id %d already committed", pid);
                mutex_unlock(&instance->lock);
                return -1;
        }

        flow->state     = PORT_STATE_ALLOCATED;
        flow->sdu_ready = rfifo_create_ni();
        if (!flow->sdu_ready) {
                kfa_pmap_remove(instance->flows, pid);
                rkfree(flow);
                mutex_unlock(&instance->lock);
                return -1;
        }

        LOG_DBG("Flow bound to port-id %d with waitqueue %pK",
                pid, &flow->wait_queue);

        mutex_unlock(&instance->lock);

        return 0;
}
EXPORT_SYMBOL(kfa_flow_bind);

int kfa_flow_rmt_bind(struct kfa * instance,
                      port_id_t    pid,
                      struct rmt * rmt)
{
        struct ipcp_flow * flow;

        if (!instance)
                return -1;

        if (!is_port_id_ok(pid))
                return -1;

        mutex_lock(&instance->lock);

        flow = kfa_pmap_find(instance->flows, pid);
        if (!flow) {
                LOG_ERR("The flow with port-id %d does not exist, "
                        "cannot bind rmt", pid);
                mutex_unlock(&instance->lock);
                return -1;
        }
        flow->rmt = rmt;

        mutex_unlock(&instance->lock);

        return 0;
}
EXPORT_SYMBOL(kfa_flow_rmt_bind);

int kfa_flow_rmt_unbind(struct kfa * instance,
                        port_id_t    pid)
{
        struct ipcp_flow * flow;

        if (!instance)
                return -1;

        if (!is_port_id_ok(pid))
                return -1;

        mutex_lock(&instance->lock);

        flow = kfa_pmap_find(instance->flows, pid);
        if (!flow) {
                LOG_ERR("The flow with port-id %d does not exist, "
                        "cannot unbind rmt", pid);
                mutex_unlock(&instance->lock);
                return -1;
        }
        flow->rmt = NULL;

        mutex_unlock(&instance->lock);

        return 0;
}
EXPORT_SYMBOL(kfa_flow_rmt_unbind);

/* NOTE: Add instance-locking IFF exporting to API */
static int kfa_flow_destroy(struct kfa *       instance,
                            struct ipcp_flow * flow,
                            port_id_t          id)
{
        struct ipcp_instance * ipcp;
        int                    retval = 0;

        ASSERT(flow);

        LOG_DBG("We are destroying flow %d", id);

        ipcp = flow->ipc_process;

        /* FIXME: Should we ASSERT() here ? */
        if (!flow->sdu_ready)
                LOG_WARN("Instance %pK SDU-ready FIFO is NULL", instance);
        else
                if (rfifo_destroy(flow->sdu_ready,
                                  (void (*) (void *)) sdu_destroy)) {
                        LOG_ERR("Flow %d FIFO has not been destroyed", id);
                        retval = -1;
                }

        if (kfa_pmap_remove(instance->flows, id)) {
                LOG_ERR("Could not remove pending flow with port-id %d", id);
                retval = -1;
        }

        if (pidm_release(instance->pidm, id)) {
                LOG_ERR("Could not release pid %d from the map", id);
                retval = -1;
        }

        if (flow->rmt) {
                if (rmt_n1port_unbind(flow->rmt, id)) {
                        LOG_ERR("Could not unbind port-id %d from "
                                "RMT queues", id);
                        retval = -1;
                }
        }
        rkfree(flow);

        ASSERT(ipcp);
        ASSERT(ipcp->ops);

        if (ipcp->ops->flow_destroy)
                if (ipcp->ops->flow_destroy(ipcp->data, id)) {
                        LOG_ERR("Problems destroying the flow "
                                "on port-id %d", id);
                        retval = -1;
                }

        return retval;
}

int kfa_flow_deallocate(struct kfa * instance,
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

        mutex_lock(&instance->lock);

        flow = kfa_pmap_find(instance->flows, id);
        if (!flow) {
                LOG_ERR("There is no flow created with port-id %d", id);
                mutex_unlock(&instance->lock);
                return -1;
        }

        flow->state = PORT_STATE_DEALLOCATED;

        if ((atomic_read(&flow->readers) == 0) &&
            (atomic_read(&flow->writers) == 0) &&
            (atomic_read(&flow->posters) == 0)) {
                if (kfa_flow_destroy(instance, flow, id))
                        LOG_ERR("Could not destroy the flow correctly");
                mutex_unlock(&instance->lock);
                return 0;
        }

        mutex_unlock(&instance->lock);

        LOG_DBG("Waking up all!");
        wake_up_interruptible_all(&flow->wait_queue);

        return 0;
}
EXPORT_SYMBOL(kfa_flow_deallocate);

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
        int                    retval = 0;

        IRQ_BARRIER;

        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                sdu_destroy(sdu);
                return -1;
        }
        if (!is_port_id_ok(id)) {
                LOG_ERR("Bogus port-id, bailing out");
                sdu_destroy(sdu);
                return -1;
        }
        if (!sdu_is_ok(sdu)) {
                LOG_ERR("Bogus sdu, bailing out");
                sdu_destroy(sdu);
                return -1;
        }

        LOG_DBG("Trying to write SDU to port-id %d", id);

        mutex_lock(&instance->lock);

        flow = kfa_pmap_find(instance->flows, id);
        if (!flow) {
                LOG_ERR("There is no flow bound to port-id %d", id);
                mutex_unlock(&instance->lock);
                sdu_destroy(sdu);
                return -1;
        }
        if (flow->state == PORT_STATE_DEALLOCATED) {
                LOG_ERR("Flow with port-id %d is already deallocated", id);
                mutex_unlock(&instance->lock);
                sdu_destroy(sdu);
                return -1;
        }

        atomic_inc(&flow->writers);

        while (flow->state == PORT_STATE_PENDING) {
                mutex_unlock(&instance->lock);

                LOG_DBG("Going to sleep on wait queue %pK (writing)",
                        &flow->wait_queue);
                retval = wait_event_interruptible(flow->wait_queue,
                                                  (flow->state !=
                                                   PORT_STATE_PENDING));
                LOG_DBG("Write woken up (%d)", retval);

                if (retval) {
                        LOG_DBG("Wait-event interrupted (%d)", retval);
                        if (signal_pending(current)) {
                                LOG_DBG("A signal is pending");
#if 0
                                LOG_DBG("A signal is pending "
                                        "(sig = 0x%08zx%08zx)",
                                        current->pending.signal.sig[0],
                                        current->pending.signal.sig[1]);
#endif
                        }
                }

                mutex_lock(&instance->lock);

                flow = kfa_pmap_find(instance->flows, id);
                if (!flow) {
                        mutex_unlock(&instance->lock);
                        sdu_destroy(sdu);

                        LOG_ERR("There is no flow bound to port-id %d anymore",
                                id);
                        return -1;
                }

                if (retval) {
                        sdu_destroy(sdu);
                        goto finish;
                }
        }

        ipcp = flow->ipc_process;
        if (!ipcp) {
                retval = -1;
                sdu_destroy(sdu);
                goto finish;
        }

        ASSERT(ipcp->ops);
        ASSERT(ipcp->ops->sdu_write);

        if (ipcp->ops->sdu_write(ipcp->data, id, sdu)) {
                LOG_ERR("Couldn't write SDU on port-id %d", id);
                retval = -1;
                goto finish;
        }

 finish:
        LOG_DBG("Finishing (write)");

        if (atomic_dec_and_test(&flow->writers) &&
            (atomic_read(&flow->readers) == 0)  &&
            (atomic_read(&flow->posters) == 0)  &&
            (flow->state == PORT_STATE_DEALLOCATED))
                if (kfa_flow_destroy(instance, flow, id))
                        LOG_ERR("Could not destroy the flow correctly");

        mutex_unlock(&instance->lock);

        return retval;
}

static bool queue_ready(struct ipcp_flow * flow)
{
        ASSERT(flow);

        LOG_DBG("Queue-ready check called");

        if (flow->state == PORT_STATE_DEALLOCATED) {
                LOG_DBG("Queue-ready / "
                        "flow state is PORT_STATE_DEALLOCATED");
                return true;
        }

        if (flow->state != PORT_STATE_PENDING &&
            !rfifo_is_empty(flow->sdu_ready)) {
                if (flow->state != PORT_STATE_PENDING)
                        LOG_DBG("Queue-ready / "
                                "flow state is not PORT_STATE_PENDING");
                if (!rfifo_is_empty(flow->sdu_ready))
                        LOG_DBG("Queue-ready / "
                                "rfifo is not empty");
                return true;
        }

        return false;
}

int kfa_flow_sdu_read(struct kfa *  instance,
                      port_id_t     id,
                      struct sdu ** sdu)
{
        struct ipcp_flow * flow;
        int                retval = 0;

        IRQ_BARRIER;

        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }
        if (!is_port_id_ok(id)) {
                LOG_ERR("Bogus port-id, bailing out");
                return -1;
        }
        if (!sdu) {
                LOG_ERR("Bogus output sdu parameter passed, bailing out");
                return -1;
        }

        LOG_DBG("Trying to read SDU from port-id %d", id);

        mutex_lock(&instance->lock);

        flow = kfa_pmap_find(instance->flows, id);
        if (!flow) {
                LOG_ERR("There is no flow bound to port-id %d", id);
                mutex_unlock(&instance->lock);
                return -1;
        }
        if (flow->state == PORT_STATE_DEALLOCATED) {
                LOG_ERR("Flow with port-id %d is already deallocated", id);
                mutex_unlock(&instance->lock);
                return -1;
        }

        atomic_inc(&flow->readers);

        while (flow->state == PORT_STATE_PENDING ||
               rfifo_is_empty(flow->sdu_ready)) {
                mutex_unlock(&instance->lock);

                LOG_DBG("Going to sleep on wait queue %pK (reading)",
                        &flow->wait_queue);
                retval = wait_event_interruptible(flow->wait_queue,
                                                  queue_ready(flow));
                LOG_DBG("Read woken up (%d)", retval);

                if (retval) {
                        LOG_DBG("Wait-event interrupted (%d)", retval);
                        if (signal_pending(current)) {
                                LOG_DBG("A signal is pending");
#if 0
                                LOG_DBG("A signal is pending "
                                        "(sig = 0x%08zx%08zx)",
                                        current->pending.signal.sig[0],
                                        current->pending.signal.sig[1]);
#endif
                        }
                }

                mutex_lock(&instance->lock);

                flow = kfa_pmap_find(instance->flows, id);
                if (!flow) {
                        mutex_unlock(&instance->lock);

                        LOG_ERR("There is no flow bound to port-id %d anymore",
                                id);
                        return -1;
                }

                if (retval)
                        goto finish;

                if (flow->state == PORT_STATE_DEALLOCATED) {
                        if (rfifo_is_empty(flow->sdu_ready)) {
                                retval = 0;
                                goto finish;
                        }
                        break;
                }
        }

        if (rfifo_is_empty(flow->sdu_ready)) {
                retval = -1;
                goto finish;
        }

        *sdu = rfifo_pop(flow->sdu_ready);
        if (!sdu_is_ok(*sdu)) {
                LOG_ERR("There is not a valid in port-id %d fifo", id);
                retval = -1;
        }

 finish:
        LOG_DBG("Finishing (read)");

        if (atomic_dec_and_test(&flow->readers) &&
            (atomic_read(&flow->writers) == 0)  &&
            (atomic_read(&flow->posters) == 0)  &&
            (flow->state == PORT_STATE_DEALLOCATED))
                if (kfa_flow_destroy(instance, flow, id))
                        LOG_ERR("Could not destroy the flow correctly");

        mutex_unlock(&instance->lock);

        return retval;
}

int kfa_sdu_post(struct kfa * instance,
                 port_id_t    id,
                 struct sdu * sdu)
{
        struct ipcp_flow *  flow;
        wait_queue_head_t * wq;
        int                 retval = 0;

        if (!instance) {
                LOG_ERR("Bogus kfa instance passed, cannot post SDU");
                return -1;
        }
        if (!is_port_id_ok(id)) {
                LOG_ERR("Bogus port-id, bailing out");
                return -1;
        }
        if (!sdu_is_ok(sdu)) {
                LOG_ERR("Bogus parameters passed, bailing out");
                return -1;
        }

        LOG_DBG("Posting SDU to port-id %d ", id);

        mutex_lock(&instance->lock);
        flow = kfa_pmap_find(instance->flows, id);
        if (!flow) {
                LOG_ERR("There is no flow bound to port-id %d", id);
                mutex_unlock(&instance->lock);
                return -1;
        }

        if (flow->state == PORT_STATE_DEALLOCATED) {
                LOG_ERR("Flow with port-id %d is already deallocated", id);
                mutex_unlock(&instance->lock);
                sdu_destroy(sdu);
                return -1;
        }

        atomic_inc(&flow->posters);

        if (!flow->rmt) {
                if (rfifo_push_ni(flow->sdu_ready, sdu)) {
                        LOG_ERR("Could not write %zd bytes into "
                                "port-id %d fifo",
                                sizeof(struct sdu *), id);
                        retval = -1;
                        goto finish;
                }
                goto finish;
        } else {
                if (rmt_receive(flow->rmt, sdu, id)) {
                        LOG_ERR("Could not post SDU into the RMT");
                        retval = -1;
                        goto finish;
                }
        }
 finish:
        if (atomic_dec_and_test(&flow->posters) &&
            (atomic_read(&flow->writers) == 0)  &&
            (atomic_read(&flow->readers) == 0)  &&
            (flow->state == PORT_STATE_DEALLOCATED)) {
                if (kfa_flow_destroy(instance, flow, id))
                        LOG_ERR("Could not destroy the flow correctly");
                flow = NULL;
        }

        mutex_unlock(&instance->lock);

        if (flow && !flow->rmt && (retval == 0)) {
                wq = &flow->wait_queue;
                ASSERT(wq);

                LOG_DBG("Wait queue %pK, next: %pK, prev: %pK",
                        wq, wq->task_list.next, wq->task_list.prev);

                wake_up_interruptible_all(wq);
                LOG_DBG("SDU posted");
                LOG_DBG("Sleeping read syscall should be working now");
        }

        return retval;
}
EXPORT_SYMBOL(kfa_sdu_post);

#if 0
struct ipcp_flow * kfa_flow_find_by_pid(struct kfa * instance, port_id_t pid)
{
        struct ipcp_flow * tmp;

        if (!instance)
                return NULL;

        mutex_lock(&instance->lock);
        tmp = kfa_pmap_find(instance->flows, pid);
        mutex_unlock(&instance->lock);

        return tmp;
}
EXPORT_SYMBOL(kfa_flow_find_by_pid);
#endif

int kfa_sdu_post_to_user_space(struct kfa * instance,
                               struct sdu * sdu,
                               port_id_t    to)
{
        if (!instance) {
                LOG_ERR("Bogus kfa instance passed, cannot post SDU");
                return -1;
        }
        if (!is_port_id_ok(to)) {
                LOG_ERR("Bogus port-id, bailing out");
                return -1;
        }
        if (!sdu_is_ok(sdu)) {
                LOG_ERR("Bogus parameters passed, bailing out");
                return -1;
        }

        LOG_DBG("Posting SDU to queue for user space in port-id %d ", to);

        LOG_MISSING;

        return 0;
}
EXPORT_SYMBOL(kfa_sdu_post_to_user_space);

int kfa_flow_ipcp_bind(struct kfa *           instance,
                       port_id_t              pid,
                       struct ipcp_instance * ipcp)
{
        struct ipcp_flow * flow;

        if (!instance)
                return -1;

        if (!is_port_id_ok(pid))
                return -1;

        if (!ipcp)
                return -1;

        mutex_lock(&instance->lock);

        flow = kfa_pmap_find(instance->flows, pid);
        if (!flow) {
                LOG_ERR("There is no flow with port-id %d, "
                        "cannot bind it", pid);
                mutex_unlock(&instance->lock);
                return -1;
        }
        flow->ipc_process = ipcp;

        mutex_unlock(&instance->lock);

        return 0;
}

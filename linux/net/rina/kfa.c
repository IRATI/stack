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

struct kfa {
        spinlock_t             lock;
        struct pidm *          pidm;
        struct kfa_pmap *      flows;
        struct ipcp_instance * ipcp;
        struct list_head       list;
};

enum flow_state {
        PORT_STATE_NULL        = 1,
        PORT_STATE_PENDING,
        PORT_STATE_ALLOCATED,
        PORT_STATE_DEALLOCATED,
        PORT_STATE_DISABLED
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
};

struct ipcp_instance_data {
        struct kfa * kfa;
};

port_id_t kfa_port_id_reserve(struct kfa *     instance,
                              ipc_process_id_t id)
{
        port_id_t pid;

        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return port_id_bad();
        }

        spin_lock(&instance->lock);

        if (!instance->pidm) {
                spin_unlock(&instance->lock);
                LOG_ERR("This KFA instance doesn't have a PIDM");
                return port_id_bad();
        }

        pid = pidm_allocate(instance->pidm);
        if (!is_port_id_ok(pid)) {
                spin_unlock(&instance->lock);
                LOG_ERR("Cannot get a port-id");
                return port_id_bad();
        }

        spin_unlock(&instance->lock);

        return pid;
}
EXPORT_SYMBOL(kfa_port_id_reserve);

int  kfa_port_id_release(struct kfa * instance,
                         port_id_t    port_id)
{
        struct ipcp_flow * flow;

        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }

        if (!is_port_id_ok(port_id)) {
                LOG_ERR("Bogus port-id, bailing out");
                return -1;
        }

        spin_lock(&instance->lock);

        /* To avoid releasing the port if it is used by a flow in the kfa (to */
        /* an app) which will be automatically destroyed when the flow is     */
        /* unbound by the provider ipcp and the writers/readers/posters in kfa*/
        /* are 0. This avoids allocating the freed port again before the kfa  */
        /* finally destroys everything. */
        flow = kfa_pmap_find(instance->flows, port_id);
        if (flow) {
                spin_unlock(&instance->lock);
                return 0;
        }

        if (!instance->pidm) {
                LOG_ERR("This KFA instance doesn't have a PIDM");
                spin_unlock(&instance->lock);
                return -1;
        }

        if (pidm_release(instance->pidm, port_id)) {
                LOG_ERR("Could not release pid %d from the map", port_id);
                spin_unlock(&instance->lock);
                return -1;
        }

        spin_unlock(&instance->lock);

        return 0;
}
EXPORT_SYMBOL(kfa_port_id_release);

/* NOTE: Add instance-locking IFF exporting to API */
static int kfa_flow_destroy(struct kfa *       instance,
                            struct ipcp_flow * flow,
                            port_id_t          id)
{
        int                    retval = 0;

        ASSERT(flow);

        LOG_DBG("We are destroying flow %d", id);

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

        rkfree(flow);

        return retval;
}

static int kfa_flow_deallocate(struct ipcp_instance_data * data,
                               port_id_t    id)
{
        struct ipcp_flow * flow;
        struct kfa *       instance;

        IRQ_BARRIER;

        if (!data) {
                LOG_ERR("Bogus ipcp data passed, bailing out");
                return -1;
        }

        instance = data->kfa;
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
                LOG_ERR("There is no flow created with port-id %d", id);
                spin_unlock(&instance->lock);
                return -1;
        }

        flow->state = PORT_STATE_DEALLOCATED;

        if ((atomic_read(&flow->readers) == 0) &&
            (atomic_read(&flow->writers) == 0) &&
            (atomic_read(&flow->posters) == 0)) {
                if (kfa_flow_destroy(instance, flow, id))
                        LOG_ERR("Could not destroy the flow correctly");
                spin_unlock(&instance->lock);
                return 0;
        }

        spin_unlock(&instance->lock);

        LOG_DBG("Waking up all!");
        wake_up_interruptible_all(&flow->wait_queue);

        return 0;
}
EXPORT_SYMBOL(kfa_flow_deallocate);

static bool ok_write(struct ipcp_flow * flow)
{
        return (!(flow->state == PORT_STATE_PENDING  ||
                 flow->state == PORT_STATE_DISABLED) ||
                 flow->state == PORT_STATE_DEALLOCATED);
}

/*
 * NOTE: This function does NOT take the mutex, because it will be called only
 *       during a write operation and then the mutex will be already taken.
 */
static int disable_write(struct ipcp_instance_data * data, port_id_t id)
{
        struct ipcp_flow * flow;
        struct kfa *        instance;

        if (!data) {
                LOG_ERR("Bogus ipcp data instance passed, can't enable pid");
                return -1;
        }

        instance = data->kfa;
        if (!instance) {
                LOG_ERR("Bogus kfa instance passed, can't enable pid");
                return -1;
        }
        if (!is_port_id_ok(id)) {
                LOG_ERR("Bogus port-id, bailing out");
                return -1;
        }
        LOG_DBG("DISABLED write op");

        flow = kfa_pmap_find(instance->flows, id);
        if (!flow) {
                LOG_ERR("There is no flow bound to port-id %d", id);
                return -1;
        }

        if (flow->state == PORT_STATE_DEALLOCATED) {
                LOG_DBG("Flow with port-id %d is already deallocated", id);
                return 0;
        }

        flow->state = PORT_STATE_DISABLED;

        LOG_DBG("IPCP notified CWQ exhausted");

        return 0;
}

static int enable_write(struct ipcp_instance_data * data, port_id_t id)
{
        struct ipcp_flow *  flow;
        struct kfa *        instance;
        wait_queue_head_t * wq;

        if (!data) {
                LOG_ERR("Bogus ipcp data instance passed, can't enable pid");
                return -1;
        }

        instance = data->kfa;
        if (!instance) {
                LOG_ERR("Bogus kfa instance passed, can't enable pid");
                return -1;
        }
        if (!is_port_id_ok(id)) {
                LOG_ERR("Bogus port-id, bailing out");
                return -1;
        }

        LOG_DBG("ENABLED write op");

        spin_lock(&instance->lock);
        flow = kfa_pmap_find(instance->flows, id);
        if (!flow) {
                spin_unlock(&instance->lock);
                LOG_ERR("There is no flow bound to port-id %d", id);
                return -1;
        }

        if (flow->state == PORT_STATE_DEALLOCATED) {
                spin_unlock(&instance->lock);
                LOG_DBG("Flow with port-id %d is already deallocated", id);
                return 0;
        }
        if (flow->state == PORT_STATE_DISABLED) {
                flow->state = PORT_STATE_ALLOCATED;
                wq = &flow->wait_queue;
                spin_unlock(&instance->lock);
                LOG_DBG("IPCP notified CWQ enabled but port deallocated");
                LOG_DBG("Enabled port id");
                wake_up_interruptible_all(wq);
                return 0;
        }
        spin_unlock(&instance->lock);
        LOG_DBG("IPCP notified CWQ enabled");

        return 0;
}

int kfa_flow_sdu_write(struct ipcp_instance_data * data,
                       port_id_t                   id,
                       struct sdu *                sdu)
{
        struct ipcp_flow *     flow;
        struct ipcp_instance * ipcp;
        struct kfa *           instance;
        int                    retval = 0;

        IRQ_BARRIER;

        if (!data) {
                LOG_ERR("Bogus ipcp data passed, bailing out");
                sdu_destroy(sdu);
                return -1;
        }

        instance = data->kfa;
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

        spin_lock(&instance->lock);

        flow = kfa_pmap_find(instance->flows, id);
        if (!flow) {
                LOG_ERR("There is no flow bound to port-id %d", id);
                spin_unlock(&instance->lock);
                sdu_destroy(sdu);
                return -1;
        }
        if (flow->state == PORT_STATE_DEALLOCATED) {
                LOG_ERR("Flow with port-id %d is already deallocated", id);
                spin_unlock(&instance->lock);
                sdu_destroy(sdu);
                return -1;
        }

        atomic_inc(&flow->writers);

        while (!ok_write(flow)) {
                spin_unlock(&instance->lock);

                LOG_DBG("Going to sleep on wait queue %pK (writing)",
                        &flow->wait_queue);
                LOG_DBG("OK_write check called: %d", flow->state);
                retval = wait_event_interruptible(flow->wait_queue,
                                                  ok_write(flow));
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

                spin_lock(&instance->lock);

                flow = kfa_pmap_find(instance->flows, id);
                if (!flow) {
                        spin_unlock(&instance->lock);
                        sdu_destroy(sdu);

                        LOG_ERR("There is no flow bound to port-id %d anymore",
                                id);
                        return -1;
                }

                if (retval) {
                        sdu_destroy(sdu);
                        goto finish;
                }

                if (flow->state == PORT_STATE_DEALLOCATED) {
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

        spin_unlock(&instance->lock);
        if (ipcp->ops->sdu_write(ipcp->data, id, sdu)) {
                LOG_ERR("Couldn't write SDU on port-id %d", id);
                retval = -1;
                goto finish;
        }
        spin_lock(&instance->lock);

 finish:
        LOG_DBG("Finishing (write)");

        if (atomic_dec_and_test(&flow->writers) &&
            (atomic_read(&flow->readers) == 0)  &&
            (atomic_read(&flow->posters) == 0)  &&
            (flow->state == PORT_STATE_DEALLOCATED))
                if (kfa_flow_destroy(instance, flow, id))
                        LOG_ERR("Could not destroy the flow correctly");

        spin_unlock(&instance->lock);

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

        spin_lock(&instance->lock);

        flow = kfa_pmap_find(instance->flows, id);
        if (!flow) {
                LOG_ERR("There is no flow bound to port-id %d", id);
                spin_unlock(&instance->lock);
                return -1;
        }
        if (flow->state == PORT_STATE_DEALLOCATED) {
                LOG_ERR("Flow with port-id %d is already deallocated", id);
                spin_unlock(&instance->lock);
                return -1;
        }

        atomic_inc(&flow->readers);

        while (flow->state == PORT_STATE_PENDING ||
               rfifo_is_empty(flow->sdu_ready)) {
                spin_unlock(&instance->lock);

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

                spin_lock(&instance->lock);

                flow = kfa_pmap_find(instance->flows, id);
                if (!flow) {
                        spin_unlock(&instance->lock);

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

        spin_unlock(&instance->lock);

        return retval;
}

static int kfa_sdu_post(struct ipcp_instance_data * data,
                        port_id_t                   id,
                        struct sdu *                sdu)
{
        struct ipcp_flow *  flow;
        wait_queue_head_t * wq;
        struct kfa *        instance;
        int                 retval = 0;

        if (!data) {
                LOG_ERR("Bogus ipcp data instance passed, cannot post SDU");
                sdu_destroy(sdu);
                return -1;
        }

        instance = data->kfa;
        if (!instance) {
                LOG_ERR("Bogus kfa instance passed, cannot post SDU");
                sdu_destroy(sdu);
                return -1;
        }
        if (!is_port_id_ok(id)) {
                LOG_ERR("Bogus port-id, bailing out");
                sdu_destroy(sdu);
                return -1;
        }
        if (!sdu_is_ok(sdu)) {
                LOG_ERR("Bogus parameters passed, bailing out");
                sdu_destroy(sdu);
                return -1;
        }

        LOG_DBG("Posting SDU to port-id %d ", id);

        spin_lock(&instance->lock);
        flow = kfa_pmap_find(instance->flows, id);
        if (!flow) {
                LOG_ERR("There is no flow bound to port-id %d", id);
                spin_unlock(&instance->lock);
                sdu_destroy(sdu);
                return -1;
        }

        if (flow->state == PORT_STATE_DEALLOCATED) {
                LOG_ERR("Flow with port-id %d is already deallocated", id);
                spin_unlock(&instance->lock);
                sdu_destroy(sdu);
                return -1;
        }

        atomic_inc(&flow->posters);

        if (rfifo_push_ni(flow->sdu_ready, sdu)) {
                LOG_ERR("Could not write %zd bytes into "
                        "port-id %d fifo",
                        sizeof(struct sdu *), id);
                retval = -1;
        }

        if (atomic_dec_and_test(&flow->posters) &&
            (atomic_read(&flow->writers) == 0)  &&
            (atomic_read(&flow->readers) == 0)  &&
            (flow->state == PORT_STATE_DEALLOCATED)) {
                if (kfa_flow_destroy(instance, flow, id))
                        LOG_ERR("Could not destroy the flow correctly");
                flow = NULL;
        }

        spin_unlock(&instance->lock);

        if (flow && (retval == 0)) {
                wq = &flow->wait_queue;
                ASSERT(wq);

                LOG_DBG("Wait queue %pK, next: %pK, prev: %pK",
                        wq, wq->task_list.next, wq->task_list.prev);

                wake_up_interruptible(wq);
                LOG_DBG("SDU posted");
                LOG_DBG("Sleeping read syscall should be working now");
        }

        return retval;
}

#if 0
struct ipcp_flow * kfa_flow_find_by_pid(struct kfa * instance, port_id_t pid)
{
        struct ipcp_flow * tmp;

        if (!instance)
                return NULL;

        spin_lock(&instance->lock);
        tmp = kfa_pmap_find(instance->flows, pid);
        spin_unlock(&instance->lock);

        return tmp;
}
EXPORT_SYMBOL(kfa_flow_find_by_pid);
#endif

int kfa_flow_create(struct kfa *           instance,
                    port_id_t              pid,
                    struct ipcp_instance * ipcp)
{
        struct ipcp_flow * flow;

        IRQ_BARRIER;

        if (!instance)
                return -1;

        if (!is_port_id_ok(pid))
                return -1;

        if (!ipcp)
                return -1;

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

        atomic_set(&flow->readers, 0);
        atomic_set(&flow->writers, 0);
        atomic_set(&flow->posters, 0);

        init_waitqueue_head(&flow->wait_queue);

        flow->ipc_process = ipcp;

        flow->state     = PORT_STATE_PENDING;
        LOG_DBG("Flow PRE bound to port-id %d with waitqueue %pK",
                pid, &flow->wait_queue);

        spin_lock(&instance->lock);

        if (kfa_pmap_add_ni(instance->flows, pid, flow)) {
                LOG_ERR("Could not map flow and port-id %d", pid);
                rkfree(flow);

                spin_unlock(&instance->lock);
                return -1;
        }

        spin_unlock(&instance->lock);

        return 0;
}
EXPORT_SYMBOL(kfa_flow_create);

static int kfa_flow_ipcp_bind(struct ipcp_instance_data * data,
                              port_id_t                   pid,
                              struct ipcp_instance *      ipcp)
{
        struct ipcp_flow * flow;
        struct kfa * instance;

        IRQ_BARRIER;

        if (!data)
                return -1;

        instance = data->kfa;
        if (!instance)
                return -1;

        if (!is_port_id_ok(pid))
                return -1;

        if (!ipcp)
                return -1;

        IRQ_BARRIER;

        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }
        if (!is_port_id_ok(pid)) {
                LOG_ERR("Bogus PID passed, bailing out");
                return-1;
        }

        spin_lock(&instance->lock);
        flow = kfa_pmap_find(instance->flows, pid);
        if (!flow) {
                spin_unlock(&instance->lock);
                return -1;
        }

        flow->ipc_process = ipcp;

        flow->state     = PORT_STATE_ALLOCATED;
        flow->sdu_ready = rfifo_create_ni();
        if (!flow->sdu_ready) {
                kfa_pmap_remove(instance->flows, pid);
                rkfree(flow);
                spin_unlock(&instance->lock);
                return -1;
        }

        LOG_DBG("Flow bound to port-id %d with waitqueue %pK",
                pid, &flow->wait_queue);

        spin_unlock(&instance->lock);

        return 0;
}

struct ipcp_instance * kfa_ipcp_instance(struct kfa * instance)
{
        if (!instance)
                return NULL;

        return instance->ipcp;
}
EXPORT_SYMBOL(kfa_ipcp_instance);

static const struct name * kfa_name(struct ipcp_instance_data * data)
{ return NULL; }

static struct ipcp_instance_ops kfa_instance_ops = {
        .flow_allocate_request     = NULL,
        .flow_allocate_response    = NULL,
        .flow_deallocate           = NULL,
        .flow_binding_ipcp         = kfa_flow_ipcp_bind,
        .flow_unbinding_ipcp       = kfa_flow_deallocate,
        .application_register      = NULL,
        .application_unregister    = NULL,
        .assign_to_dif             = NULL,
        .update_dif_config         = NULL,
        .connection_create         = NULL,
        .connection_update         = NULL,
        .connection_destroy        = NULL,
        .connection_create_arrived = NULL,
        .sdu_enqueue               = kfa_sdu_post,
        .sdu_write                 = kfa_flow_sdu_write,
        .ipcp_name                 = kfa_name,
        .enable_write              = enable_write,
        .disable_write             = disable_write
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
                pidm_destroy(instance->pidm);
                rkfree(instance);
                return NULL;
        }

        instance->ipcp = rkzalloc(sizeof(struct ipcp_instance), GFP_KERNEL);
        if (!instance->ipcp) {
                pidm_destroy(instance->pidm);
                kfa_pmap_destroy(instance->flows);
                rkfree(instance);
                return NULL;
        }
        instance->ipcp->ops  = &kfa_instance_ops;
        instance->ipcp->data = rkzalloc(sizeof(struct ipcp_instance_data),
                                        GFP_KERNEL);
        if (!instance->ipcp->data) {
                LOG_ERR("Could not allocate memory for kfa ipcp "
                        "internal data");
                pidm_destroy(instance->pidm);
                kfa_pmap_destroy(instance->flows);
                rkfree(instance);
                return NULL;
        }
        instance->ipcp->data->kfa = instance;

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

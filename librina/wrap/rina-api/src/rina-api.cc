/*
 *  POSIX-like RINA API, wrapper for IRATI librina
 *
 *    Vincenzo Maffione   <v.maffione@nextworks.it>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <iostream>
#include <string>
#include <cassert>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <librina/librina.h>
#include <rina/api.h>

using namespace rina;
using namespace std;

extern "C"
{

#define APIDBG
#undef APIDBG

/*
 * Global variable, not protected by lock. Its purpose is to avoid calling
 * rina::initialize() for every API call. Actually, without a lock there is
 * is race condition and the function may be called twice. However,
 * if this really happens, rina::initialize() - which is internally protected
 * by a lock - will raise and exception that we can catch. The race is
 * therefore harmless. */
static int initialized = 0;

static int
librina_init(void)
{
        errno = 0; /* reset at the beginning of each API call */

        if (initialized) {
                return 0;
        }
        initialized = 1;
        try {
                rina::initialize("INFO", "/dev/null");
        } catch (rina::Exception &e) {
#ifdef APIDBG
                cout << __func__ << ": " << e.what() << endl;
#endif /* APIDBG */
        } catch (...) {
                /*
                 * We got an exception because librina is already
                 * initialized. The race happened, but it was harmless
                 * and there is nothing that we need to do.
                 */
        }
        return 0;
}

int
rina_open(void)
{
        if (librina_init()) {
                return -1;
        }

        /* The control file descriptor is process-wise, so we duplicate
         * it to provide a consistent API. However, a process should
         * not call rina_open() more than once, otherwise there would
         * be a race on reading the control messages. */
        return dup(ipcManager->getControlFd());
}

static IPCEvent *
wait_for_event(IPCEventType wtype, unsigned int wseqnum)
{
        /* Wait for events to come, forever. In the future we
         * could use select to add a timeout mechanism, and return
         * errno = ETIMEDOUT in case of timeout. */
        for (;;) {
                IPCEvent* event = ipcEventProducer->eventWait();
                bool match;

                if (!event) {
                        errno = ENXIO;
                        return NULL;
                }

                /* Exit the loop if the event matches what we asked for, or
                 * if we were not asked for anything in particular. */
                match = ((wtype == NO_EVENT || event->eventType == wtype) &&
                        (wseqnum == ~0U || event->sequenceNumber == wseqnum));

                switch (event->eventType) {

                /* Process here the events for which we only need some
                 * bookkeeping. */
                case REGISTER_APPLICATION_RESPONSE_EVENT: {
                        RegisterApplicationResponseEvent *resp;

                        resp = dynamic_cast<RegisterApplicationResponseEvent*>(event);

                        /* Update librina state */
                        if (resp->result) {
                                errno = EPERM;
                                ipcManager->withdrawPendingRegistration(
                                                        event->sequenceNumber);
                        } else {
                                ipcManager->commitPendingRegistration(
                                                        event->sequenceNumber,
                                                        resp->DIFName);
                        }
                        delete event;
                        event = NULL;
                        break;
                }

                case UNREGISTER_APPLICATION_RESPONSE_EVENT: {
                        UnregisterApplicationResponseEvent *resp;

                        resp = dynamic_cast<UnregisterApplicationResponseEvent*>(event);
                        ipcManager->appUnregistrationResult(
                                                event->sequenceNumber,
                                                resp->result == 0);
                        delete event;
                        event = NULL;
                        break;
                }

                case FLOW_DEALLOCATED_EVENT:
                        ipcManager->flowDeallocated(dynamic_cast<FlowDeallocatedEvent*>(event)->portId);
                        delete event;
                        event = NULL;
                        break;

                case DEALLOCATE_FLOW_RESPONSE_EVENT: {
                        DeallocateFlowResponseEvent *resp;

                        resp = dynamic_cast<DeallocateFlowResponseEvent*>(event);
                        ipcManager->flowDeallocationResult(resp->portId, resp->result == 0);
                        delete event;
                        event = NULL;
                        break;
                }

                /* Other events are returned to the caller, if they match. */
                case FLOW_ALLOCATION_REQUESTED_EVENT:
                case ALLOCATE_FLOW_REQUEST_RESULT_EVENT:
                        break;
                default:
                        break;
                }

                if (match) {
                        return event;
                }
        }

        return NULL;
}

/* Data structures used to implement:
 *    - the splitted call rina_flow_accept(RINA_F_NORESP)/rina_flow_respond():
 *              - a table for pending requests
 *              - a counter for handles.
 *    - the rina_flow_alloc() with RINA_F_NOWAIT
 *              - a table for pending requests
 *    - the rina_register() and rina_unregister() with RINA_F_NOWAIT
 *              - a table for pending requests
 */

struct RegPendingEvent {
        unsigned seqnum;
        rina::IPCEventType evtype;
};

static map<int, FlowRequestEvent *> pending_fre;
static int handle_next = 0;
static map<int, unsigned int> pending_fa;
static map<int, RegPendingEvent> pending_reg;
rina::Lockable split_lock;

static int
registration_complete(const RegPendingEvent &rpe)
{
        IPCEvent *event = wait_for_event(rpe.evtype, rpe.seqnum);

        assert(event == NULL);
        if (errno != 0) {
                return -1;
        }

        return 0;
}

int
rina_register_wait(int fd, int wfd)
{
        map<int, RegPendingEvent>::iterator mit;
        RegPendingEvent rpe;
        int ret = -1;

        (void)fd; /* The netlink socket file descriptor is used internally */
        if (librina_init()) {
                return -1;
        }

        split_lock.lock();
        mit = pending_reg.find(wfd);
        if (mit == pending_reg.end()) {
                split_lock.unlock();
                errno = EINVAL;
                return -1;
        }
        rpe = mit->second;
        pending_reg.erase(mit);
        split_lock.unlock();

        try {
                ret = registration_complete(rpe);
        } catch (rina::Exception &e) {
#ifdef APIDBG
                cout << __func__ << ": " << e.what() << endl;
#endif /* APIDBG */
                errno = ENXIO; /* IPC Manager Daemon is not running */
        } catch (...) {
                errno = ENOMEM;
        }

        return ret;
}

static void
str2apninfo(const string& str, ApplicationProcessNamingInformation &apni)
{
        std::string *vps[] = { &apni.processName, &apni.processInstance,
                               &apni.entityName, &apni.entityInstance };
        std::stringstream ss(str);

        for (int i = 0; i < 4 && getline(ss, *(vps[i]), '|'); i++) {
        }
}

int
rina_register(int fd, const char *dif_name, const char *local_appl, int flags)
{
        ApplicationRegistrationInformation ari;
        RegPendingEvent rpe;

        (void)fd; /* The netlink socket file descriptor is used internally */
        if (librina_init()) {
                return -1;
        }

        if (flags & ~RINA_F_NOWAIT) {
                errno = EINVAL;
                return -1;
        }

        ari.ipcProcessId = 0;  /* This is an application, not an IPC process */
        str2apninfo(string(local_appl), ari.appName);

        if (dif_name) {
                ari.applicationRegistrationType = APPLICATION_REGISTRATION_SINGLE_DIF;
                ari.difName = ApplicationProcessNamingInformation(
                                                string(dif_name), string());
        } else {
                ari.applicationRegistrationType = APPLICATION_REGISTRATION_ANY_DIF;
        }

        try {
                /* Issue a registration request. */
                rpe.seqnum = ipcManager->requestApplicationRegistration(ari);
                rpe.evtype = REGISTER_APPLICATION_RESPONSE_EVENT;

                if (flags & RINA_F_NOWAIT) {
                        int wfd = rina_open();

                        /* Store the seqnum in the pending registration
                         * mapped from a duplicate of the control
                         * file descriptor. */
                        split_lock.lock();
                        pending_reg[wfd] = rpe;
                        split_lock.unlock();

                        return wfd;
                }

                return registration_complete(rpe);

        } catch (rina::Exception &e) {
#ifdef APIDBG
                cout << __func__ << ": " << e.what() << endl;
#endif /* APIDBG */
                errno = ENXIO; /* IPC Manager Daemon is not running */
                return -1;
        } catch (...) {
                /* Operations can fail because of allocation failures. */
                errno = ENOMEM;
                return -1;
        }

        return 0;
}

int
rina_unregister(int fd, const char *dif_name, const char *local_appl, int flags)
{
        ApplicationProcessNamingInformation appi;
        ApplicationProcessNamingInformation difi;
        RegPendingEvent rpe;

        (void)fd; /* The netlink socket file descriptor is used internally */
        if (librina_init()) {
                return -1;
        }

        if (flags & ~RINA_F_NOWAIT) {
                errno = EINVAL;
                return -1;
        }

        difi = ApplicationProcessNamingInformation(string(dif_name), string());
        str2apninfo(string(local_appl), appi);

        try {
                /* Issue unregistration request. */
                rpe.seqnum = ipcManager->requestApplicationUnregistration(appi,
                                difi);
                rpe.evtype = UNREGISTER_APPLICATION_RESPONSE_EVENT;

                if (flags & RINA_F_NOWAIT) {
                        int wfd = rina_open();

                        /* Store the seqnum in the pending registration
                         * mapped from a duplicate of the control
                         * file descriptor. */
                        split_lock.lock();
                        pending_reg[wfd] = rpe;
                        split_lock.unlock();

                        return wfd;
                }

                return registration_complete(rpe);

        } catch (rina::Exception &e) {
#ifdef APIDBG
                cout << __func__ << ": " << e.what() << endl;
#endif /* APIDBG */
                errno = ENXIO; /* IPC Manager Daemon is not running */
                return -1;
        } catch (...) {
                /* Operations can fail because of allocation failures. */
                errno = ENOMEM;
                return -1;
        }

        return 0;
}

static void
remote_appl_fill(FlowRequestEvent *fre, char **remote_appl)
{
        if (remote_appl == NULL) {
                return;
        }

        *remote_appl = strdup(fre->remoteApplicationName.toString().c_str());
        if (*remote_appl == NULL) {
                throw std::bad_alloc();
                ipcManager->allocateFlowResponse(*fre,
                                /* result */ -1,
                                /* notifySource */ true,
                                /* blocking */ true);
        }
}

int
rina_flow_respond(int fd, int handle, int response)
{
        map<int, FlowRequestEvent *>::iterator mit;
        FlowRequestEvent *fre;
        FlowInformation flow;

        (void)fd; /* The netlink socket file descriptor is used internally */
        if (librina_init()) {
                return -1;
        }

        /* Look up the event in the pending table. */
        split_lock.lock();
        mit = pending_fre.find(handle);
        if (mit == pending_fre.end()) {
                split_lock.unlock();
                errno = EINVAL;
                return -1;
        }
        fre = mit->second;
        mit->second = NULL;
        pending_fre.erase(mit);
        split_lock.unlock();

        try {
                flow = ipcManager->allocateFlowResponse(*fre,
                                                /* result */ response,
                                                /* notifySource */ true,
                                                /* blocking */ true);
        } catch (rina::Exception &e) {
#ifdef APIDBG
                cout << __func__ << ": " << e.what() << endl;
#endif /* APIDBG */
                errno = ENXIO; /* IPC Manager Daemon is not running */
                flow.fd = -1;
        } catch (...) {
                errno = ENOMEM;
                flow.fd = -1;
        }
        delete fre;

        return flow.fd;
}

int
rina_flow_accept(int fd, char **remote_appl, struct rina_flow_spec *spec,
                 unsigned int flags)
{
        FlowInformation flow;
        IPCEvent *event = NULL;
        int retfd = -1;

        if (flags & ~RINA_F_NORESP) {
                errno = EINVAL;
                return -1;
        }

        if (remote_appl) {
                *remote_appl = NULL;
        }

        if (spec) {
                rina_flow_spec_default(spec);
        }

        (void)fd; /* The netlink socket file descriptor is used internally */
        if (librina_init()) {
                return -1;
        }

        try {
                FlowRequestEvent *fre;

                event = wait_for_event(FLOW_ALLOCATION_REQUESTED_EVENT, ~0U);
                if (event == NULL) {
                        /* This is an error, errno already set internally. */
                        return -1;
                }

                fre = dynamic_cast<FlowRequestEvent*>(event);
                assert(fre);

                remote_appl_fill(fre, remote_appl);

                if (spec) {
                        /* Convert fre->flowSpecification (FlowSpecification)
                         *           --> spec (struct rina_flow_spec) */
                        spec->max_sdu_gap = fre->flowSpecification.maxAllowableGap;
                        spec->avg_bandwidth = fre->flowSpecification.averageBandwidth;
                        spec->max_delay = fre->flowSpecification.delay;
                        spec->max_jitter = fre->flowSpecification.jitter;
                        spec->in_order_delivery = fre->flowSpecification.orderedDelivery;
                        spec->msg_boundaries = fre->flowSpecification.partialDelivery;
                }

                if (flags & RINA_F_NORESP) {
                        int handle;

                        /* Store the event in the pending table. */
                        split_lock.lock();
                        handle = handle_next ++;
                        if (handle_next < 0) { /* Overflow */
                                handle_next = 0;
                        }
                        pending_fre[handle] = fre;
                        split_lock.unlock();

                        return handle;
                }

                flow = ipcManager->allocateFlowResponse(*fre,
                                                /* result */ 0,
                                                /* notifySource */ true,
                                                /* blocking */ true);
                retfd = flow.fd;
        } catch (rina::Exception &e) {
#ifdef APIDBG
                cout << __func__ << ": " << e.what() << endl;
#endif /* APIDBG */
                errno = ENXIO; /* IPC Manager Daemon is not running */
        } catch (...) {
                errno = ENOMEM;
        }
        delete event;

        return retfd;
}

static FlowInformation
flow_alloc_complete(unsigned int seqnum)
{
        AllocateFlowRequestResultEvent *resp;
        FlowInformation flow;
        IPCEvent *event;

        event = wait_for_event(ALLOCATE_FLOW_REQUEST_RESULT_EVENT,
                                         seqnum);

        /*
         * Note that it may be resp->portId < 0, which means the
         * flow allocation request was denied. In this case the
         * commitPendingFlow() function will set flow.fd to -1,
         * and the same invalid fd is returned to the application,
         * which is what we want.
         */
        resp = dynamic_cast<AllocateFlowRequestResultEvent *>(event);

        try {
                flow = ipcManager->commitPendingFlow(resp->sequenceNumber,
                                                     resp->portId,
                                                     resp->difName);
                if (flow.fd < 0) {
                        errno = EPERM;
                }
        } catch (...) {
                delete event;
                throw;
        }

        delete event;

        return flow;
}

int
rina_flow_alloc(const char *dif_name, const char *local_appl,
                const char *remote_appl, const struct rina_flow_spec *flowspec,
                unsigned int flags)
{
        ApplicationProcessNamingInformation local_apni, remote_apni, dif_apni;
        FlowSpecification flowspec_i;
        struct rina_flow_spec spec;
        FlowInformation flow;
        unsigned int seqnum;

        if (librina_init()) {
                return -1;
        }

        if (flags & ~RINA_F_NOWAIT) {
                errno = EINVAL;
                return -1;
        }

        if (local_appl == NULL || remote_appl == NULL) {
                errno = EINVAL;
                return -1;
        }

        if (flowspec == NULL) {
                rina_flow_spec_default(&spec);
                flowspec = &spec;
        }

        /* Convert flowspec (struct rina_flow_spec) -->
         *         flowspec_i (FlowSpecification) */
        flowspec_i.maxAllowableGap = flowspec->max_sdu_gap;
        flowspec_i.averageBandwidth = flowspec->avg_bandwidth;
        flowspec_i.delay = flowspec->max_delay;
        flowspec_i.jitter = flowspec->max_jitter;
        flowspec_i.orderedDelivery = flowspec->in_order_delivery;
        flowspec_i.partialDelivery = flowspec->msg_boundaries;

        str2apninfo(string(local_appl), local_apni);
        str2apninfo(string(remote_appl), remote_apni);

        try {
                if (dif_name == NULL) {
                        seqnum = ipcManager->requestFlowAllocation(local_apni,
                                                                   remote_apni,
                                                                   flowspec_i);
                } else {
                        str2apninfo(string(dif_name), dif_apni);
                        seqnum = ipcManager->requestFlowAllocationInDIF(
                                                                local_apni,
                                                                remote_apni,
                                                                dif_apni,
                                                                flowspec_i);
                }

                if (flags & RINA_F_NOWAIT) {
                        int wfd = rina_open();

                        /* Store the seqnum in the pending flow allocation
                         * table, mapped from a duplicate of the control
                         * file descriptor. */
                        split_lock.lock();
                        pending_fa[wfd] = seqnum;
                        split_lock.unlock();

                        return wfd;
                }

                flow = flow_alloc_complete(seqnum);
        } catch (rina::Exception &e) {
#ifdef APIDBG
                cout << __func__ << ": " << e.what() << endl;
#endif /* APIDBG */
                errno = ENXIO; /* IPC Manager Daemon is not running */
                flow.fd = -1;
        } catch (...) {
                errno = ENOMEM;
                flow.fd = -1;
        }

        return flow.fd;
}

int
rina_flow_alloc_wait(int wfd)
{
        map<int, unsigned int>::iterator mit;
        FlowInformation flow;
        unsigned int seqnum;

        split_lock.lock();
        mit = pending_fa.find(wfd);
        if (mit == pending_fa.end()) {
                split_lock.unlock();
                errno = EINVAL;
                return -1;
        }
        seqnum = mit->second;
        pending_fa.erase(mit);
        split_lock.unlock();

        try {
                flow = flow_alloc_complete(seqnum);
        } catch (rina::Exception &e) {
#ifdef APIDBG
                cout << __func__ << ": " << e.what() << endl;
#endif /* APIDBG */
                errno = ENXIO; /* IPC Manager Daemon is not running */
                flow.fd = -1;
        } catch (...) {
                errno = ENOMEM;
                flow.fd = -1;
        }

        return flow.fd;
}

void
rina_flow_spec_default(struct rina_flow_spec *spec)
{
        memset(spec, 0, sizeof(*spec));
        spec->max_sdu_gap = -1;
        spec->avg_bandwidth = 0;
        spec->max_delay = 0;
        spec->max_jitter = 0;
        spec->in_order_delivery = 0;
        spec->msg_boundaries = 1;
}

}

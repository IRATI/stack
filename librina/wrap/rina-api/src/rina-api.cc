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
#include <rina-api/api.h>

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

int
rina_register(int fd, const char *dif_name, const char *local_appl)
{
        ApplicationRegistrationInformation ari;
        unsigned int seqnum;

        (void)fd; /* The netlink socket file descriptor is used internally */
        if (librina_init()) {
                return -1;
        }

        ari.ipcProcessId = 0;  /* This is an application, not an IPC process */
        ari.appName = ApplicationProcessNamingInformation(string(local_appl),
                                                          string());

        if (dif_name) {
                ari.applicationRegistrationType = APPLICATION_REGISTRATION_SINGLE_DIF;
                ari.difName = ApplicationProcessNamingInformation(
                                                string(dif_name), string());
        } else {
                ari.applicationRegistrationType = APPLICATION_REGISTRATION_ANY_DIF;
        }

        try {
                IPCEvent *event;

                /* Issue a registration request. */
                seqnum = ipcManager->requestApplicationRegistration(ari);
                event = wait_for_event(REGISTER_APPLICATION_RESPONSE_EVENT,
                                       seqnum);
                assert(event == NULL);
                if (errno != 0) {
                        return -1;
                }
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
rina_unregister(int fd, const char *dif_name, const char *local_appl)
{
        ApplicationProcessNamingInformation appi;
        ApplicationProcessNamingInformation difi;
        unsigned int seqnum;

        (void)fd; /* The netlink socket file descriptor is used internally */
        if (librina_init()) {
                return -1;
        }

        difi = ApplicationProcessNamingInformation(string(dif_name), string());
        appi = ApplicationProcessNamingInformation(string(local_appl),
                                                   string());

        try {
                IPCEvent *event;

                seqnum = ipcManager->requestApplicationUnregistration(appi,
                                difi);

                event = wait_for_event(UNREGISTER_APPLICATION_RESPONSE_EVENT,
                                       seqnum);
                assert(event == NULL);
                if (errno != 0) {
                        return -1;
                }
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
rina_flow_accept(int fd, const char **remote_appl)
{
        FlowInformation flow;
        IPCEvent *event = NULL;

        if (remote_appl) {
                /* This should be filled with the name
                 * of the requesting application. */
                *remote_appl = NULL;
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

                flow = ipcManager->allocateFlowResponse(*fre,
                                                /* result */ 0,
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
        delete event;

        return flow.fd;
}

int
rina_flow_alloc(const char *dif_name, const char *local_appl,
                const char *remote_appl, const struct rina_flow_spec *flowspec)
{
        ApplicationProcessNamingInformation local_apni, remote_apni, dif_apni;
        FlowSpecification flowspec_i;
        struct rina_flow_spec spec;
        FlowInformation flow;
        unsigned int seqnum;
        IPCEvent *event = NULL;

        if (librina_init()) {
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

        /* Convert flowspec --> flowspec_i */
        flowspec_i.maxAllowableGap = flowspec->max_sdu_gap;
        flowspec_i.averageBandwidth = flowspec->avg_bandwidth;
        flowspec_i.delay = flowspec->max_delay;
        flowspec_i.jitter = flowspec->max_jitter;
        flowspec_i.orderedDelivery = flowspec->in_order_delivery;

        local_apni.processName = string(local_appl);
        remote_apni.processName = string(remote_appl);

        try {
                AllocateFlowRequestResultEvent *resp;

                if (dif_name == NULL) {
                        seqnum = ipcManager->requestFlowAllocation(local_apni,
                                                                   remote_apni,
                                                                   flowspec_i);
                } else {
                        dif_apni.processName = string(dif_name);
                        seqnum = ipcManager->requestFlowAllocationInDIF(
                                                                local_apni,
                                                                remote_apni,
                                                                dif_apni,
                                                                flowspec_i);
                }

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
                flow = ipcManager->commitPendingFlow(resp->sequenceNumber,
                                                     resp->portId,
                                                     resp->difName);
                if (flow.fd < 0) {
                        errno = EPERM;
                }
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
        delete event;

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
}

}

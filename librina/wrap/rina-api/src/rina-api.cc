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

static int initialized = 0;

static int
librina_init(void)
{
        if (initialized) {
                return 0;
        }
        initialized = 1;
        try {
                rina::initialize("INFO", "/dev/null");
                return 0;
        } catch (...) {
                /* The operation can fail because librina is already
                 * initialized. */
                errno = EBUSY;
                return -1;
        }
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
        for (;;) {
                IPCEvent* event = ipcEventProducer->eventWait();
                DeallocateFlowResponseEvent *resp = NULL;

                if (!event) {
                        errno = ENXIO;
                        return NULL;
                }

                switch (event->eventType) {

                /* Process here the events for which we only need some
                 * bookkeeping. */
                case REGISTER_APPLICATION_RESPONSE_EVENT:
                        ipcManager->commitPendingRegistration(event->sequenceNumber,
                                dynamic_cast<RegisterApplicationResponseEvent*>(event)->DIFName);
                        // TODO delete event;
                        event = NULL;
                        break;

                case UNREGISTER_APPLICATION_RESPONSE_EVENT:
                        ipcManager->appUnregistrationResult(event->sequenceNumber,
                                dynamic_cast<UnregisterApplicationResponseEvent*>(event)->result == 0);
                        // TODO delete event;
                        event = NULL;
                        break;

                case FLOW_DEALLOCATED_EVENT:
                        ipcManager->flowDeallocated(dynamic_cast<FlowDeallocatedEvent*>(event)->portId);
                        // TODO delete event;
                        event = NULL;
                        break;

                case DEALLOCATE_FLOW_RESPONSE_EVENT:
                        resp = dynamic_cast<DeallocateFlowResponseEvent*>(event);
                        ipcManager->flowDeallocationResult(resp->portId, resp->result == 0);
                        // TODO delete event;
                        event = NULL;
                        break;

                /* Other events are returned to the caller, if they match. */
                case FLOW_ALLOCATION_REQUESTED_EVENT:
                        break;
                default:
                        break;
                }

                /* Exit the loop if the event matches what we asked for, or
                 * if we were not asked for anything in particular. */
                if ((wtype == NO_EVENT || event->eventType == wtype) &&
                        (wseqnum == ~0U || event->sequenceNumber == wseqnum)) {
                        return event;
                }
        }

        return NULL;
}

int
rina_register(int fd, const char *dif_name, const char *local_appl)
{
        ApplicationRegistrationInformation ari;
        RegisterApplicationResponseEvent *resp;
        unsigned int seqnum;
        IPCEvent *event;

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
                /* Issue a registration request. */
                seqnum = ipcManager->requestApplicationRegistration(ari);
                //TODO use wait_for_event
                /* Wait for the response to come, forever. In the future we
                 * could use select to add a timeout mechanism, and return
                 * ETIMEDOUT in errno. */
                for (;;) {
                        event = ipcEventProducer->eventWait();
                        if (event && event->eventType ==
                                REGISTER_APPLICATION_RESPONSE_EVENT &&
                                        event->sequenceNumber == seqnum) {
                                break;
                        }
                }

                resp = dynamic_cast<RegisterApplicationResponseEvent*>(event);

                /* Update librina state */
                if (resp->result) {
                        errno = EPERM;
                        ipcManager->withdrawPendingRegistration(seqnum);
                        return -1;
                }

                ipcManager->commitPendingRegistration(seqnum, resp->DIFName);
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
        UnregisterApplicationResponseEvent *resp;
        ApplicationProcessNamingInformation appi;
        ApplicationProcessNamingInformation difi;
        unsigned int seqnum;
        IPCEvent *event;

        (void)fd; /* The netlink socket file descriptor is used internally */
        if (librina_init()) {
                return -1;
        }

        difi = ApplicationProcessNamingInformation(string(dif_name), string());
        appi = ApplicationProcessNamingInformation(string(local_appl),
                                                   string());

        try {
                seqnum = ipcManager->requestApplicationUnregistration(appi,
                                difi);

                //TODO use wait_for_event
                event = ipcEventProducer->eventWait();
                while (event == NULL ||
                        event->eventType != UNREGISTER_APPLICATION_RESPONSE_EVENT
                                || event->sequenceNumber != seqnum) {
                        event = ipcEventProducer->eventWait();
                }

                resp = dynamic_cast<UnregisterApplicationResponseEvent*>(event);

                ipcManager->appUnregistrationResult(seqnum, resp->result == 0);
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

        (void)fd; /* The netlink socket file descriptor is used internally */
        if (librina_init()) {
                return -1;
        }

        try {
                FlowRequestEvent *fre;
                IPCEvent *event;

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
        } catch (...) {
                errno = ENOMEM;
                flow.fd = -1;
        }
        //TODO delete event;

        return flow.fd;
}

int
rina_flow_alloc(const char *dif_name, const char *local_appl,
              const char *remote_appl, const struct rina_flow_spec *flowspec)
{
        return -1;
}

void
rina_flow_spec_default(struct rina_flow_spec *spec)
{
}

}

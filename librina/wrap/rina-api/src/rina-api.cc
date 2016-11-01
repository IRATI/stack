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
#include <stdlib.h>
#include <unistd.h>
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
        } catch (Exception) {
                return -1;
        }
}

int
rina_open(void)
{
        librina_init();
        return dup(ipcManager->getControlFd());
}

int
rina_register(int fd, const char *dif_name, const char *local_appl)
{
        ApplicationRegistrationInformation ari;
        RegisterApplicationResponseEvent *resp;
        unsigned int seqnum;
        IPCEvent *event;

        (void)fd; /* The netlink socket file descriptor is used internally */

        ari.ipcProcessId = 0;  // This is an application, not an IPC process
        ari.appName = ApplicationProcessNamingInformation(string(local_appl),
                                                          string());

        if (dif_name) {
                ari.applicationRegistrationType = APPLICATION_REGISTRATION_SINGLE_DIF;
                ari.difName = ApplicationProcessNamingInformation(
                                                string(dif_name), string());
        } else {
                ari.applicationRegistrationType = APPLICATION_REGISTRATION_ANY_DIF;
        }

        // Request the registration
        seqnum = ipcManager->requestApplicationRegistration(ari);

        // Wait for the response to come
        for (;;) {
                event = ipcEventProducer->eventWait();
                if (event && event->eventType ==
                                REGISTER_APPLICATION_RESPONSE_EVENT &&
                                event->sequenceNumber == seqnum) {
                        break;
                }
        }

        resp = dynamic_cast<RegisterApplicationResponseEvent*>(event);

        // Update librina state
        if (resp->result) {
                errno = EPERM;
                ipcManager->withdrawPendingRegistration(seqnum);
                return -1;
        }

        ipcManager->commitPendingRegistration(seqnum, resp->DIFName);

        return 0;
}

int
rina_unregister(int fd, const char *dif_name, const char *local_appl)
{
        return -1;
}

int
rina_flow_accept(int fd, const char **remote_appl)
{
        return -1;
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

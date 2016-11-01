/*
 *  C wrapper for librina
 *
 *    Sander Vrijders   <sander.vrijders@intec.ugent.be>
 *    Dimitri Staessens <dimitri.staessens@intec.ugent.be>
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
#include <librina/librina.h>
#include <librina-c/librina-c.h>

using namespace rina;
using namespace std;

/* TODO With the POSIX api, this one should rethought. File descriptors
 * must be allocated by librina, not by this module. */

extern "C"
{

// TODO: Add global lock
struct flow {
        int              port_id;
        FlowRequestEvent fre;
        unsigned int     seq_num;
};

static int               fd_counter = 1;
static map <int, flow *> flows;
static string            ap_name = "";


int ap_init(char * name)
{
        // Sssssssh. Be quiet library.
        rina::initialize("INFO", "/dev/null");

        ap_name  = string(name);

        return 0;
}


void ap_fini(void)
{}

int ap_reg(char ** difs, size_t difs_size)
{
        ApplicationRegistrationInformation ari;
        RegisterApplicationResponseEvent * resp = NULL;
        unsigned int			   seq_num = 0;
        IPCEvent *			   event = NULL;
        string                             api_id = "0";
        stringstream                       ss;

        if (ap_name == "" || difs == NULL ||
            difs_size == 0 || difs[0] == NULL) {
                return -1;
        }

        // You can only register with 1 DIF (can be any DIF)
        if (difs_size > 1)
                return -1;

        ari.ipcProcessId = 0;
        ari.appName = ApplicationProcessNamingInformation(ap_name, api_id);
        if (string(difs[0]) == "*") {
                ari.applicationRegistrationType =
                        APPLICATION_REGISTRATION_ANY_DIF;
        } else {
                ari.applicationRegistrationType =
                        APPLICATION_REGISTRATION_SINGLE_DIF;
                ari.difName =
                        ApplicationProcessNamingInformation(string(difs[0]),
                                                            string());
        }

        try {
                seq_num = ipcManager->requestApplicationRegistration(ari);
        } catch (...) {
                return -1;
        }

        event = ipcEventProducer->eventWait();
        while (event == NULL ||
               event->eventType != REGISTER_APPLICATION_RESPONSE_EVENT ||
               event->sequenceNumber != seq_num) {
                event = ipcEventProducer->eventWait();
        }

        resp = dynamic_cast <RegisterApplicationResponseEvent*> (event);

        if (resp->result == 0) {
                try {
                        ipcManager->commitPendingRegistration(seq_num,
                                                              resp->DIFName);
                } catch (...) {
                        return -1;
                }
        } else {
                try {
                        ipcManager->withdrawPendingRegistration(seq_num);
                } catch (...) {
                        return -1;
                }
                return -1;
        }

        return 0;
}

int ap_unreg(char ** difs, size_t difs_size)
{
        ApplicationProcessNamingInformation  ap;
        ApplicationProcessNamingInformation  dif_name;
        UnregisterApplicationResponseEvent * resp = NULL;
        unsigned int			     seq_num = 0;
        IPCEvent *			     event = NULL;
        string                               api_id = "0";
        map <int, string>::iterator          it;
        stringstream                         ss;

        if (ap_name == "" || difs == NULL ||
            difs_size == 0 || difs[0] == NULL) {
                return -1;
        }

        // You can only register with 1 DIF (can be any DIF)
        if (difs_size > 1)
                return -1;

        ap = ApplicationProcessNamingInformation(ap_name, api_id);
        if (string(difs[0]) == "*") {
                return 0;
        } else {
                dif_name = ApplicationProcessNamingInformation(string(difs[0]),
                                                               string());
        }

        try {
                seq_num =
                        ipcManager->requestApplicationUnregistration(ap,
                                                                     dif_name);
        } catch (...) {
                return -1;
        }

        event = ipcEventProducer->eventWait();
        while (event == NULL ||
               event->eventType != UNREGISTER_APPLICATION_RESPONSE_EVENT ||
               event->sequenceNumber != seq_num) {
                event = ipcEventProducer->eventWait();
        }

        resp = dynamic_cast<UnregisterApplicationResponseEvent*>(event);

        try {
                ipcManager->appUnregistrationResult(seq_num,
                                                    resp->result == 0);
        } catch (...) {
                return -1;
        }

        return 0;
}

int flow_accept(int fd, char ** name, char ** ae_name)
{
        IPCEvent *                 event = NULL;
        FlowRequestEvent *         fre = NULL;
        map<int, string>::iterator it;
        bool                       for_us = false;
        int                        cli_fd = 0;
        struct flow *              mein_flow;

        // Horrible
        while (for_us != true) {
                event = ipcEventProducer->eventWait();
                while (event == NULL ||
                       event->eventType != FLOW_ALLOCATION_REQUESTED_EVENT) {
                        event = ipcEventProducer->eventWait();
                }

                fre = dynamic_cast <FlowRequestEvent *> (event);
                if (fre->localApplicationName.processName != ap_name)
                        continue;

                for_us = true;
        }

        if (name != NULL)
                *name = strdup(fre->remoteApplicationName.processName.c_str());
        if (ae_name != NULL)
                *ae_name =
                        strdup(fre->remoteApplicationName.entityName.c_str());

        cli_fd = fd_counter++;

        mein_flow = new struct flow;
        mein_flow->port_id = fre->portId;
        mein_flow->fre = *fre;

        flows.insert(pair <int, struct flow *> (cli_fd, mein_flow));

        return cli_fd;
}

int flow_alloc_resp(int fd, int result)
{
        struct flow *                      mein_flow;
        map <int, struct flow *>::iterator it;

        it = flows.find(fd);
        if (it == flows.end())
                return -1;

        mein_flow = it->second;

        try {
                ipcManager->allocateFlowResponse(mein_flow->fre,
                                                 result, true);
        } catch (...) {
                return -1;
        }

        return 0;
}

int flow_alloc(char * dst_name, char * src_ae_name, struct qos_spec * qos)
{
        FlowSpecification                   qos_spec;
        ApplicationProcessNamingInformation src;
        ApplicationProcessNamingInformation dst;
        int                                 fd = 0;
        struct flow *                       mein_flow;
        ApplicationProcessNamingInformation dif_name;
        string                              api_id;
        stringstream                        ss;

        if (dst_name == NULL) {
                return -1;
        }

        ss << getpid();
        api_id = ss.str();

        mein_flow = new struct flow;

        if (qos != NULL) {
                qos_spec.jitter = qos->jitter;
                qos_spec.delay = qos->delay;
        }

        src = ApplicationProcessNamingInformation();
        src.processName = ap_name;
        if (src_ae_name != NULL)
                src.entityName = string(src_ae_name);
        src.processInstance = api_id;

        dst = ApplicationProcessNamingInformation();
        dst.processName = string(dst_name);
        dst.processInstance = "0";

        try {
                if (qos == NULL ||
                    qos->dif_name == NULL ||
                    string(qos->dif_name) == "*") {
                        mein_flow->seq_num =
                                ipcManager->requestFlowAllocation
                                (src, dst, qos_spec);
                } else {
                        dif_name = ApplicationProcessNamingInformation();
                        dif_name.processName = qos->dif_name;
                        mein_flow->seq_num =
                                ipcManager->requestFlowAllocationInDIF
                                (src, dst, dif_name, qos_spec);
                }
        }  catch (...) {
                delete mein_flow;
                return -1;
        }

        fd = fd_counter++;
        flows.insert(pair <int, struct flow *> (fd, mein_flow));

        return fd;
}

int flow_alloc_res(int fd)
{
        AllocateFlowRequestResultEvent *   flow_req = NULL;
        FlowInformation                    flow;
        unsigned int                       seq_num = 0;
        IPCEvent *                         event = NULL;
        struct flow *                      mein_flow;
        map <int, struct flow *>::iterator it;

        it = flows.find(fd);
        if (it == flows.end())
                return -1;

        mein_flow = it->second;
        seq_num = mein_flow->seq_num;

        event = ipcEventProducer->eventWait();
        while (event == NULL ||
            event->eventType != ALLOCATE_FLOW_REQUEST_RESULT_EVENT ||
            event->sequenceNumber != seq_num) {
                event = ipcEventProducer->eventWait();
        }

        flow_req = dynamic_cast<AllocateFlowRequestResultEvent*>(event);

        try {
                flow = ipcManager->commitPendingFlow(flow_req->sequenceNumber,
                                                     flow_req->portId,
                                                     flow_req->difName);
        } catch (...) {
                return -1;
        }

        mein_flow->port_id = flow.portId;

        return 0;
}

int flow_dealloc(int fd)
{
        unsigned int                       port_id = 0;
        struct flow *                      mein_flow;
        map <int, struct flow *>::iterator it;

        it = flows.find(fd);
        if (it == flows.end())
                return -1;

        flows.erase(fd);

        mein_flow = it->second;
        port_id = mein_flow->port_id;
        delete mein_flow;

        try {
                ipcManager->requestFlowDeallocation(port_id);
        } catch (...) {
                return -1;
        }

	return 0;
}

int flow_cntl(int fd, int cmd, int oflags)
{
        unsigned int                       port_id = 0;
        struct flow *                      mein_flow;
        map <int, struct flow *>::iterator it;

        it = flows.find(fd);
        if (it == flows.end())
                return -1;

        mein_flow = it->second;
        port_id = mein_flow->port_id;

        if (oflags & 00004000)
                return ipcManager->setFlowOptsBlocking(port_id, false);
        else
                return ipcManager->setFlowOptsBlocking(port_id, true);
}

ssize_t flow_write(int fd, void * buf, size_t count)
{
        struct flow *                      mein_flow;
        map <int, struct flow *>::iterator it;
        int                                ret = 0;

        if (buf == NULL) {
                return -1;
        }

        it = flows.find(fd);
        if (it == flows.end())
                return -1;

        mein_flow = it->second;

        try {
                ret = ipcManager->writeSDU(mein_flow->port_id, buf, count);
        } catch (...) {
                return -1;
        }

        return ret;
}

ssize_t flow_read(int fd, void * buf, size_t count)

{
        struct flow *                      mein_flow;
        map <int, struct flow *>::iterator it;
        int                                ret = 0;

        if (buf == NULL) {
                return -1;
        }

        it = flows.find(fd);
        if (it == flows.end())
                return -1;

        mein_flow = it->second;

        try {
                ret = ipcManager->readSDU(mein_flow->port_id, buf, count);
        } catch (...) {
                return -1;
        }

        return ret;
}

}

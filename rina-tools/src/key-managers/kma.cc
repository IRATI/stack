//
// Key Management Agent
//
// Eduard Grasa <eduard.grasa@i2cat.net>
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//   1. Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//   2. Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.
//

#include <cstring>
#include <iostream>
#include <sstream>
#include <time.h>

#define RINA_PREFIX "key-management-agent"
#include <librina/ipc-api.h>
#include <librina/logs.h>

#include "kma.h"

using namespace std;
using namespace rina;

KeyManagementAgent::KeyManagementAgent(const std::string& creds_folder,
		   const std::list<std::string>& dif_names,
		   const std::string& apn,
		   const std::string& api,
		   const std::string& ckm_apn,
		   const std::string& ckm_api,
		   bool  q) :
        		Application(dif_names, apn, api)
{
	creds_location = creds_folder;
	dif_name = dif_names.front();
	ckm_name = ckm_apn;
	ckm_instance = ckm_api;
	quiet = q;
}

void KeyManagementAgent::run()
{
	int port_id = 0;

	cout << setprecision(5);

        port_id = createFlow();
        if (port_id < 0) {
        	LOG_ERR("Problems creating flow, exiting");
        	return;
        }

        destroyFlow(port_id);
}

int KeyManagementAgent::createFlow()
{
        AllocateFlowRequestResultEvent* afrrevent;
        FlowSpecification qosspec;
        IPCEvent* event;
        uint seqnum;

        if (dif_name != string()) {
                seqnum = ipcManager->requestFlowAllocationInDIF(
                        ApplicationProcessNamingInformation(app_name, app_instance),
                        ApplicationProcessNamingInformation(ckm_name, ckm_instance),
                        ApplicationProcessNamingInformation(dif_name, string()),
                        qosspec);
        } else {
                seqnum = ipcManager->requestFlowAllocation(
                        ApplicationProcessNamingInformation(app_name, app_instance),
                        ApplicationProcessNamingInformation(ckm_name, ckm_instance),
                        qosspec);
        }

        for (;;) {
                event = ipcEventProducer->eventWait();
                if (event && event->eventType == ALLOCATE_FLOW_REQUEST_RESULT_EVENT
                                && event->sequenceNumber == seqnum) {
                        break;
                }
                LOG_DBG("Client got new event %d", event->eventType);
        }

        afrrevent = dynamic_cast<AllocateFlowRequestResultEvent*>(event);

        rina::FlowInformation flow = ipcManager->commitPendingFlow(afrrevent->sequenceNumber,
                                              	      	           afrrevent->portId,
                                              	      	           afrrevent->difName);
        if (flow.portId < 0) {
                LOG_ERR("Failed to allocate a flow");
                return flow.portId;
        } else {
                LOG_DBG("[DEBUG] Port id = %d", flow.portId);
        }

        return flow.portId;
}

void KeyManagementAgent::destroyFlow(int port_id)
{
        DeallocateFlowResponseEvent *resp = NULL;
        unsigned int seqnum;
        IPCEvent* event;

        seqnum = ipcManager->requestFlowDeallocation(port_id);

        for (;;) {
                event = ipcEventProducer->eventWait();
                if (event && event->eventType == DEALLOCATE_FLOW_RESPONSE_EVENT
                                && event->sequenceNumber == seqnum) {
                        break;
                }
                LOG_DBG("Client got new event %d", event->eventType);
        }
        resp = dynamic_cast<DeallocateFlowResponseEvent*>(event);

        ipcManager->flowDeallocationResult(port_id, resp->result == 0);
}

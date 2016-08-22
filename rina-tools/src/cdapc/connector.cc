//
// CDAP Connector
//
// Micheal Crotty <mcrotty@tssg.org>
// Copyright (C) 2016, PRISTINE, Waterford Insitute of Technology
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
#include <iostream>

#include <cassert>
#define RINA_PREFIX     "cdap-connect"
#include <librina/logs.h>
//#include <librina/cdap_v2.h>
#include <librina/common.h>
#include <rinad/common/configuration.h>
//#include <rinad/common/encoder.h>

#include "connector.h"
#include "workers.h"


 
Connector::Connector(const std::list<std::string>& dif_names,
		 const std::string& apn,
     const std::string& api,
     const std::string& ws)
        : Server(dif_names, apn, api)
{
    client_app_reg_ = false;
    ws_address_ = ws;
}

void Connector::run()
{
  // Attempt to start the DMS worker
  ws_run();
  
  // Attempt to start the RINA server
  for (;;) {
    sleep(1);
    //LOG_INFO("Doing RINA stuff");
  }
}

void Connector::ws_run() {
  // have we something for the DMS worker to do
  if (!(ws_address_.empty())) {
    ServerWorker * worker = internal_start_worker(ws_address_);
    if (worker != NULL) {
      // register it with the base class
      worker_started(worker);
      LOG_INFO("New DMS worker allocated");
    }
  }
}

void Connector::rina_run() {
  
    rina::initialize("INFO", "");
    
    int order = 1;
    std::map<int, rina::FlowInformation> waiting;
    rina::FlowInformation flow;
    applicationRegister();

    for (;;)
    {
        rina::IPCEvent* event = rina::ipcEventProducer->eventWait();
        int port_id = 0;
        rina::DeallocateFlowResponseEvent *resp = NULL;

        if (!event)
            return;

        switch (event->eventType) {

            case rina::REGISTER_APPLICATION_RESPONSE_EVENT:
                rina::ipcManager->commitPendingRegistration(
                        event->sequenceNumber,
                        dynamic_cast<rina::RegisterApplicationResponseEvent*>(event)
                                ->DIFName);
                break;

            case rina::UNREGISTER_APPLICATION_RESPONSE_EVENT:
                rina::ipcManager->appUnregistrationResult(
                        event->sequenceNumber,
                        dynamic_cast<rina::UnregisterApplicationResponseEvent*>(event)
                                ->result == 0);
                break;

            case rina::FLOW_ALLOCATION_REQUESTED_EVENT:
                flow = rina::ipcManager->allocateFlowResponse(
                        *dynamic_cast<rina::FlowRequestEvent*>(event), 0, true);
                port_id = flow.portId;
                LOG_INFO("New flow allocated [port-id = %d]", port_id);

                if (flow.remoteAppName.processName == "rina.apps.mad.1")
                {
                    if (waiting.find(1) == waiting.end())
                    {
                        waiting[1] = flow;
                    } else
                    {
                        std::cout
                                << "Error, flow with the same mad already exist: "
                                << flow.remoteAppName.processName << std::endl;
                        rina::ipcManager->requestFlowDeallocation(flow.portId);
                    }
                }
                if (flow.remoteAppName.processName == "rina.apps.mad.2")
                {
                    if (waiting.find(2) == waiting.end())
                    {
                        waiting[2] = flow;
                    } else
                    {
                        std::cout
                                << "Error, flow with the same mad already exist: "
                                << flow.remoteAppName.processName << std::endl;
                        rina::ipcManager->requestFlowDeallocation(flow.portId);
                    }
                }
                if (flow.remoteAppName.processName == "rina.apps.mad.3")
                {
                    if (waiting.find(3) == waiting.end())
                    {
                        waiting[3] = flow;
                    } else
                    {
                        std::cout
                                << "Error, flow with the same mad already exist: "
                                << flow.remoteAppName.processName << std::endl;
                        rina::ipcManager->requestFlowDeallocation(flow.portId);
                    }
                }
                while (waiting.find(order) != waiting.end())
                {
                    startWorker(waiting[order]);
                    order++;
                }
                break;

            case rina::FLOW_DEALLOCATED_EVENT:
                port_id = dynamic_cast<rina::FlowDeallocatedEvent*>(event)
                        ->portId;
                rina::ipcManager->flowDeallocated(port_id);
                LOG_INFO("Flow torn down remotely [port-id = %d]", port_id);
                break;

            case rina::DEALLOCATE_FLOW_RESPONSE_EVENT:
                LOG_INFO("Destroying the flow after time-out");
                resp = dynamic_cast<rina::DeallocateFlowResponseEvent*>(event);
                port_id = resp->portId;

                rina::ipcManager->flowDeallocationResult(port_id,
                                                         resp->result == 0);
                break;

            default:
                LOG_INFO("Server got new event of type %d", event->eventType);
                break;
        }
        sleep(1);
    }
}

// Create the appropriate MA worker
ServerWorker * Connector::internal_start_worker(rina::FlowInformation flow)
{
    rina::ThreadAttributes threadAttributes;
    MAWorker * worker = new MAWorker(&threadAttributes, flow,
                                               max_sdu_size_in_bytes, this);
    worker->start();
    worker->detach();
    ma_worker_ = worker;
    return worker;
}

// Create the appropriate DMS worker
ServerWorker * Connector::internal_start_worker(const std::string& ws_address)
{
    rina::ThreadAttributes threadAttributes;
    DMSWorker * worker = new DMSWorker(&threadAttributes, ws_address,
                                               max_sdu_size_in_bytes, this);
    worker->start();
    worker->detach();
    dms_worker_ = worker;
    return worker;
}


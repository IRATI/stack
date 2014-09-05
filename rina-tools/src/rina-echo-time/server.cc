//
// Echo Server
// 
// Addy Bombeke <addy.bombeke@ugent.be>
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#include <iostream>
#include <thread>

#define RINA_PREFIX     "rina-echo-app"
#include <librina/logs.h>

#include "server.h"

using namespace std;
using namespace rina;

Server::Server(const string & app_name_,
               const string & app_instance_,
               bool           debug_mes_) :
        Application(app_name_, app_instance_),
        debug_mes(debug_mes_)
{ }

void Server::run()
{
        init();

        for(;;) {
                IPCEvent* event = ipcEventProducer->eventWait();
                if (!event)
                        return;
                switch(event->eventType) {
                case REGISTER_APPLICATION_RESPONSE_EVENT:
                        ipcManager->commitPendingRegistration(event->sequenceNumber,
                                                             reinterpret_cast<RegisterApplicationResponseEvent*>(event)->DIFName);
                        break;
                case UNREGISTER_APPLICATION_RESPONSE_EVENT:
                        ipcManager->appUnregistrationResult(event->sequenceNumber,
                                                            reinterpret_cast<UnregisterApplicationResponseEvent*>(event)->result == 0);
                        break;
                case FLOW_ALLOCATION_REQUESTED_EVENT: {
                        Flow* flow = ipcManager->
                                allocateFlowResponse(*reinterpret_cast<FlowRequestEvent*>(event), 0, true);
                        thread t(&Server::runFlow, this, flow);
                        LOG_DBG("port-id = %d", flow->getPortId());
                        t.detach();
                        break;
                }
                case FLOW_DEALLOCATED_EVENT:
                        ipcManager->requestFlowDeallocation(reinterpret_cast<FlowDeallocatedEvent*>
                                                            (event)->portId);
                        break;
                default:
                        LOG_INFO("Server got new event of type %d",
                                        event->eventType);
                        break;
                }
        }
}

void Server::init()
{ applicationRegister(); }

void Server::runFlow(Flow* flow)
{
        char buffer[max_buffer_size];
        try {
                for(;;) {
                        int bytesreaded = flow->readSDU(buffer,
                                                        max_buffer_size);
                        flow->writeSDU(buffer, bytesreaded);
                }
        } catch(...) {
                LOG_ERR("SDU write/read failed");
        }
}

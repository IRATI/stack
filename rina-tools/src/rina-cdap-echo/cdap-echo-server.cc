//
// Echo CDAP Server
// 
// Addy Bombeke <addy.bombeke@ugent.be>
// Bernat Gastón <bernat.gaston@i2cat.net>
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

#include <cstring>
#include <iostream>
#include <thread>
#include <time.h>
#include <signal.h>
#include <time.h>

#define RINA_PREFIX     "rina-cdap-echo-server"
#include <librina/logs.h>

#include "cdap-echo-server.h"

using namespace std;
using namespace rina;

ConnectionCallback::ConnectionCallback(bool *keep_serving,
                                       rina::cdap::CDAPProviderInterface **prov)
{
  keep_serving_ = keep_serving;
  prov_ = prov;
}
void ConnectionCallback::open_connection(
    const rina::cdap_rib::con_handle_t &con,
    const rina::cdap_rib::flags_t &flags, int message_id)
{
  (void) flags;
  cdap_rib::res_info_t res;
  res.result_ = 1;
  res.result_reason_ = "Ok";
  std::cout<<"open conection request CDAP message received"<<std::endl;
  (*prov_)->open_connection_response(con, res, message_id);
  std::cout<<"open conection response CDAP message sent"<<std::endl;
}
void ConnectionCallback::remote_read_request(
    const rina::cdap_rib::con_handle_t &con,
    const rina::cdap_rib::obj_info_t &obj,
    const rina::cdap_rib::filt_info_t &filt, int message_id)
{
  (void) filt;
  cdap_rib::flags_t flags;
  flags.flags_ = cdap_rib::flags_t::NONE_FLAGS;
  cdap_rib::res_info_t res;
  res.result_ = 1;
  res.result_reason_ = "Ok";
  std::cout<<"read request CDAP message received"<<std::endl;
  (*prov_)->remote_read_response(con.port_, obj, flags, res, message_id);
  std::cout<<"read response CDAP message sent"<<std::endl;
}

void ConnectionCallback::close_connection(const rina::cdap_rib::con_handle_t &con,
                      const rina::cdap_rib::flags_t &flags, int message_id)
{
  cdap_rib::res_info_t res;
  res.result_ = 1;
  res.result_reason_ = "Ok";
  std::cout<<"conection close request CDAP message received"<<std::endl;
  (*prov_)->close_connection_response(con.port_, flags, res, message_id);
  std::cout<<"conection close response CDAP message sent"<<std::endl;
  *keep_serving_ = false;
}

Server::Server(const string& dif_name, const string& app_name,
               const string& app_instance, const int dealloc_wait)
    : Application(dif_name, app_name, app_instance),
      dw(dealloc_wait)
{
}
void Server::run()
{
  applicationRegister();

  for (;;) {
    IPCEvent* event = ipcEventProducer->eventWait();
    Flow *flow = 0;
    unsigned int port_id;
    DeallocateFlowResponseEvent *resp = 0;

    if (!event)
      return;

    switch (event->eventType) {

      case REGISTER_APPLICATION_RESPONSE_EVENT:
        ipcManager->commitPendingRegistration(
            event->sequenceNumber,
            dynamic_cast<RegisterApplicationResponseEvent*>(event)->DIFName);
        break;

      case UNREGISTER_APPLICATION_RESPONSE_EVENT:
        ipcManager->appUnregistrationResult(
            event->sequenceNumber,
            dynamic_cast<UnregisterApplicationResponseEvent*>(event)->result
                == 0);
        break;

      case FLOW_ALLOCATION_REQUESTED_EVENT:
        flow = ipcManager->allocateFlowResponse(
            *dynamic_cast<FlowRequestEvent*>(event), 0, true);
        LOG_INFO("New flow allocated [port-id = %d]", flow->getPortId());
        startWorker (flow);
        break;

      case FLOW_DEALLOCATED_EVENT:
        port_id = dynamic_cast<FlowDeallocatedEvent*>(event)->portId;
        ipcManager->flowDeallocated(port_id);
        LOG_INFO("Flow torn down remotely [port-id = %d]", port_id);
        break;

      case DEALLOCATE_FLOW_RESPONSE_EVENT:
        LOG_INFO("Destroying the flow after time-out");
        resp = dynamic_cast<DeallocateFlowResponseEvent*>(event);
        port_id = resp->portId;

        ipcManager->flowDeallocationResult(port_id, resp->result == 0);
        break;

      default:
        LOG_INFO("Server got new event of type %d", event->eventType);
        break;
    }
  }
}

void Server::startWorker(Flow *flow)
{
  void (Server::*server_function)(Flow *flow);

  server_function = &Server::serveEchoFlow;

  thread t(server_function, this, flow);
  t.detach();
}

void Server::serveEchoFlow(rina::Flow* flow)
{
  bool keep_serving = true;
  char buffer[max_sdu_size_in_bytes];
  rina::cdap::CDAPProviderInterface *cdap_prov = 0;
  ConnectionCallback* callback = new ConnectionCallback(&keep_serving, &cdap_prov);
  std::cout<<"cdap_prov created"<<std::endl;
  cdap::CDAPProviderFactory::init(2000);
  cdap_prov = cdap::CDAPProviderFactory::create(false, callback);
  while (keep_serving) {
    int bytes_read = flow->readSDU(buffer, max_sdu_size_in_bytes);
    cdap_rib::SerializedObject message;
    message.message_ = buffer;
    message.size_ = bytes_read;
    cdap_prov->process_message(message, flow->getPortId());
  }
  cdap::CDAPProviderFactory::destroy(flow->getPortId());
  delete cdap_prov;
  delete callback;
}
/*
void Server::destroyFlow(sigval_t val)
{
  Flow *flow = (Flow *) val.sival_ptr;

  if (flow->getState() != FlowState::FLOW_ALLOCATED)
    return;

  int port_id = flow->getPortId();

  // TODO here we should store the seqnum (handle) returned by
  // requestFlowDeallocation() and match it in the event loop
  ipcManager->requestFlowDeallocation(port_id);
}
*/

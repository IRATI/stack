//
// Manager
//
// Addy Bombeke <addy.bombeke@ugent.be>
// Vincenzo Maffione <v.maffione@nextworks.it>
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
#include <iostream>

#include <cassert>
#include <thread>
#define RINA_PREFIX     "manager"
#include <librina/logs.h>
#include <librina/cdap_v2.h>
#include <librina/common.h>
#include "encoders_mad.h"

#include "manager.h"

using namespace std;
using namespace rina;
using namespace rinad;

const std::string Manager::mad_name = "mad";
const std::string Manager::mad_instance = "1";

Application::Application(const string& dif_name_, const string& app_name_,
                         const string& app_instance_)
    : dif_name(dif_name_),
      app_name(app_name_),
      app_instance(app_instance_)
{
}

void Application::applicationRegister()
{
  ApplicationRegistrationInformation ari;
  RegisterApplicationResponseEvent *resp;
  unsigned int seqnum;
  IPCEvent *event;

  ari.ipcProcessId = 0;  // This is an application, not an IPC process
  ari.appName = ApplicationProcessNamingInformation(app_name, app_instance);
  if (dif_name == string()) {
    ari.applicationRegistrationType = rina::APPLICATION_REGISTRATION_ANY_DIF;
  } else {
    ari.applicationRegistrationType = rina::APPLICATION_REGISTRATION_SINGLE_DIF;
    ari.difName = ApplicationProcessNamingInformation(dif_name, string());
  }

  // Request the registration
  seqnum = ipcManager->requestApplicationRegistration(ari);

  // Wait for the response to come
  for (;;) {
    event = ipcEventProducer->eventWait();
    if (event && event->eventType == REGISTER_APPLICATION_RESPONSE_EVENT
        && event->sequenceNumber == seqnum) {
      break;
    }
  }

  resp = dynamic_cast<RegisterApplicationResponseEvent*>(event);

  // Update librina state
  if (resp->result == 0) {
    ipcManager->commitPendingRegistration(seqnum, resp->DIFName);
  } else {
    ipcManager->withdrawPendingRegistration(seqnum);
    throw ApplicationRegistrationException("Failed to register application");
  }
}

const uint Application::max_buffer_size = 1 << 18;

ConnectionCallback::ConnectionCallback(rina::cdap::CDAPProviderInterface **prov)
{
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

  mad_manager::structures::ipcp_config_t ipc_config;
  ipc_config.process_instance = 1;
  ipc_config.process_name = "test1.IRATI";
  ipc_config.process_type = "normal.IPC";
  ipc_config.dif_to_register = "400";
  ipc_config.dif_to_assign = "normal.DIF";

  cdap_rib::obj_info_t obj;
  obj.name_ =
      "root, computingSystemID = 1, processingSystemID=1, kernelApplicationProcess, osApplicationProcess, ipcProcessID=1";
  obj.class_ = "IPCProcess";
  obj.inst_ = 0;
  mad_manager::encoders::IPCPConfigEncoder().encode(ipc_config, obj.value_);

  cdap_rib::flags_t flags_res;
  flags_res.flags_ = cdap_rib::flags_t::NONE_FLAGS;

  cdap_rib::filt_info_t filt;
  filt.filter_ = 0;
  filt.scope_ = 0;

  (*prov_)->remote_create(con, obj, flags_res, filt);
  std::cout << "create IPC request CDAP message sent" << std::endl;
}

void ConnectionCallback::remote_create_result(const rina::cdap_rib::con_handle_t &con,
                                  const rina::cdap_rib::res_info_t &res)
{
  (void) con;
  std::cout<<"Result code is: "<<res.result_<<std::endl;
}

Manager::Manager(const std::string& dif_name, const std::string& apn,
                 const std::string& api, bool quiet)
    : Application(dif_name, apn, api),
      quiet_(quiet)
{
  (void)quiet_;
  (void)client_app_reg_;
}

Manager::~Manager()
{
}

void Manager::run()
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

void Manager::startWorker(rina::Flow *flow)
{
  void (Manager::*server_function)(Flow *flow);

  server_function = &Manager::createIPCP;

  thread t(server_function, this, flow);
  t.detach();
}

void Manager::createIPCP(rina::Flow* flow)
{
  char buffer[max_sdu_size_in_bytes];
  rina::cdap::CDAPProviderInterface *cdap_prov = 0;
  ConnectionCallback callback(&cdap_prov);
  std::cout<<"cdap_prov created"<<std::endl;
  cdap::CDAPProviderFactory::init(2000);
  cdap_prov = cdap::CDAPProviderFactory::create(false, &callback);

  // CACEP
  int bytes_read = flow->readSDU(buffer, max_sdu_size_in_bytes);
  cdap_rib::SerializedObject message;
  message.message_ = buffer;
  message.size_ = bytes_read;
  cdap_prov->process_message(message, flow->getPortId());

  // CREATE RESPONSE
  bytes_read = flow->readSDU(buffer, max_sdu_size_in_bytes);
  message.message_ = buffer;
  message.size_ = bytes_read;
  cdap_prov->process_message(message, flow->getPortId());


  cdap::CDAPProviderFactory::destroy(flow->getPortId());
  delete cdap_prov;
}
/*
void Manager::release()
{
  char buffer[max_sdu_size_in_bytes];
  std::cout << "release request CDAP message sent" << std::endl;
  cdap_prov_->close_connection(con_);
  int bytes_read = flow_->readSDU(buffer, max_sdu_size_in_bytes);
  cdap_rib::SerializedObject message;
  message.message_ = buffer;
  message.size_ = bytes_read;
  std::cout << "Waiting for release response" << std::endl;
  cdap_prov_->process_message(message, flow_->getPortId());
  std::cout << "Release completed" << std::endl;
}

void Manager::destroyFlow()
{
  DeallocateFlowResponseEvent *resp = 0;
  unsigned int seqnum;
  IPCEvent* event;
  int port_id = flow_->getPortId();
  cdap::CDAPProviderFactory::destroy(port_id);
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
  assert(resp);

  ipcManager->flowDeallocationResult(port_id, resp->result == 0);
  cdap::CDAPProviderFactory::finit();
}
*/

//
// Echo CDAP Client
//
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
#include <sstream>
#include <cassert>
#include <random>
#include <thread>
#include <unistd.h>
#include <chrono>

#define RINA_PREFIX     "cdap-echo-client"
#include <librina/logs.h>
#include <librina/cdap_rib_structures.h>
#include <librina/cdap_v2.h>

#include "cdap-echo-client.h"

using namespace std;
using namespace rina;

Client::Client(const string& dif_nm, const string& apn, const string& api,
               const string& server_apn, const string& server_api, bool q,
               unsigned long count, bool registration, unsigned int w, int g,
               int dw)
    : Application(dif_nm, apn, api),
      dif_name(dif_nm),
      server_name(server_apn),
      server_instance(server_api),
      quiet(q),
      echo_times(count),
      client_app_reg(registration),
      wait(w),
      gap(g),
      dealloc_wait(dw)
{
  count_ = 0;
}

Client::~Client()
{
  delete cdap_prov_;
}

void Client::run()
{
  keep_running_ = true;
  if (client_app_reg) {
    applicationRegister();
  }
  createFlow();
  if (flow_) {
    // CACEP
    cacep();

    sendReadRMessage();
  }
}

void Client::createFlow()
{
  flow_ = 0;
  AllocateFlowRequestResultEvent* afrrevent;
  FlowSpecification qosspec;
  IPCEvent* event;
  uint seqnum;

  if (gap >= 0)
    qosspec.maxAllowableGap = gap;

  if (dif_name != string()) {
    seqnum = ipcManager->requestFlowAllocationInDIF(
        ApplicationProcessNamingInformation(app_name, app_instance),
        ApplicationProcessNamingInformation(server_name, server_instance),
        ApplicationProcessNamingInformation(dif_name, string()), qosspec);
  } else {
    seqnum = ipcManager->requestFlowAllocation(
        ApplicationProcessNamingInformation(app_name, app_instance),
        ApplicationProcessNamingInformation(server_name, server_instance),
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

  flow_ = ipcManager->commitPendingFlow(afrrevent->sequenceNumber,
                                        afrrevent->portId, afrrevent->difName);
  if (!flow_ || flow_->getPortId() == -1) {
    LOG_ERR("Failed to allocate a flow");
    flow_ = 0;
  } else {
    LOG_DBG("[DEBUG] Port id = %d", flow_->getPortId());
  }
}

void Client::cacep()
{
  char buffer[max_sdu_size_in_bytes];
  cdap::CDAPProviderFactory::init(2000);
  cdap_prov_ = cdap::CDAPProviderFactory::create(false, this);
  cdap_rib::vers_info_t ver;
  ver.version_ = 1;
  cdap_rib::src_info_t src;
  src.ap_name_ = flow_->getLocalApplicationName().processName;
  src.ae_name_ = flow_->getLocalApplicationName().entityName;
  src.ap_inst_ = flow_->getLocalApplicationName().processInstance;
  src.ae_inst_ = flow_->getLocalApplicationName().entityInstance;
  cdap_rib::dest_info_t dest;
  dest.ap_name_ = flow_->getRemoteApplcationName().processName;
  ;
  dest.ae_name_ = flow_->getRemoteApplcationName().entityName;
  ;
  dest.ap_inst_ = flow_->getRemoteApplcationName().processInstance;
  ;
  dest.ae_inst_ = flow_->getRemoteApplcationName().entityInstance;
  ;
  cdap_rib::auth_info auth;
  auth.auth_mech_ = auth.AUTH_NONE;

  std::cout << "open conection request CDAP message sent" << std::endl;
  con_ = cdap_prov_->open_connection(ver, src, dest, auth, flow_->getPortId());
  int bytes_read = flow_->readSDU(buffer, max_sdu_size_in_bytes);
  cdap_rib::SerializedObject message;
  message.message_ = buffer;
  message.size_ = bytes_read;
  cdap_prov_->new_message(message, flow_->getPortId());
}

void Client::open_connection_result(const cdap_rib::con_handle_t &con,
                                    const cdap_rib::result_info &res)
{
  (void) con;
  (void) res;
  std::cout << "open conection response CDAP message received" << std::endl;
}
void Client::remote_read_result(const rina::cdap_rib::con_handle_t &con,
                                const rina::cdap_rib::res_info_t &res)
{
  (void) con;
  (void) res;
  std::cout << "read response CDAP message received" << std::endl;
}

void Client::close_connection_result(const cdap_rib::con_handle_t &con,
                                     const cdap_rib::result_info &res)
{
  (void) con;
  (void) res;
  std::cout << "release response CDAP message received" << std::endl;
  destroyFlow();
}

void Client::sendReadRMessage()
{
  while (count_ < echo_times)
  {
    IPCEvent* event = ipcEventProducer->eventPoll();
    char buffer[max_sdu_size_in_bytes];
    if (event) {
      switch (event->eventType) {
        case FLOW_DEALLOCATED_EVENT:
          std::cout<<"Client received a flow deallocation event"<<std::endl;
          destroyFlow();
          break;
        default:
          LOG_INFO("Client got new event %d", event->eventType);
          break;
      }
    } else {

      // READ
      cdap_rib::obj_info_t obj;
      obj.name_ = "test name";
      obj.class_ = "test class";
      cdap_rib::flags_t flags;
      flags.flags_ = cdap_rib::flags_t::NONE_FLAGS;
      cdap_rib::filt_info_t filt;
      filt.filter_ = 0;
      filt.scope_ = 0;
      cdap_prov_->remote_read(con_, obj, flags, filt);
      std::cout << "read request CDAP message sent" << std::endl;
      int bytes_read = flow_->readSDU(buffer, max_sdu_size_in_bytes);
      cdap_rib::SerializedObject message;
      message.message_ = buffer;
      message.size_ = bytes_read;
      cdap_prov_->new_message(message, flow_->getPortId());
      count_++;
      std::cout << "count: " << count_ << std::endl;
    }
  }
  release();
}

void Client::release()
{
  char buffer[max_sdu_size_in_bytes];
  std::cout << "release request CDAP message sent" << std::endl;
  cdap_prov_->close_connection(con_);
  int bytes_read = flow_->readSDU(buffer, max_sdu_size_in_bytes);
  cdap_rib::SerializedObject message;
  message.message_ = buffer;
  message.size_ = bytes_read;
  std::cout<<"Esperant el release response"<<std::endl;
  cdap_prov_->new_message(message, flow_->getPortId());
  std::cout<<"Finalitzant funció"<<std::endl;
}

void Client::destroyFlow()
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

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

static std::chrono::seconds thres_s = std::chrono::seconds(9);
static std::chrono::milliseconds thres_ms = std::chrono::milliseconds(9);
static std::chrono::microseconds thres_us = std::chrono::microseconds(9);

APPcallback::APPcallback(Client *client)
{
  client_ = client;
}
void APPcallback::open_connection_result(const cdap_rib::con_handle_t &con,
                                         const cdap_rib::result_info &res)
{
  (void) con;
  (void) res;
  std::cout << "CACEP success" << std::endl;
  client_->sendReadRMessage();
}
void APPcallback::remote_read_result(const rina::cdap_rib::con_handle_t &con,
                                const rina::cdap_rib::res_info_t &res)
{
  (void) con;
  (void) res;
  std::cout<<"Remote Read Result received"<<std::endl;
  client_->sendReadRMessage();
}

void APPcallback::close_connection_result(const cdap_rib::con_handle_t &con,
                                          const cdap_rib::result_info &res)
{
  (void) con;
  (void) res;
  std::cout<<"Connection closed"<<std::endl;
  client_->destroyFlow();
}



static string durationToString(
    const std::chrono::high_resolution_clock::duration& dur)
{
  std::stringstream ss;

  if (dur > thres_s)
    ss << std::chrono::duration_cast < std::chrono::seconds
        > (dur).count() << "s";
  else if (dur > thres_ms)
    ss << std::chrono::duration_cast < std::chrono::milliseconds
        > (dur).count() << "ms";
  else if (dur > thres_us)
    ss << std::chrono::duration_cast < std::chrono::microseconds
        > (dur).count() << "us";
  else
    ss << std::chrono::duration_cast < std::chrono::nanoseconds
        > (dur).count() << "ns";

  return ss.str();
}

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
}

void Client::run()
{
  if (client_app_reg) {
    applicationRegister();
  }
  createFlow();
  if (flow_) {
    // CACEP
    cacep();
  }
  /*
   if (flow) {
   if (dealloc_wait > 0) {
   sleep(dealloc_wait);
   }
   destroyFlow(flow);
   }
   */
}

void Client::createFlow()
{
  flow_ = 0;
  std::chrono::high_resolution_clock::time_point begintp =
      std::chrono::high_resolution_clock::now();
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

  std::chrono::high_resolution_clock::time_point eindtp =
      std::chrono::high_resolution_clock::now();
  std::chrono::high_resolution_clock::duration dur = eindtp - begintp;

  if (!quiet) {
    cout << "Flow allocation time = " << durationToString(dur) << endl;
  }
}

void Client::cacep()
{
  callback_ = new APPcallback(this);
  cdap_prov_ = cdap::CDAPProviderFactory->create(2000, false, callback_);
  cdap_rib::vers_info_t ver;
  ver.version_ = 1;
  cdap_rib::src_info_t src;
  src.ap_name_ = flow_->getLocalApplicationName().processName;
  src.ae_name_ = flow_->getLocalApplicationName().entityName;
  src.ap_inst_ = flow_->getLocalApplicationName().processInstance;
  src.ae_inst_ = flow_->getLocalApplicationName().entityInstance;
  cdap_rib::dest_info_t dest;
  dest.ap_name_ = flow_->getRemoteApplcationName().processName;;
  dest.ae_name_ = flow_->getRemoteApplcationName().entityName;;
  dest.ap_inst_ = flow_->getRemoteApplcationName().processInstance;;
  dest.ae_inst_ = flow_->getRemoteApplcationName().entityInstance;;
  cdap_rib::auth_info auth;
  auth.auth_mech_ = auth.AUTH_NONE;
  int port = flow_->getPortId();
  con_ = cdap_prov_->open_connection(ver, src, dest, auth, port);
}


void Client::sendReadRMessage()
{
  if (count_ >= echo_times)
  {
    IPCEvent* event = ipcEventProducer->eventPoll();

    if (event) {
      switch (event->eventType) {
        case FLOW_DEALLOCATED_EVENT:
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
      count_++;
    }
  }
  else
    release();
}

void Client::release()
{
  cdap_prov_->close_connection(con_);
}

void Client::destroyFlow()
{
DeallocateFlowResponseEvent *resp = 0;
unsigned int seqnum;
IPCEvent* event;
int port_id = flow_->getPortId();

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
}

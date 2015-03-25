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

class APPcallback: public cdap::CDAPCallbackInterface
{
 public:
  void open_connection_result(const cdap_rib::con_handle_t &con,
                                      const cdap_rib::result_info &res)
  {}
  void open_connection(const cdap_rib::con_handle_t &con,
                               const cdap_rib::result_info &res,
                               int message_id)
  {}
  void close_connection_result(const cdap_rib::con_handle_t &con,
                                       const cdap_rib::result_info &res)
  {}
  void close_connection(const cdap_rib::con_handle_t &con,
                                const cdap_rib::flags_t &flags,
                                const cdap_rib::result_info &res,
                                int message_id)
  {}

  void remote_create_result(const cdap_rib::con_handle_t &con,
                                    const cdap_rib::res_info_t &res)
  {}
  void remote_delete_result(const cdap_rib::con_handle_t &con,
                                    const cdap_rib::res_info_t &res)
  {}
  void remote_read_result(const cdap_rib::con_handle_t &con,
                                  const cdap_rib::res_info_t &res)
  {}
  void remote_cancel_read_result(const cdap_rib::con_handle_t &con,
                                         const cdap_rib::res_info_t &res)
  {}
  void remote_write_result(const cdap_rib::con_handle_t &con,
                                   const cdap_rib::res_info_t &res)
  {}
  void remote_start_result(const cdap_rib::con_handle_t &con,
                                   const cdap_rib::res_info_t &res)
  {}
  void remote_stop_result(const cdap_rib::con_handle_t &con,
                                  const cdap_rib::res_info_t &res)
  {}
  void remote_create_request(const cdap_rib::con_handle_t &con,
                                     const cdap_rib::obj_info_t &obj,
                                     const cdap_rib::filt_info_t &filt,
                                     int message_id){}
  void remote_delete_request(const cdap_rib::con_handle_t &con,
                                     const cdap_rib::obj_info_t &obj,
                                     const cdap_rib::filt_info_t &filt,
                                     int message_id)
  {}
  void remote_read_request(const cdap_rib::con_handle_t &con,
                                   const cdap_rib::obj_info_t &obj,
                                   const cdap_rib::filt_info_t &filt,
                                   int message_id)
  {}
  void remote_cancel_read_request(const cdap_rib::con_handle_t &con,
                                          const cdap_rib::obj_info_t &obj,
                                          const cdap_rib::filt_info_t &filt,
                                          int message_id){}
  void remote_write_request(const cdap_rib::con_handle_t &con,
                                    const cdap_rib::obj_info_t &obj,
                                    const cdap_rib::filt_info_t &filt,
                                    int message_id){}
  void remote_start_request(const cdap_rib::con_handle_t &con,
                                    const cdap_rib::obj_info_t &obj,
                                    const cdap_rib::filt_info_t &filt,
                                    int message_id){}
  void remote_stop_request(const cdap_rib::con_handle_t &con,
                                   const cdap_rib::obj_info_t &obj,
                                   const cdap_rib::filt_info_t &filt,
                                   int message_id){}
};

static string durationToString(
    const std::chrono::high_resolution_clock::duration& dur) {
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
      dealloc_wait(dw) {
}

Client::~Client()
{
  delete manager_;
}

void Client::run() {
  Flow *flow;

  if (client_app_reg) {
    applicationRegister();
  }
  flow = createFlow();
  if (flow) {
    echoFlow(flow);
  }
  if (flow) {
    if (dealloc_wait > 0) {
      sleep(dealloc_wait);
    }
    destroyFlow(flow);
  }
}

Flow* Client::createFlow() {
  Flow* flow = 0;
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

  flow = ipcManager->commitPendingFlow(afrrevent->sequenceNumber,
                                       afrrevent->portId, afrrevent->difName);
  if (!flow || flow->getPortId() == -1) {
    LOG_ERR("Failed to allocate a flow");
    return 0;
  } else {
    LOG_DBG("[DEBUG] Port id = %d", flow->getPortId());
  }

  std::chrono::high_resolution_clock::time_point eindtp =
      std::chrono::high_resolution_clock::now();
  std::chrono::high_resolution_clock::duration dur = eindtp - begintp;

  if (!quiet) {
    cout << "Flow allocation time = " << durationToString(dur) << endl;
  }

  return flow;
}


bool Client::cacep(Flow *flow) {
  APPcallback callback = new APPcallback();
  cdap::CDAPProviderInterface* cdap_prov = cdap::CDAPProviderFactory->create(2000, false, )("GPB", 2000, false);
  cdap_rib::vers_info_t ver;
  ver.version_ = 1;
  cdap_rib::src_info_t src;
  src.ap_name_ = flow->getLocalApplicationName();
  src.ae_name_ = "A instance";
  src.ap_inst_ = 1;
  src.ae_inst_ = 1;
  cdap_rib::dest_info_t dest;
  dest.ap_name_ = flow->getRemoteApplcationName();
  dest.ae_name_ = "B instance";
  dest.ap_inst_ = 1;
  dest.ae_inst_ = 1;
  cdap_rib::auth_info auth;
  auth.auth_mech_ = auth.AUTH_NONE;
  int port = flow->getPortId();
  cdap_prov->open_connection(ver, src, dest, auth, port);
}

/*
bool Client::cacep(Flow *flow) {
  rina::WireMessageProviderFactory wire_factory;
  rina::CDAPSessionManagerFactory factory;
  char buffer[max_sdu_size_in_bytes], *bytes_read;
  int n_bytes_read;
  const CDAPMessage *m_sent, *m_rcv;

  manager_ = factory.createCDAPSessionManager(&wire_factory, 2000);
  manager_->createCDAPSession(flow->getPortId());

  // M_CONNECT
  AuthValue auth_value;
  m_sent = manager_->getOpenConnectionRequestMessage(flow->getPortId(),
                                                     CDAPMessage::AUTH_NONE,
                                                     auth_value, "1",
                                                     "B instance", "1", "B",
                                                     "1", "A instance", "1",
                                                     "A");
  const SerializedObject *ser_sent_m = manager_->encodeNextMessageToBeSent(
      *m_sent, flow->getPortId());
  manager_->messageSent(*m_sent, flow->getPortId());
  flow->writeSDU(ser_sent_m->message_, ser_sent_m->size_);
  delete ser_sent_m;
  delete m_sent;

  //M_CONNECT_R
  try {
    n_bytes_read = flow->readSDU(buffer, max_sdu_size_in_bytes);
  } catch (rina::IPCException &e) {
    std::cout << "CACEP not stablished. Exception while reading SDU: "
              << e.what() << std::endl;
    return false;
  }
  bytes_read = new char[n_bytes_read];
  memcpy(bytes_read, buffer, n_bytes_read);
  SerializedObject ser_rec_m(bytes_read, n_bytes_read);
  m_rcv = manager_->messageReceived(ser_rec_m, flow->getPortId());
  delete m_rcv;
  std::cout << "CACEP stablished" << std::endl;

  return true;
}
*/
bool Client::release(rina::Flow *flow) {
  char buffer[max_sdu_size_in_bytes], *bytes_read;
  int n_bytes_read;
  const CDAPMessage *m_sent, *m_rcv;

  // M_RELEASE
  m_sent = manager_->getReleaseConnectionRequestMessage(flow->getPortId(),
                                                        CDAPMessage::NONE_FLAGS,
                                                        true);
  const SerializedObject *ser_sent_m = manager_->encodeNextMessageToBeSent(
      *m_sent, flow->getPortId());
  manager_->messageSent(*m_sent, flow->getPortId());
  flow->writeSDU(ser_sent_m->message_, ser_sent_m->size_);
  std::cout << "Sent RELEASE request with invoke id " << m_sent->invoke_id_
            << std::endl;
  delete ser_sent_m;
  delete m_sent;

  //M_RELEASE_R
  try {
    n_bytes_read = flow->readSDU(buffer, max_sdu_size_in_bytes);
  } catch (rina::IPCException &e) {
    std::cout << "RELEASE response problems. Exception while reading SDU: "
              << e.what() << std::endl;
    return false;
  }
  bytes_read = new char[n_bytes_read];
  memcpy(bytes_read, buffer, n_bytes_read);
  SerializedObject ser_rec_m(bytes_read, n_bytes_read);
  m_rcv = manager_->messageReceived(ser_rec_m, flow->getPortId());
  delete m_rcv;
  std::cout << "Connection released" << std::endl;

  return true;
}

void Client::echoFlow(Flow* flow) {
  int n_bytes_read;
  const CDAPMessage *m_sent, *m_rcv;
  char buffer[max_sdu_size_in_bytes], *bytes_read;
  bool keep_running = true;

  ulong n = 0;
  random_device rd;
  default_random_engine ran(rd());
  uniform_int_distribution<int> dis(0, 255);

  // CACEP
  cacep(flow);

  while (keep_running) {
    IPCEvent* event = ipcEventProducer->eventPoll();

    if (event) {
      switch (event->eventType) {
        case FLOW_DEALLOCATED_EVENT:
          flow = 0;
          keep_running = false;
          break;
        default:
          LOG_INFO("Client got new event %d", event->eventType);
          break;
      }
    } else if (n < echo_times) {
      std::chrono::high_resolution_clock::time_point begintp, endtp;

      // READ
      m_sent = manager_->getReadObjectRequestMessage(flow->getPortId(), 0,
                                                     CDAPMessage::NONE_FLAGS,
                                                     "TEST class", 1,
                                                     "TEST name", 1, true);
      const SerializedObject *ser_sent_m = manager_->encodeNextMessageToBeSent(
          *m_sent, flow->getPortId());
      manager_->messageSent(*m_sent, flow->getPortId());

      begintp = std::chrono::high_resolution_clock::now();
      flow->writeSDU(ser_sent_m->message_, ser_sent_m->size_);
      std::cout << "Sent READ request with invoke id " << m_sent->invoke_id_
                << std::endl;
      delete ser_sent_m;
      delete m_sent;

      // READ_R
      try {
        n_bytes_read = flow->readSDU(buffer, max_sdu_size_in_bytes);
        endtp = std::chrono::high_resolution_clock::now();
      } catch (rina::IPCException &e) {
        std::cout << "Exception while reading SDU: " << e.what() << std::endl;
        break;
      }
      bytes_read = new char[n_bytes_read];
      memcpy(bytes_read, buffer, n_bytes_read);
      SerializedObject ser_rec_m(bytes_read, n_bytes_read);
      m_rcv = manager_->messageReceived(ser_rec_m, flow->getPortId());
      cout << "READ response received with invoke id " << m_rcv->invoke_id_
          << ", RTT = " << durationToString(endtp - begintp) << std::endl;
      delete m_rcv;
      n++;
      if (n < echo_times) {
        this_thread::sleep_for(std::chrono::milliseconds(wait));
      }
    } else {
      release(flow);
      keep_running = false;
    }
  }
}

void Client::destroyFlow(Flow *flow) {
  DeallocateFlowResponseEvent *resp = 0;
  unsigned int seqnum;
  IPCEvent* event;
  int port_id = flow->getPortId();

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

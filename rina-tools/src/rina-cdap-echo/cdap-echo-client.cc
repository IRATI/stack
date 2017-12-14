//
// Echo CDAP Client
//
// Addy Bombeke <addy.bombeke@ugent.be>
// Vincenzo Maffione <v.maffione@nextworks.it>
// Bernat Gastón <bernat.gaston@i2cat.net>
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
#include <cassert>
#include <unistd.h>
#include <cerrno>

#define RINA_PREFIX     "cdap-echo-client"
#include <librina/logs.h>
#include <librina/cdap_v2.h>
#include <librina/security-manager.h>

#include "cdap-echo-client.h"

using namespace std;
using namespace rina;

Client::Client(const std::list<std::string>& dif_nms, const string& apn, const string& api,
               const string& server_apn, const string& server_api, bool q,
               unsigned long count, bool registration, unsigned int w, int g,
               int dw)
        : Application(dif_nms, apn, api),
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
    if (client_app_reg)
    {
        applicationRegister();
    }
    createFlow();
    if (flow_.portId >= 0)
    {
        // CACEP
        cacep();

        sendReadRMessage();
    }
}

void Client::createFlow()
{
    AllocateFlowRequestResultEvent* afrrevent;
    FlowSpecification qosspec;
    IPCEvent* event;
    uint seqnum;

    qosspec.msg_boundaries = true;
    if (gap >= 0)
        qosspec.maxAllowableGap = gap;

    if (dif_names.front() != string())
    {
        seqnum = ipcManager->requestFlowAllocationInDIF(ApplicationProcessNamingInformation(app_name,
        										    app_instance),
        						ApplicationProcessNamingInformation(server_name,
        										    server_instance),
						        ApplicationProcessNamingInformation(dif_names.front(),
						        				    string()),
						        qosspec);
    } else
    {
        seqnum = ipcManager->requestFlowAllocation(ApplicationProcessNamingInformation(app_name,
        									       app_instance),
        					   ApplicationProcessNamingInformation(server_name,
        							   	   	       server_instance),
						   qosspec);
    }

    for (;;)
    {
        event = ipcEventProducer->eventWait();
        if (event && event->eventType == ALLOCATE_FLOW_REQUEST_RESULT_EVENT
                && event->sequenceNumber == seqnum)
        {
            break;
        }
        LOG_DBG("Client got new event %d", event->eventType);
    }

    afrrevent = dynamic_cast<AllocateFlowRequestResultEvent*>(event);

    flow_ = ipcManager->commitPendingFlow(afrrevent->sequenceNumber,
                                          afrrevent->portId,
                                          afrrevent->difName);
    if (flow_.portId < 0)
    {
        LOG_ERR("Failed to allocate a flow");
    } else
    {
        if (fcntl(flow_.fd, F_SETFL, O_NONBLOCK)) {
                LOG_ERR("Failed to set O_NONBLOCK for port id = %d", flow_.portId);
        }
        LOG_DBG("[DEBUG] Port id = %d", flow_.portId);
    }
}

void Client::cacep()
{
    unsigned char buffer[max_buffer_size];
    rina::cdap_rib::concrete_syntax_t syntax;
    cdap::init(this, syntax, flow_.fd);
    cdap_prov_ = cdap::getProvider();
    cdap_rib::vers_info_t ver;
    ver.version_ = 1;
    cdap_rib::ep_info_t src;
    int bytes_read = 0;

    src.ap_name_ = flow_.localAppName.processName;
    src.ae_name_ = flow_.localAppName.entityName;
    src.ap_inst_ = flow_.localAppName.processInstance;
    src.ae_inst_ = flow_.localAppName.entityInstance;
    cdap_rib::ep_info_t dest;
    dest.ap_name_ = flow_.remoteAppName.processName;
    dest.ae_name_ = flow_.remoteAppName.entityName;
    dest.ap_inst_ = flow_.remoteAppName.processInstance;
    dest.ae_inst_ = flow_.remoteAppName.entityInstance;
    cdap_rib::auth_policy auth;
    auth.name = rina::IAuthPolicySet::AUTH_NONE;

    std::cout << "open conection request CDAP message sent" << std::endl;
    con_ = cdap_prov_->remote_open_connection(ver, src, dest, auth,
                                              flow_.portId);
    while (true)
    {
            bytes_read = read(flow_.fd, buffer, max_buffer_size);
            if (bytes_read < 0 && errno == EAGAIN) {
                sleep_wrapper.sleepForMili(50);
            } else if (bytes_read <= 0) {
                LOG_ERR("Problems while reading or EOF: %s", strerror(errno));
                break;
            } else {
                break;
            }
    }

    if (bytes_read <= 0) {
        return;
    }

    ser_obj_t message;
    message.message_ = buffer;
    message.size_ = bytes_read;
    cdap_prov_->process_message(message, flow_.portId);
}

void Client::open_connection_result(const cdap_rib::con_handle_t &con,
                                    const cdap_rib::result_info &res)
{
    std::cout << "open conection response CDAP message received" << std::endl;
}
void Client::remote_read_result(const rina::cdap_rib::con_handle_t &con,
                                const rina::cdap_rib::obj_info_t &obj,
                                const rina::cdap_rib::res_info_t &res,
                                const rina::cdap_rib::flags_t &flags,
                                const int invoke_id)
{
    std::cout << "read response CDAP message received" << std::endl;
}

void Client::close_connection_result(const cdap_rib::con_handle_t &con,
                                     const cdap_rib::result_info &res)
{
    std::cout << "release response CDAP message received" << std::endl;
    destroyFlow();
}

void Client::sendReadRMessage()
{
    while (count_ < echo_times)
    {
        IPCEvent* event = ipcEventProducer->eventWait();
        unsigned char buffer[max_buffer_size];
        if (event)
        {
            switch (event->eventType) {
                case FLOW_DEALLOCATED_EVENT:
                    std::cout << "Client received a flow deallocation event"
                              << std::endl;
                    destroyFlow();
                    break;
                default:
                    LOG_INFO("Client got new event %d", event->eventType);
                    break;
            }
        } else
        {
            // READ
            cdap_rib::obj_info_t obj;
            cdap_rib::auth_policy_t auth;
            int bytes_read = 0;
            obj.name_ = "test name";
            obj.class_ = "test class";
            obj.inst_ = 1;
            cdap_rib::flags_t flags;
            flags.flags_ = cdap_rib::flags_t::NONE_FLAGS;
            cdap_rib::filt_info_t filt;
            filt.filter_ = 0;
            filt.scope_ = 0;
            cdap_prov_->remote_read(con_, obj, flags, filt, auth, 35);
            std::cout << "read request CDAP message sent" << std::endl;

            while (true)
            {
                    bytes_read = read(flow_.fd, buffer, max_buffer_size);
                    if (bytes_read < 0 && errno == EAGAIN) {
                        sleep_wrapper.sleepForMili(50);
                    } else if (bytes_read <= 0) {
                        LOG_ERR("Problems while reading or EOF: %s", strerror(errno));
                        break;
                    } else {
                        break;
                    }
            }

            if (bytes_read > 0) {
                    ser_obj_t message;
                    message.message_ = buffer;
                    message.size_ = bytes_read;
                    cdap_prov_->process_message(message, flow_.portId);
                    count_++;
                    std::cout << "count: " << count_ << std::endl;
            }
        }
    }
    release();
}

void Client::release()
{
    unsigned char buffer[max_buffer_size];
    std::cout << "release request CDAP message sent" << std::endl;
    int bytes_read = 0;
    cdap_prov_->remote_close_connection(con_.port_id, false);
    std::cout << "Waiting for release response" << std::endl;

    while (true)
    {
            bytes_read = read(flow_.fd, buffer, max_buffer_size);
            if (bytes_read < 0 && errno == EAGAIN) {
                sleep_wrapper.sleepForMili(50);
            } else if (bytes_read <= 0) {
                LOG_ERR("Problems while reading or EOF: %s", strerror(errno));
                break;
            } else {
                break;
            }
    }

    if (bytes_read < 0) {
        return;
    }

    ser_obj_t message;
    message.message_ = buffer;
    message.size_ = bytes_read;
    cdap_prov_->process_message(message, flow_.portId);
    std::cout << "Release completed" << std::endl;
}

void Client::destroyFlow()
{
    cdap::destroy(flow_.portId);
    ipcManager->deallocate_flow(flow_.portId);
}

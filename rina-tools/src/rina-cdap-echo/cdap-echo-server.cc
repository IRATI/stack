//
// Echo CDAP Server
// 
// Addy Bombeke <addy.bombeke@ugent.be>
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
#include <time.h>
#include <signal.h>
#include <time.h>
#include <cerrno>

#define RINA_PREFIX     "rina-cdap-echo-server"
#include <librina/logs.h>

#include "cdap-echo-server.h"

using namespace std;
using namespace rina;

ConnectionCallback::ConnectionCallback(bool *keep_serving){
	keep_serving_ = keep_serving;
}

void ConnectionCallback::open_connection(rina::cdap_rib::con_handle_t &con,
					 const rina::cdap::CDAPMessage& m)
{
	cdap_rib::res_info_t res;
	res.code_ = rina::cdap_rib::CDAP_SUCCESS;
	int message_id = m.invoke_id_;
	std::cout<<"open conection request CDAP message received"<<std::endl;
	get_provider()->send_open_connection_result(con, res, message_id);
	std::cout<<"open conection response CDAP message sent"<<std::endl;
}

void ConnectionCallback::remote_read_request(
		const rina::cdap_rib::con_handle_t &con,
		const rina::cdap_rib::obj_info_t &obj,
		const rina::cdap_rib::filt_info_t &filt, int message_id)
{
	cdap_rib::flags_t flags;
	flags.flags_ = cdap_rib::flags_t::NONE_FLAGS;
	cdap_rib::res_info_t res;
	res.code_ = rina::cdap_rib::CDAP_SUCCESS;
	std::cout<<"read request CDAP message received"<<std::endl;
	get_provider()->send_read_result(con, obj, flags, res, message_id);
	std::cout<<"read response CDAP message sent"<<std::endl;
}

void ConnectionCallback::close_connection(const rina::cdap_rib::con_handle_t &con,
		const rina::cdap_rib::flags_t &flags, int message_id)
{
	cdap_rib::res_info_t res;
	res.code_ = rina::cdap_rib::CDAP_SUCCESS;
	std::cout<<"conection close request CDAP message received"<<std::endl;
	get_provider()->send_close_connection_result(con.port_id, flags, res, message_id);
	std::cout<<"conection close response CDAP message sent"<<std::endl;
	*keep_serving_ = false;
}

CDAPEchoWorker::CDAPEchoWorker(int port_id, int fd,
		     	       unsigned int max_sdu_size,
		     	       Server * serv) : ServerWorker(serv),
                                port_id(port_id), fd(fd), max_sdu_size(max_sdu_size)
{
}

int CDAPEchoWorker::internal_run()
{
	serveEchoFlow();
	return 0;
}

void CDAPEchoWorker::serveEchoFlow()
{
	bool keep_serving = true;
	unsigned char buffer[max_sdu_size];
	rina::cdap::CDAPProviderInterface *cdap_prov;
	rina::cdap_rib::concrete_syntax_t syntax;
	int bytes_read = 0;

	ConnectionCallback callback(&keep_serving);
	cdap::init(&callback, syntax, fd);

	cdap_prov = cdap::getProvider();

	while (keep_serving) {
                while (true) {
                        bytes_read = read(fd, buffer, max_sdu_size);
                        if (bytes_read < 0 && errno == EAGAIN) {
                                sleep_wrapper.sleepForMili(50);
                        } else {
                                if (bytes_read <= 0) {
                                        LOG_ERR("Error while reading from flow %d [%s] or EOF",
                                                        port_id, strerror(errno));
                                }
                                break;
                        }
                }

                if (bytes_read <= 0) {
                        break;
                }

		ser_obj_t message;
		message.message_ = buffer;
		message.size_ = bytes_read;
		cdap_prov->process_message(message, port_id);
	}

	delete cdap_prov;
}

const unsigned int CDAPEchoServer::max_sdu_size_in_bytes = 10000;

CDAPEchoServer::CDAPEchoServer(const list<string>& dif_names,
			       const string& app_name,
			       const string& app_instance,
			       const int dealloc_wait)
			    : Server(dif_names, app_name, app_instance),
			      dw(dealloc_wait)
{
}

ServerWorker * CDAPEchoServer::internal_start_worker(rina::FlowInformation flow)
{
	CDAPEchoWorker * worker = new CDAPEchoWorker(flow.portId, flow.fd,
						     max_sdu_size_in_bytes,
						     this);
	worker->start();
        worker->detach();
        return worker;
}

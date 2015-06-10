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

CDAPEchoWorker::CDAPEchoWorker(rina::ThreadAttributes * threadAttributes,
		     	       int port,
		     	       unsigned int max_size,
		     	       Server * serv) : ServerWorker(threadAttributes, serv)
{
	port_id = port;
	max_sdu_size = max_size;
}

int CDAPEchoWorker::internal_run()
{
	serveEchoFlow(port_id);
	return 0;
}

void CDAPEchoWorker::serveEchoFlow(int port_id)
{
	bool keep_serving = true;
	char buffer[max_sdu_size];
	rina::cdap::CDAPProviderInterface *cdap_prov = 0;
	int bytes_read = 0;

	ConnectionCallback* callback = new ConnectionCallback(&keep_serving, &cdap_prov);
	std::cout<<"cdap_prov created"<<std::endl;

	cdap::CDAPProviderFactory::init(2000);
	cdap_prov = cdap::CDAPProviderFactory::create(false, callback);

	while (keep_serving) {
		try {
			while (true) {
				try {
					bytes_read = ipcManager->readSDU(port_id,
							buffer,
							max_sdu_size);
					break;
				} catch (TryAgainException &e) {
					sleep_wrapper.sleepForMili(50);
				}
			}
		} catch(rina::UnknownFlowException &e) {
			LOG_ERR("Unknown flow descriptor");
			break;
		} catch(rina::FlowNotAllocatedException &e) {
			LOG_ERR("Flow has been deallocated");
			break;
		} catch (rina::Exception &e){
			LOG_ERR("Got unkown exception while reading %s", e.what());
			break;
		}

		cdap_rib::SerializedObject message;
		message.message_ = buffer;
		message.size_ = bytes_read;
		cdap_prov->process_message(message, port_id);
	}

	cdap::CDAPProviderFactory::destroy(port_id);
	delete cdap_prov;
	delete callback;
}

const unsigned int CDAPEchoServer::max_sdu_size_in_bytes = 10000;

CDAPEchoServer::CDAPEchoServer(const string& dif_name,
			       const string& app_name,
			       const string& app_instance,
			       const int dealloc_wait)
    : Server(dif_name, app_name, app_instance),
      dw(dealloc_wait)
{
}

ServerWorker * CDAPEchoServer::internal_start_worker(int port_id)
{
	ThreadAttributes threadAttributes;
	CDAPEchoWorker * worker = new CDAPEchoWorker(&threadAttributes,
						     port_id,
						     max_sdu_size_in_bytes,
						     this);
        worker->detach();
        return worker;
}

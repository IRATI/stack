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
#define RINA_PREFIX     "manager"
#include <librina/logs.h>
#include <librina/cdap_v2.h>
#include <librina/common.h>
#include <rinad/ipcm/encoders_mad.h>

#include "manager.h"

using namespace std;
using namespace rina;
using namespace rinad;

const std::string Manager::mad_name = "mad";
const std::string Manager::mad_instance = "1";

ConnectionCallback::ConnectionCallback(
		rina::cdap::CDAPProviderInterface **prov) {
	prov_ = prov;
}

void ConnectionCallback::open_connection(
		const rina::cdap_rib::con_handle_t &con,
		const rina::cdap_rib::flags_t &flags, int message_id) {
	cdap_rib::res_info_t res;
	res.result_ = 1;
	res.result_reason_ = "Ok";
	std::cout << "open conection request CDAP message received"
			<< std::endl;
	(*prov_)->open_connection_response(con, res, message_id);
	std::cout << "open conection response CDAP message sent" << std::endl;
}

void ConnectionCallback::remote_create_result(
		const rina::cdap_rib::con_handle_t &con,
		const rina::cdap_rib::obj_info_t &obj,
		const rina::cdap_rib::res_info_t &res) {
	std::cout << "Result code is: " << res.result_ << std::endl;
}

void ConnectionCallback::remote_read_result(
		const rina::cdap_rib::con_handle_t &con,
		const rina::cdap_rib::obj_info_t &obj,
		const rina::cdap_rib::res_info_t &res) {
	// decode object value
	// print object value
	std::cout << "Query Rib operation returned result " << res.result_
			<< std::endl;
	std::string query_rib;
	rinad::mad_manager::encoders::StringEncoder().decode(obj.value_,
								query_rib);
	std::cout << "QueryRIB:" << std::endl << query_rib << std::endl;
}

ManagerWorker::ManagerWorker(rina::ThreadAttributes * threadAttributes,
		     	     int port,
		     	     unsigned int max_size,
		     	     Server * serv) : ServerWorker(threadAttributes, serv)
{
	port_id = port;
	max_sdu_size = max_size;
	cdap_prov = 0;
}

int ManagerWorker::internal_run()
{
	operate(port_id);
	return 0;
}

void ManagerWorker::operate(int port_id)
{
        ConnectionCallback callback(&cdap_prov);
        std::cout << "cdap_prov created" << std::endl;
        cdap::CDAPProviderFactory::init(2000);
        cdap_prov = cdap::CDAPProviderFactory::create(false, &callback);
        // CACEP
        cacep(port_id);
        // CREATE IPCP
        createIPCP(port_id);
        // QUERY RIB
        queryRIB(port_id);
        // FINISH
        cdap::CDAPProviderFactory::destroy(port_id);
        delete cdap_prov;
}

void ManagerWorker::cacep(int port_id)
{
        char buffer[max_sdu_size_in_bytes];
        int bytes_read = ipcManager->readSDU(port_id, buffer, max_sdu_size_in_bytes);
        cdap_rib::SerializedObject message;
        message.message_ = buffer;
        message.size_ = bytes_read;
        cdap_prov->process_message(message, port_id);
}

void ManagerWorker::createIPCP(int port_id)
{
        char buffer[max_sdu_size_in_bytes];

        mad_manager::structures::ipcp_config_t ipc_config;
        ipc_config.process_instance = "1";
        ipc_config.process_name = "normal-1.IPCP";
        ipc_config.process_type = "normal-ipc";
        ipc_config.dif_to_assign = "normal.DIF";

        cdap_rib::obj_info_t obj;
        obj.name_ =
                        "root, computingSystemID = 1, processingSystemID=1, kernelApplicationProcess, osApplicationProcess, ipcProcesses, ipcProcessID=2";
        obj.class_ = "IPCProcess";
        obj.inst_ = 0;
        mad_manager::encoders::IPCPConfigEncoder().encode(ipc_config,
                                                          obj.value_);
        mad_manager::structures::ipcp_config_t object;
        mad_manager::encoders::IPCPConfigEncoder().decode(obj.value_, object);

        cdap_rib::flags_t flags;
        flags.flags_ = cdap_rib::flags_t::NONE_FLAGS;

        cdap_rib::filt_info_t filt;
        filt.filter_ = 0;
        filt.scope_ = 0;

        cdap_prov->remote_create(port_id, obj, flags, filt);
        std::cout << "create IPC request CDAP message sent" << std::endl;

        int bytes_read = ipcManager->readSDU(port_id,
        				     buffer,
        				     max_sdu_size_in_bytes);
        cdap_rib::SerializedObject message;
        message.message_ = buffer;
        message.size_ = bytes_read;
        cdap_prov->process_message(message, port_id);
}

void ManagerWorker::queryRIB(int port_id)
{
        char buffer[max_sdu_size_in_bytes];

	cdap_rib::obj_info_t obj;
	obj.name_ =
			"root, computingSystemID = 1, processingSystemID=1, kernelApplicationProcess, osApplicationProcess, ipcProcesses, ipcProcessID=2, RIBDaemon";
	obj.class_ = "RIBDaemon";
	obj.inst_ = 0;

	cdap_rib::flags_t flags;
	flags.flags_ = cdap_rib::flags_t::NONE_FLAGS;

	cdap_rib::filt_info_t filt;
	filt.filter_ = 0;
	filt.scope_ = 0;

        cdap_prov->remote_read(port_id, obj, flags, filt);
        std::cout << "Read RIBDaemon request CDAP message sent" << std::endl;

        int bytes_read = ipcManager->readSDU(port_id,
        				     buffer,
        				     max_sdu_size_in_bytes);
        cdap_rib::SerializedObject message;
        message.message_ = buffer;
        message.size_ = bytes_read;
        cdap_prov->process_message(message, port_id);
}

Manager::Manager(const std::string& dif_name, const std::string& apn,
			const std::string& api)
		: Server(dif_name, apn, api)
{
}

ServerWorker * Manager::internal_start_worker(int port_id)
{
	ThreadAttributes threadAttributes;
	ManagerWorker * worker = new ManagerWorker(&threadAttributes,
						   port_id,
						   max_sdu_size_in_bytes,
						   this);
	worker->start();
        worker->detach();
        return worker;
}

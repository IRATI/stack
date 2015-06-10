//
// Manager
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

#ifndef MANAGER_HPP
#define MANAGER_HPP

#include <string>
#include <librina/application.h>
#include <librina/cdap_v2.h>
#include "server.h"

static const unsigned int max_sdu_size_in_bytes = 10000;

class ConnectionCallback : public rina::cdap::CDAPCallbackInterface {
 public:
	ConnectionCallback(rina::cdap::CDAPProviderInterface **prov);
	void open_connection(const rina::cdap_rib::con_handle_t &con,
				const rina::cdap_rib::flags_t &flags,
				int message_id);
	void remote_create_result(const rina::cdap_rib::con_handle_t &con,
					const rina::cdap_rib::obj_info_t &obj,
					const rina::cdap_rib::res_info_t &res);
	void remote_read_result(const rina::cdap_rib::con_handle_t &con,
				const rina::cdap_rib::obj_info_t &obj,
				const rina::cdap_rib::res_info_t &res);
 private:
	rina::cdap::CDAPProviderInterface **prov_;
};

class ManagerWorker : public ServerWorker {
public:
	ManagerWorker(rina::ThreadAttributes * threadAttributes,
		      int port,
		      unsigned int max_sdu_size,
	              Server * serv);
	~ManagerWorker() throw() { };
	int internal_run();

private:
        void operate(int port_id);
        void cacep(int port_id);
        void createIPCP(int port_id);
        void queryRIB(int port_id);

	int port_id;
	unsigned int max_sdu_size;
	rina::cdap::CDAPProviderInterface *cdap_prov;
};

class Manager : public Server {
 public:
	Manager(const std::string& dif_name,
		const std::string& apn,
		const std::string& api);
	~Manager() { };

 protected:
 	ServerWorker * internal_start_worker(int port_id);

 private:
	rina::cdap_rib::con_handle_t con_;
	static const std::string mad_name;
	static const std::string mad_instance;
};
#endif//MANAGER_HPP

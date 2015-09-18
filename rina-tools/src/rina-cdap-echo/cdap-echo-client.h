//
// Echo CDAP Client
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

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <librina/librina.h>
#include <librina/cdap_v2.h>

#include "application.h"

class Client;

class Client : public Application, public rina::cdap::CDAPCallbackInterface
{
	friend class APPcallback;
 public:
	Client(const std::string& dif_name, const std::string& apn,
	       const std::string& api, const std::string& server_apn,
	       const std::string& server_api, bool quiet, unsigned long count,
	       bool registration, unsigned int wait, int g, int dw);
	void run();
	~Client();
	void open_connection_result(const rina::cdap_rib::con_handle_t &con,
			            const rina::cdap_rib::result_info &res);
	void close_connection_result(const rina::cdap_rib::con_handle_t &con,
			             const rina::cdap_rib::result_info &res);
	void remote_read_result(const rina::cdap_rib::con_handle_t &con,
			        const rina::cdap_rib::obj_info_t &onj,
			        const rina::cdap_rib::res_info_t &res);
 protected:
	void createFlow();
	void cacep();
	void sendReadRMessage();
	void release();
	void destroyFlow();

 private:
	std::string dif_name;
	std::string server_name;
	std::string server_instance;
	bool quiet;
	unsigned long echo_times;  // -1 is infinite
	bool client_app_reg;
	unsigned int wait;
	int gap;
	int dealloc_wait;
	rina::FlowInformation flow_;
	rina::cdap::CDAPProviderInterface* cdap_prov_;
	rina::cdap_rib::con_handle_t con_;
	unsigned long count_;
	bool keep_running_;
	rina::Sleep sleep_wrapper;
};

#endif//CLIENT_HPP

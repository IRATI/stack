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

static const unsigned int max_sdu_size_in_bytes = 10000;

class Application {
 public:
	Application(const std::string& dif_name_, const std::string & app_name_,
			const std::string & app_instance_);

	static const uint max_buffer_size;

 protected:
	void applicationRegister();

	std::string dif_name;
	std::string app_name;
	std::string app_instance;

};

class Manager : public Application {
 public:
	Manager(const std::string& dif_name, const std::string& apn,
		const std::string& api);
	void run();
	~Manager();
 protected:
        void startWorker(rina::FlowInformation &flow);
        void operate(rina::FlowInformation flow);
        bool cacep(rina::FlowInformation &flow);
        bool createIPCP_1(rina::FlowInformation &flow);
        bool createIPCP_2(rina::FlowInformation &flow);
        bool createIPCP_3(rina::FlowInformation &flow);
        void queryRIB(rina::FlowInformation &flow, std::string name);
 private:
        const std::string IPCP_1 = "root, computingSystemID = 1, processingSystemID=1, kernelApplicationProcess, osApplicationProcess, ipcProcesses, ipcProcessID=4";
        const std::string IPCP_2 = "root, computingSystemID = 1, processingSystemID=1, kernelApplicationProcess, osApplicationProcess, ipcProcesses, ipcProcessID=6";
        const std::string IPCP_3 = "root, computingSystemID = 1, processingSystemID=1, kernelApplicationProcess, osApplicationProcess, ipcProcesses, ipcProcessID=4";
	std::string dif_name_;
	bool client_app_reg_;
	rina::cdap_rib::con_handle_t con_;
	rina::cdap::CDAPProviderInterface *cdap_prov_;
};
#endif//MANAGER_HPP

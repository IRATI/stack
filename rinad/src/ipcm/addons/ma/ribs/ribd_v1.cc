/*
 * RIB factory
 *
 *    Bernat Gaston         <bernat.gaston@i2cat.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

#include "ribd_v1.h"

#define RINA_PREFIX "ipcm.mad.ribd_v1"
#include <librina/logs.h>
#include <librina/exceptions.h>
#include "../../../ipcm.h"
#include "../ribf.h"

//Object definitions
#include "ipcp_obj.h"

namespace rinad {
namespace mad {
namespace rib_v1 {

const char IPCP_NAME[] = "/computingSystemID=1/processingSystemID=1/kernelApplicationProcess/osApplicationProcess/ipcProcesses";
// Create the schema
void createSchema(void){
	rina::cdap_rib::vers_info_t vers;
	uint64_t version = 0x1ULL;
	rina::rib::RIBDaemonProxy *const ribd = RIBFactory::getProxy();

	vers.version_ = version;

	//Create schema
	ribd->createSchema(vers);

	const std::string IPCProcesses_name = IPCP_NAME;

	// Create callbacks
	ribd->addCreateCallbackSchema(vers, "IPCProcess", IPCProcesses_name, IPCPObj::create_cb);
}


// Create and initialize the RIB with the current status
rina::rib::rib_handle_t createRIB(void){

	rina::rib::RIBObj* tmp;
	rina::cdap_rib::vers_info_t vers;
	uint64_t version = 0x1ULL;
	rina::rib::RIBDaemonProxy *const ribd = RIBFactory::getProxy();

	//Create the RIB
	vers.version_ = version;
	rina::rib::rib_handle_t rib = ribd->createRIB(vers);

	try {
		tmp = new rina::rib::RIBObj("DAF");
		ribd->addObjRIB(rib, "/dafID=1", &tmp);

		tmp = new rina::rib::RIBObj("ComputingSystem");
		ribd->addObjRIB(rib,"/computingSystemID=1", &tmp);

		tmp = new rina::rib::RIBObj("ProcessingSystem");
		ribd->addObjRIB(rib, "/computingSystemID=1/processingSystemID=1", &tmp);

		tmp = new rina::rib::RIBObj("Software");
		ribd->addObjRIB(rib, "/computingSystemID=1/processingSystemID=1/software", &tmp);

		tmp = new rina::rib::RIBObj("Hardware");
		ribd->addObjRIB(rib, "/computingSystemID=1/processingSystemID=1/hardware", &tmp);

		tmp = new rina::rib::RIBObj("KernelApplicationProcess");
		ribd->addObjRIB(rib, "/computingSystemID=1/processingSystemID=1/kernelApplicationProcess", &tmp);

		tmp = new rina::rib::RIBObj("OSApplicationProcess");
		ribd->addObjRIB(rib, "/computingSystemID=1/processingSystemID=1/kernelApplicationProcess/osApplicationProcess", &tmp);

		const std::string IPCProcesses_name = IPCP_NAME;
		tmp = new rina::rib::RIBObj("IPCProcesses");
		ribd->addObjRIB(rib, IPCProcesses_name, &tmp);

		tmp = new rina::rib::RIBObj("ManagementAgents");
		ribd->addObjRIB(rib, "/computingSystemID=1/processingSystemID=1/kernelApplicationProcess/osApplicationProcess/managementAgents", &tmp);

		tmp = new rina::rib::RIBObj("ManagementAgent");
		ribd->addObjRIB(rib, "/computingSystemID=1/processingSystemID=1/kernelApplicationProcess/osApplicationProcess/managementAgents/managementAgentID=1", &tmp);

		// IPCManagement branch
		tmp = new rina::rib::RIBObj("IPCManagement");
		ribd->addObjRIB(rib, "/computingSystemID=1/processingSystemID=1/kernelApplicationProcess/osApplicationProcess/managementAgents/managementAgentID=1/ipcManagement", &tmp);

		tmp = new rina::rib::RIBObj("IPCResourceManager");
		ribd->addObjRIB(rib, "/computingSystemID=1/processingSystemID=1/kernelApplicationProcess/osApplicationProcess/managementAgents/managementAgentID=1/ipcManagement/ipcResourceManager", &tmp);

		tmp = new rina::rib::RIBObj("UnderlayingFlows");
		ribd->addObjRIB(rib, "/computingSystemID=1/processingSystemID=1/kernelApplicationProcess/osApplicationProcess/managementAgents/managementAgentID=1/ipcManagement/ipcResourceManager/underlayingFlows", &tmp);

		tmp = new rina::rib::RIBObj("UnderlayingDIFs");
		ribd->addObjRIB(rib, "/computingSystemID=1/processingSystemID=1/kernelApplicationProcess/osApplicationProcess/managementAgents/managementAgentID=1/ipcManagement/ipcResourceManager/underlayingDIFs", &tmp);

		tmp = new rina::rib::RIBObj("QueryDIFAllocator");
		ribd->addObjRIB(rib, "/computingSystemID=1/processingSystemID=1/kernelApplicationProcess/osApplicationProcess/managementAgents/managementAgentID=1/ipcManagement/ipcResourceManager/queryDIFAllocator", &tmp);

		tmp = new rina::rib::RIBObj("UnderlayingRegistrations");
		ribd->addObjRIB(rib, "/computingSystemID=1/processingSystemID=1/kernelApplicationProcess/osApplicationProcess/managementAgents/managementAgentID=1/ipcManagement/ipcResourceManager/underlayingRegistrations", &tmp);

		tmp = new rina::rib::RIBObj("SDUPRotection");
		ribd->addObjRIB(rib, "/computingSystemID=1/processingSystemID=1/kernelApplicationProcess/osApplicationProcess/managementAgents/managementAgentID=1/ipcManagement/sduProtection", &tmp);
		// RIBDaemon branch
		tmp = new rina::rib::RIBObj("RIBDaemon");
		ribd->addObjRIB(rib, "/computingSystemID=1/processingSystemID=1/kernelApplicationProcess/osApplicationProcess/managementAgents/managementAgentID=1/ribDaemon", &tmp);

		tmp = new rina::rib::RIBObj("Discriminators");
		ribd->addObjRIB(rib, "/computingSystemID=1/processingSystemID=1/kernelApplicationProcess/osApplicationProcess/managementAgents/managementAgentID=1/ribDaemon/discriminators", &tmp);
		// DIFManagement
		tmp = new rina::rib::RIBObj("DIFManagement");
		ribd->addObjRIB(rib, "/computingSystemID=1/processingSystemID=1/kernelApplicationProcess/osApplicationProcess/managementAgents/managementAgentID=1/difManagement", &tmp);

		//
		//Add the IPCPs
		//
		std::list<int> ipcps;
		IPCManager->list_ipcps(ipcps);

		std::list<int>::const_iterator it;
		for (it = ipcps.begin(); it != ipcps.end(); ++it)
			createIPCPObj(rib, *it);

	} catch (rina::Exception &e1) {
		LOG_ERR("RIB basic objects were not created because %s",
			e1.what());
	}

	return rib;
}


void destroyRIB(const rina::rib::rib_handle_t& rib){
	//uint64_t version = 0x1ULL;
	//vers.version_ = version;
	rina::rib::RIBDaemonProxy *const ribd = RIBFactory::getProxy();
	ribd->destroyRIB(rib);
}


void associateRIBtoAE(const rina::rib::rib_handle_t& rib,
						const std::string& ae_name){
	rina::rib::RIBDaemonProxy *const ribd = RIBFactory::getProxy();
	ribd->associateRIBtoAE(rib, ae_name);
}




void createIPCPObj(const rina::rib::rib_handle_t& rib, int ipcp_id){

	rina::rib::RIBDaemonProxy *const ribd = RIBFactory::getProxy();
	rina::rib::RIBObj* tmp;

	std::stringstream ss;
	ss << "/computingSystemID=1/processingSystemID=1/kernelApplicationProcess/osApplicationProcess/ipcProcesses/ipcProcessID=";
	ss << ipcp_id;
	try {
		std::stringstream ss;
		ss << "/computingSystemID=1/processingSystemID=1/kernelApplicationProcess/osApplicationProcess/ipcProcesses/";
		ss << "ipcProcessID=" << ipcp_id;
		tmp = new IPCPObj(ipcp_id);
		ribd->addObjRIB(rib, ss.str(), &tmp);

	} catch (...) {
		LOG_ERR("Unable to create an IPCP object '%s'; out of memory?",
			ss.str().c_str());
	}
}

void destroyIPCPObj(const rina::rib::rib_handle_t& rib, int ipcp_id){

	rina::rib::RIBDaemonProxy *const ribd = RIBFactory::getProxy();

	std::stringstream ss;
	ss << "/computingSystemID=1/processingSystemID=1/kernelApplicationProcess/osApplicationProcess/ipcProcesses/ipcProcessID=";
	ss << ipcp_id;
	try {
		int64_t inst_id = ribd->getObjInstId(rib, ss.str());
		if(inst_id < 0)
			throw rina::Exception("Invalid object to be removed");
		ribd->removeObjRIB(rib, inst_id);
	} catch (...) {
		LOG_ERR("Unable to delete an IPCP object '%s'",
			ss.str().c_str());
	}
}

}//namespace rib_v1;
}//namespace mad
}//namespace rinad


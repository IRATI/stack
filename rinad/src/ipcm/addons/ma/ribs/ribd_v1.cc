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

const char IPCP_NAME[] = "/csid=1/psid=1/kernelap/osap/ipcps";
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
		ribd->addObjRIB(rib,"/csid=1", &tmp);

		tmp = new rina::rib::RIBObj("ProcessingSystem");
		ribd->addObjRIB(rib, "/csid=1/psid=1", &tmp);

		tmp = new rina::rib::RIBObj("Software");
		ribd->addObjRIB(rib, "/csid=1/psid=1/software", &tmp);

		tmp = new rina::rib::RIBObj("Hardware");
		ribd->addObjRIB(rib, "/csid=1/psid=1/hardware", &tmp);

		tmp = new rina::rib::RIBObj("KernelApplicationProcess");
		ribd->addObjRIB(rib, "/csid=1/psid=1/kernelap", &tmp);

		tmp = new rina::rib::RIBObj("OSApplicationProcess");
		ribd->addObjRIB(rib, "/csid=1/psid=1/kernelap/osap", &tmp);

		const std::string IPCProcesses_name = IPCP_NAME;
		tmp = new rina::rib::RIBObj("IPCProcesses");
		ribd->addObjRIB(rib, IPCProcesses_name, &tmp);

		tmp = new rina::rib::RIBObj("ManagementAgents");
		ribd->addObjRIB(rib, "/csid=1/psid=1/kernelap/osap/mads", &tmp);

		tmp = new rina::rib::RIBObj("ManagementAgent");
		ribd->addObjRIB(rib, "/csid=1/psid=1/kernelap/osap/mads/mad=1", &tmp);

		// IPCManagement branch
		tmp = new rina::rib::RIBObj("IPCManagement");
		ribd->addObjRIB(rib, "/csid=1/psid=1/kernelap/osap/mads/mad=1/ipcm", &tmp);

		tmp = new rina::rib::RIBObj("IPCResourceManager");
		ribd->addObjRIB(rib, "/csid=1/psid=1/kernelap/osap/mads/mad=1/ipcm/ipcrm", &tmp);

		tmp = new rina::rib::RIBObj("UnderlayingFlows");
		ribd->addObjRIB(rib, "/csid=1/psid=1/kernelap/osap/mads/mad=1/ipcm/ipcrm/uflows", &tmp);

		tmp = new rina::rib::RIBObj("UnderlayingDIFs");
		ribd->addObjRIB(rib, "/csid=1/psid=1/kernelap/osap/mads/mad=1/ipcm/ipcrm/udifs", &tmp);

		tmp = new rina::rib::RIBObj("QueryDIFAllocator");
		ribd->addObjRIB(rib, "/csid=1/psid=1/kernelap/osap/mads/mad=1/ipcm/ipcrm/queryda", &tmp);

		tmp = new rina::rib::RIBObj("UnderlayingRegistrations");
		ribd->addObjRIB(rib, "/csid=1/psid=1/kernelap/osap/mads/mad=1/ipcm/ipcrm/uregs", &tmp);

		tmp = new rina::rib::RIBObj("SDUPRotection");
		ribd->addObjRIB(rib, "/csid=1/psid=1/kernelap/osap/mads/mad=1/ipcm/sdup", &tmp);
		// RIBDaemon branch
		tmp = new rina::rib::RIBObj("RIBDaemon");
		ribd->addObjRIB(rib, "/csid=1/psid=1/kernelap/osap/mads/mad=1/ribd", &tmp);

		tmp = new rina::rib::RIBObj("Discriminators");
		ribd->addObjRIB(rib, "/csid=1/psid=1/kernelap/osap/mads/mad=1/ribd/discs", &tmp);
		// DIFManagement
		tmp = new rina::rib::RIBObj("DIFManagement");
		ribd->addObjRIB(rib, "/csid=1/psid=1/kernelap/osap/mads/mad=1/difm", &tmp);

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

	try {
		std::stringstream ss;
		ss << "/csid=1/psid=1/kernelap/osap/ipcps/";
		ss << "ipcpid=" << ipcp_id;
		tmp = new IPCPObj(ipcp_id);
		ribd->addObjRIB(rib, ss.str(), &tmp);

	} catch (...) {
		LOG_ERR("Unable to create an IPCP object; out of memory?");
	}
}

void destroyIPCPObj(const rina::rib::rib_handle_t& rib, int ipcp_id){

	rina::rib::RIBDaemonProxy *const ribd = RIBFactory::getProxy();

	std::stringstream ss;
	ss << "/csid=1/psid=1/kernelap/osap/ipcps/ipcpid=";
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


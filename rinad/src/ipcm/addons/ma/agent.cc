/*
 * ManagementAgent class
 *
 *    Marc Sune <marc.sune (at) bisdn.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "agent.h"

//Subsystems
#include "bgtm.h"
#include "confm.h"
#include "flowm.h"
#include "ribf.h"
#include "ribs/ribd_v1.h"

#define RINA_PREFIX "ipcm.mad"
#include <librina/logs.h>

#include "../../ipcm.h"

// std libraries
#include <list>

namespace rinad {
namespace mad {

//Static members
ManagementAgent* ManagementAgent::inst = NULL;
const std::string ManagementAgent::NAME = "mad";

//
// Private methods
//


//Creates the NMS DIFs required by the MA
void ManagementAgent::bootstrapNMSDIFs(){
	//TODO FIXME XXX
	std::list<std::string>::const_iterator it;

	for(it=nmsDIFs.begin(); it!=nmsDIFs.end(); ++it) {
		//Nice trace
		LOG_INFO("Bootstraping DIF '%s'", it->c_str());

		//TODO FIXME XXX: call ipcmanager bootstrapping of the DIFs
	}

}
//Registers the application in the IPCManager
void ManagementAgent::reg(){

#if 0
        unsigned int seqnum;
	rina::ApplicationRegistrationInformation ari;
	std::list<std::string>::const_iterator it;
        rina::RegisterApplicationResponseEvent *resp;
	rina::IPCEvent *event;

	//Check if there are available DIFs
	if(nmsDIFs.empty()){
		LOG_ERR("No DIFs to register to. Aborting...");
		throw eNoNMDIF("No DIFs to register to");
	}

	LOG_DBG("Will register as %s, to DIF: %s", info.processName.c_str(),
						(*nmsDIFs.begin()).c_str());
	//Register to DIF0 (FIXME)
	ari.ipcProcessId = 0;  //TODO: remove!
	ari.appName = info;
	ari.applicationRegistrationType =
	rina::APPLICATION_REGISTRATION_SINGLE_DIF;
	ari.difName = rina::ApplicationProcessNamingInformation(
							*nmsDIFs.begin(),
							std::string());

	// Request the registration
	seqnum = rina::ipcManager->requestApplicationRegistration(ari);

	// Wait for the response to come
	while(1){
		event = rina::ipcEventProducer->eventWait();
		if (event && event->eventType ==
			rina::REGISTER_APPLICATION_RESPONSE_EVENT &&
			event->sequenceNumber == seqnum)
			break;
	}

	resp = dynamic_cast<rina::RegisterApplicationResponseEvent*>(event);

	// Update librina state
	if (resp->result == 0){
		rina::ipcManager->commitPendingRegistration(seqnum,
								resp->DIFName);
	}else{
		rina::ipcManager->withdrawPendingRegistration(seqnum);
		LOG_ERR("FATAL ERROR: unable to register to DIF '%s'",
					ari.difName.processName.c_str());
		throw rina::ApplicationRegistrationException("Failed to register application");
	}

	/*
	//TODO: this is for when librina supports multiple SINGLE DIF regs
	//and the config is properly read
	for(it=nmsDIFs.begin(); it!=nmsDIFs.end(); ++it) {
		//Nice trace
		LOG_INFO("Registering agent at DIF '%s'", it->c_str());

		//TODO FIXME XXX: call ipcmanager to register MA to this DIF
	}
	*/
#endif
}

void ManagementAgent::connect(void){

	unsigned int w_id;
	std::list<AppConnection>::iterator it;

	for(it=connections.begin(); it!=connections.end(); ++it) {
		//Nice trace
		LOG_INFO("Connecting to manager application name '%s'",
			it->flow_info.remoteAppName.processName.c_str());

		//Set our application name information
		it->flow_info.localAppName = info;

		//Instruct flow manager to bootstrap an active connection
		//worker
		w_id = flow_manager->connectTo(*it);

		//TODO: store this
	}
}

//
// Public methods
//

//Add NMS DIF
void ManagementAgent::addNMSDIF(std::string& difName){
	nmsDIFs.push_back(difName);
}

//Add Manager connection
void ManagementAgent::addManagerConnection(AppConnection& con){
	connections.push_back(con);
}

//Process event
void ManagementAgent::process_librina_event(rina::IPCEvent** event){
	flow_manager->process_librina_event(event);
}

void ManagementAgent::process_ipcm_event(const IPCMEvent& event){

	std::string ipcp_name;

	if(event.ipcp_id != -1)
		ipcp_name = IPCManager->get_ipcp_name(event.ipcp_id);

	switch(event.type){
		//Addon related events
		case IPCM_ADDON_LOADED:
			LOG_DBG("The addon '%s' has been loaded",
							event.addon.c_str());
				break;

		//General events
		case IPCM_IPCP_CREATED:
				LOG_DBG("The IPCP '%s'(%d) has been created",
							ipcp_name.c_str(),
							event.ipcp_id);
				rib_factory->createIPCPevent(event.ipcp_id);
				break;
		case IPCM_IPCP_CRASHED:
				LOG_DBG("IPCP '%s'(%d) has CRASHED!",
							ipcp_name.c_str(),
							event.ipcp_id);
				rib_factory->destroyIPCPevent(event.ipcp_id);
				break;
		case IPCM_IPCP_TO_BE_DESTROYED:
				LOG_DBG("IPCP '%s'(%d) is about to be destroyed...",
							ipcp_name.c_str(),
							event.ipcp_id);
				rib_factory->destroyIPCPevent(event.ipcp_id);
				break;
		case IPCM_IPCP_UPDATED:
				LOG_DBG("The configuration of the IPCP '%s'(%d) has been updated",
							ipcp_name.c_str(),
							event.ipcp_id);
				break;
	}

}

//Initialization and destruction routines
ManagementAgent::ManagementAgent(const rinad::RINAConfiguration& config) :
							AppAddon(MAD_NAME){

	//Set ourselves as the instance
	assert(inst == NULL);
	inst = this;

	//Nice trace
	LOG_INFO("Initializing components...");

	//ConfManager must be initialized first, to
	//proper configure the logging according to the cli level
	//or the config file
	conf_manager = new ConfManager(config);

	/*
	* Initialize subsystems
	*/
	//Create RIBs
	//TODO charge from configuration
	RIBAEassoc assocs;
	uint64_t v1 = 1;
	std::list<std::string> aes;
	aes.push_back("v1");

	//Add
	assocs[v1] = aes;
	rib_factory = new RIBFactory(assocs);

	//TODO
	//FlowManager
	flow_manager = new FlowManager(this);


	//Background task manager; MUST be the last one
	//Will not return until SIGINT is sent
	bg_task_manager = new BGTaskManager();

	/*
	* Load configuration
	*/
	conf_manager->configure(*this);

	//Bootstrap necessary NMS DIFs and shim-DIFs
	bootstrapNMSDIFs();

	LOG_INFO("Components initialized");

	//Register agent AP into the IPCManager
	reg();

	//Perform connection to the Manager(s)
	connect();
}

ManagementAgent::~ManagementAgent(){
	/*
	* Destroy all subsystems
	*/

	//TODO

	//FlowManager
	delete flow_manager;

	//Bg
	delete bg_task_manager;

	//Conf Manager
	delete conf_manager;

	LOG_INFO("Goodbye!");
}


}; //namespace mad
}; //namespace rinad

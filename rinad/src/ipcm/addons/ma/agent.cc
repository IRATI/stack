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

// std libraries
#include <list>

namespace rinad {
namespace mad {

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
		(void)w_id;
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
void ManagementAgent::process_event(rina::IPCEvent** event){
	flow_manager->process_event(event);
}



RIBFactory* ManagementAgent::get_rib() const
{
  return rib_factory;
}

//Initialization and destruction routines
ManagementAgent::ManagementAgent(const rinad::RINAConfiguration& config) :
							AppAddon(MAD_NAME){

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
	std::list<uint64_t> supported_versions;
	uint64_t v1 = 1;
	supported_versions.push_back(v1);
	rib_factory = new RIBFactory(supported_versions);

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

/*
 * ManagementAgent class
 *
 *    Marc Sune <marc.sune (at) bisdn.de>
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
		worker_handles.push_back(w_id);
		usleep(100000);
	}

	if (key_manager_connection.flow_info.remoteAppName.processName != std::string()) {
		w_id = flow_manager->connectTo(key_manager_connection, true);
		worker_handles.push_back(w_id);
	}
}

//
// Public methods
//

//Add Manager connection
void ManagementAgent::addManagerConnection(AppConnection& con){
	connections.push_back(con);
}

void ManagementAgent::setKeyManagerConnection(AppConnection& con)
{
	key_manager_connection = con;
}

std::string ManagementAgent::console_command(enum console_command command,
			     	     	     std::list<std::string>& args)
{
	std::stringstream ss;
	rina::rib::RIBDaemonProxy * ribd;
	std::list<rina::rib::RIBObjectData> objects;
	rina::rib::rib_handle_t rib_handle;

	switch(command) {
	case LIST_MAD_STATE:
	        ss << "Management Agent name: " << info.getEncodedString() << std::endl;
	        ss << "Management Agent active connections  ( Manager name | via DIF )" << std::endl;
	        for (std::list<AppConnection>::iterator it = connections.begin();
	        		it != connections.end(); ++it) {
	        	ss << "        ";
	        	ss << it->flow_info.remoteAppName.getEncodedString() << " | ";
	        	ss << it->flow_info.difName.processName << std::endl;
	        }
	        ss << std::endl;
	        return ss.str();
	case QUERY_MAD_RIB:
		ribd = rib_factory->getProxy();
		rib_handle = rib_factory->getRIBHandle(1, "v1");
		objects = ribd->get_rib_objects_data(rib_handle);
		for(std::list<rina::rib::RIBObjectData>::iterator it = objects.begin();
				it != objects.end(); ++it) {
			ss << "Name: " << it->name_ << "; Class: " << it->class_ << "; Instance "
			   << it->instance_ << std::endl << "Value: " << it->displayable_value_ << std::endl;
			ss << std::endl;
		}
		return ss.str();
	default:
		return "";
	}
}

//Process event, do nothing
void ManagementAgent::process_librina_event(rina::IPCEvent** event)
{
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

	LOG_INFO("Components initialized");

	//Perform connection to the Manager(s) and local
	//key Management Agent
	connect();
}

ManagementAgent::~ManagementAgent()
{
	/*
	* Destroy all subsystems in a separate thread
	* Otherwise there may be a deadlock
	*/

	MATerminationThread * mat = new MATerminationThread(flow_manager,
			bg_task_manager, conf_manager, rib_factory);
	mat->start();

	LOG_INFO("Goodbye!");
}

MATerminationThread::MATerminationThread(FlowManager * fm, BGTaskManager * bg,
					 ConfManager * cm, RIBFactory * rf)
	: rina::SimpleThread(std::string("MATermination"), true)
{
	flowman = fm;
	bgman = bg;
	cman = cm;
	rfac = rf;
}

int MATerminationThread::run()
{
	delete flowman;
	delete bgman;
	delete cman;
	delete rfac;
}

}; //namespace mad
}; //namespace rinad

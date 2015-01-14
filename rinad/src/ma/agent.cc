#include "agent.h"

//Subsystems
#include "bgtm.h"
#include "confm.h"
#include "flowm.h"

#define RINA_PREFIX "mad"
#include <librina/logs.h>

namespace rinad {
namespace mad {

//Singleton instance
Singleton<ManagementAgent_> ManagementAgent;

//Add NMS DIF
void ManagementAgent_::addNMSDIF(std::string& difName){
	nmsDIFs.push_back(difName);
}

//Creates the NMS DIFs required by the MA
void ManagementAgent_::bootstrapNMSDIFs(){
	//TODO FIXME XXX
	std::list<std::string>::const_iterator it;

	for(it=nmsDIFs.begin(); it!=nmsDIFs.end(); ++it) {
		//Nice trace
		LOG_INFO("Bootstraping DIF '%s'", it->c_str());

		//TODO FIXME XXX: call ipcmanager bootstrapping of the DIFs
	}

}
//Registers the application in the IPCManager
void ManagementAgent_::reg(){

	rina::ApplicationRegistrationInformation ari;
	std::list<std::string>::const_iterator it;

	//Check if there are available DIFs
	if(nmsDIFs.empty()){
		LOG_ERR("No DIFs to register to. Aborting...");
		throw eNoNMDIF("No DIFs to register to");
	}

	//Register

	for(it=nmsDIFs.begin(); it!=nmsDIFs.end(); ++it) {
		//Nice trace
		LOG_INFO("Registering agent at DIF '%s'", it->c_str());

		//TODO FIXME XXX: call ipcmanager to register MA to this DIF
	}
}

//Initialization and destruction routines
void ManagementAgent_::init(const std::string& conf,
					const std::string& cl_logfile,
					const std::string& cl_loglevel){

	//Nice trace
	LOG_INFO("Initializing components...");

	//ConfManager must be initialized first, to
	//proper configure the logging according to the cli level
	//or the config file
	ConfManager->init(conf, cl_logfile, cl_loglevel);

	/*
	* Initialize subsystems
	*/

	//Create RIBs
	//RIBFactory->createRIB(version1);

	//TODO
	//FlowManager
	FlowManager->init();


	//Background task manager; MUST be the last one
	//Will not return until SIGINT is sent
	BGTaskManager->init();

	/*
	* Load configuration
	*/
	ConfManager->configure();

	//Bootstrap necessary NMS DIFs and shim-DIFs
	bootstrapNMSDIFs();

	//Register agent AP into the IPCManager
	reg();

	LOG_INFO("Components initialized");

	/*
	* Run the bg task manager loop in the main thread
	*/
	FlowManager->runIOLoop();
}

void ManagementAgent_::destroy(){
	/*
	* Destroy all subsystems
	*/

	//TODO

	//FlowManager
	FlowManager->destroy();

	//Bg
	BGTaskManager->destroy();

	//Conf Manager
	ConfManager->destroy();

	LOG_INFO("Goodbye!");
}


//Constructors destructors (singleton only)
ManagementAgent_::ManagementAgent_(){
}

ManagementAgent_::~ManagementAgent_(void){
}

}; //namespace mad
}; //namespace rinad

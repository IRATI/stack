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

//Initialization and destruction routines
void ManagementAgent_::init(const std::string& conf,
					const std::string& cl_logfile,
					const std::string& cl_loglevel){
	//ConfManager must be initialized first, to
	//proper configure the logging according to the cli level
	//or the config file
	ConfManager->init(conf, cl_logfile, cl_loglevel);

	//Nice trace
	LOG_INFO(" Initializing...");

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

	/*
	* Run the bg task manager loop in the main thread
	*/
	BGTaskManager->run(NULL);

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

	LOG_INFO(" Goodbye!");
}


//Constructors destructors (singleton only)
ManagementAgent_::ManagementAgent_(){
}

ManagementAgent_::~ManagementAgent_(void){
}

}; //namespace mad
}; //namespace rinad

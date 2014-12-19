#include "agent.h"

//Subsystems
#include "bgtm.h"

#define RINA_PREFIX "mad"
#include <librina/logs.h>

namespace rinad {
namespace mad {

//Static initializations
ManagementAgent* ManagementAgent::inst = NULL;

void ManagementAgent::init(const std::string& logfile, 
						const std::string& loglevel){
	if(inst){
		throw Exception(
			"Invalid double call to ManagementAgent::init()");	
	}
	inst = new ManagementAgent(logfile, loglevel);
}

void ManagementAgent::destroy(void){
	//TODO: be gentle with on going events
	delete ManagementAgent::inst;
}

//Constructors destructors
ManagementAgent::ManagementAgent(const std::string& logfile, 
						const std::string& loglevel){
	(void)logfile;
	(void)loglevel;
	
	/*
	* Initialize subsystems
	*/

	//TODO
	
	//Background task manager; MUST be the last one
	//Will not return until SIGINT is sent
	BGTaskManager::init();	
}

ManagementAgent::~ManagementAgent(void){

	/*
	* Destroy all subsystems
	*/

	//TODO	
	
	//Bg
	BGTaskManager::destroy();
}

}; //namespace mad 
}; //namespace rinad

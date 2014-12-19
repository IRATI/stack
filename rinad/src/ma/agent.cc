#include "agent.h"

//Subsystems
#include "bgtm.h"

#define RINA_PREFIX "mad"
#include <librina/logs.h>

namespace rinad {
namespace mad {

//Static initializations
ManagementAgent* ManagementAgent::inst = NULL;

void ManagementAgent::init(const std::string conf, const std::string& logfile,
						const std::string& loglevel){
	if(inst){
		throw Exception(
			"Invalid double call to ManagementAgent::init()");	
	}
	inst = new ManagementAgent(conf, logfile, loglevel);
}

void ManagementAgent::destroy(void){
	//TODO: be gentle with on going events
	delete ManagementAgent::inst;
}

//Constructors destructors
ManagementAgent::ManagementAgent(const std::string& conf,
					const std::string& cl_logfile,
					const std::string& cl_loglevel){
	

	//ConfManager must be initialized first, to
	//proper configure the logging according to the cli level
	//or the config file
	ConfManager::init(conf_file, cl_logfile, cl_loglevel);

	//Nice trace
	LOG_INFO(" Initializing...");

	/*
	* Initialize subsystems
	*/

	//TODO

	//Background task manager; MUST be the last one
	//Will not return until SIGINT is sent
	BGTaskManager::init();

	/*
	* Load configuration
	*/
	ConfManager::get()->configure();

	/*
	* Run the bg task manager loop in the main thread
	*/
	BGTaskManager::get()->run(NULL);
}

ManagementAgent::~ManagementAgent(void){

	/*
	* Destroy all subsystems
	*/

	//TODO	

	//Bg
	BGTaskManager::destroy();

	LOG_INFO(" Goodbye!");
}

}; //namespace mad 
}; //namespace rinad

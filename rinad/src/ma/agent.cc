#include "agent.h"

namespace rinad {
namespace mad {

//Static initializations
ManagementAgent* ManagementAgent::inst = NULL;

void ManagementAgent::init(const std::string& logfile, 
						const std::string& loglevel){
	if(ManagementAgent::inst){
		throw Exception(
			"Invalid double call to ManagementAgent::init()");	
	}
	ManagementAgent::inst = new ManagementAgent(logfile, loglevel);
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
}

ManagementAgent::~ManagementAgent(void){
	
}

}; //namespace mad 
}; //namespace rinad

#include "confm.h"
#include "agent.h"

#define RINA_PREFIX "mad.conf"
#include <librina/logs.h>

namespace rinad{
namespace mad{

//Static initializations
ConfManager* ConfManager::inst = NULL;

void ConfManager::init(const std::string& conf, const std::string& logfile,
						const std::string& loglevel){
	if(inst){
		throw Exception(
			"Invalid double call to ConfManager::init()");
	}
	inst = new ConfManager(conf, logfile, loglevel);
}

void ConfManager::destroy(void){
	//TODO
}

//Constructors destructors
ConfManager::ConfManager(const std::string& conf,
					const std::string& cl_logfile,
					const std::string& cl_loglevel){

	(void)conf;
	//TODO: read config and get logging level and file

	setLogLevel(cl_loglevel);
	setLogFile(cl_logfile);
}

void ConfManager::_configure(){

	//Configure the AP nameand instance ID
	//XXX get it from the configuration source (e.g. config file)
	rina::ApplicationProcessNamingInformation info("mad_node1", "1");
	ManagementAgent::setAPInfo(info);

	//TODO
}

}; //namespace mad
}; //namespace rinad



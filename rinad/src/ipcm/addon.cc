#include "addon.h"


#define RINA_PREFIX "ipcm.addon_factory"
#include <librina/logs.h>

//Addons
#include "addons/console.h"
#include "addons/scripting.h"
#include "addons/ma/agent.h"
//[+] Add more here...

namespace rinad {

/**
* Factory
*/
Addon* Addon::factory(const std::string& name, const std::string& params){

	Addon* addon;

	try{
		//TODO this is a transitory solution. A proper auto-registering
		// to the factory would be the right way to go
		if(name == "mad"){
			addon = new mad::ManagementAgent(params);
			return addon;
		}else{
			//TODO add other types
			assert(0);
			LOG_ERR("Uknown addon name '%s'. Ignoring...", name.c_str());
		}
	}catch(...){
		LOG_CRIT("Unable to bootstrap addon '%s'", name.c_str());
	}
	return NULL;
}


} //namespace rinad

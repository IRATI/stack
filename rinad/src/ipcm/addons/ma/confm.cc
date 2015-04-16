#include "confm.h"
#include "agent.h"

#define RINA_PREFIX "ipcm.mad.conf"
#include <librina/logs.h>

namespace rinad{
namespace mad{

//Singleton instance
Singleton<ConfManager> ConfManager;

//Public constructor destructor
ConfManager::ConfManager(const std::string& params){

	(void)params;
	//TODO: read config and get logging level and file

	LOG_DBG("Initialized");
}

ConfManager::~ConfManager(){
	//TODO
}

//Module configuration routine
void ConfManager::configure(ManagementAgent& agent){

	//Configure the AP nameand instance ID
	//XXX get it from the configuration source (e.g. config file)
	//Configure the MA
	rina::ApplicationProcessNamingInformation info("rina.apps.mad", "1");
	agent.setAPInfo(info);


	//NMS
	std::string dif("normal.DIF");
	agent.addNMSDIF(dif);


	//Configure Manager connections
	AppConnection ap_con;
	ap_con.flow_info.remoteAppName =
		rina::ApplicationProcessNamingInformation("rina.apps.manager", "1");
	ap_con.flow_info.difName =
		rina::ApplicationProcessNamingInformation(dif, "");

	agent.addManagerConnection(ap_con);
	//TODO
}

}; //namespace mad
}
;
//namespace rinad

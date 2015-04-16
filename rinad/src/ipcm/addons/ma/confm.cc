#include "confm.h"
#include "agent.h"

#define RINA_PREFIX "ipcm.mad.conf"
#include <librina/logs.h>
#include <debug.h>

#include <fstream>
#include <sstream>

#include "json/json.h"

namespace rinad{
namespace mad{

//Singleton instance
Singleton<ConfManager> ConfManager;

//Public constructor destructor
ConfManager::ConfManager(const std::string& conf_file){

	//TODO: read config and get logging level and file

	Json::Reader reader;
	Json::Value  root;
	std::ifstream     fin;

	fin.open(conf_file.c_str());
	if (fin.fail()) {
		LOG_ERR("Failed to open config file");
		return;
	}

	if (!reader.parse(fin, root, false)) {
		std::stringstream ss;

		ss << "Failed to parse JSON configuration" << std::endl
			<< reader.getFormatedErrorMessages() << std::endl;
		FLUSH_LOG(ERR, ss);

		return;
	}

	fin.close();

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

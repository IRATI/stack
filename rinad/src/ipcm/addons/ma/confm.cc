#include "confm.h"
#include "agent.h"

#define RINA_PREFIX "ipcm.mad.conf"
#include <librina/logs.h>
#include <librina/common.h>
#include <librina/security-manager.h>
#include <debug.h>

#include <librina/json/json.h>

#include <fstream>
#include <sstream>

namespace rinad{
namespace mad{

//Singleton instance
Singleton<ConfManager> ConfManager;

//Public constructor destructor
ConfManager::ConfManager(const rinad::RINAConfiguration& config){

	Json::Reader    reader;
	Json::Value     root;
        Json::Value     mad_conf;
	std::ifstream   fin;

	fin.open(config.configuration_file.c_str());
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

        // Set default values for MAD configuration variables
        app_name = config.local.system_name;
        app_name.processInstance = "1";

        // Parse MAD configuration from JSON, overwriting default configuration
        // variables
        mad_conf = root["addons"]["mad"];
        if (mad_conf != 0) {
                Json::Value mad_conns_conf = mad_conf["managerConnections"];
                Json::Value key_mgr_conf = mad_conf["keyMgrConnection"];
                std::string app_name_enc;
                std::string dif;
                std::string auth_pol_name;

                if (mad_conns_conf != 0) {
                        for (unsigned i = 0; i< mad_conns_conf.size(); i++) {
                                ManagerConnInfo mci;
                                std::string ma_name;

                                ma_name = mad_conns_conf[i]
					  .get("managerAppName",
                                               ma_name).asString();
                                dif = mad_conns_conf[i].get("DIF", dif)
                                                       .asString();
                                auth_pol_name = mad_conns_conf[i].get("authPolicy", auth_pol_name)
                                                                 .asString();
                                if (auth_pol_name == std::string())
                                	auth_pol_name = rina::IAuthPolicySet::AUTH_NONE;

                                mci.manager_name = rina::decode_apnameinfo(ma_name);
                                mci.manager_dif = dif;
                                mci.auth_policy_name = auth_pol_name;
                                manager_connections.push_back(mci);
                        }
                }

                if (key_mgr_conf != 0) {
                	std::string kma_name;
                	kma_name = key_mgr_conf.get("kmaAppName",
                				    kma_name).asString();
                	dif = key_mgr_conf.get("DIF", dif).asString();
                        auth_pol_name = key_mgr_conf.get("authPolicy", auth_pol_name)
                                                         .asString();
                        if (auth_pol_name == std::string())
                        	auth_pol_name = rina::IAuthPolicySet::AUTH_NONE;

                        key_manager_connection.manager_name = rina::decode_apnameinfo(kma_name);
                        key_manager_connection.manager_dif = dif;
                        key_manager_connection.auth_policy_name = auth_pol_name;
                }
        }

        LOG_INFO("MAD application name will be %s",
                 app_name.toString().c_str());

        for (std::list<ManagerConnInfo>::iterator
                        it = manager_connections.begin();
                                it != manager_connections.end(); it++) {
                LOG_INFO("MAD Manager connection Name=%s DIF=%s authPolicy=%s",
                          it->manager_name.toString().c_str(),
                          it->manager_dif.c_str(),
			  it->auth_policy_name.c_str());
        }

        if (key_manager_connection.manager_name.processName != std::string()) {
        	LOG_INFO("MAD Local Key Management Agent connection Name=%s DIF=%s authPolicy=%s",
        		 key_manager_connection.manager_name.processName.c_str(),
			 key_manager_connection.manager_dif.c_str(),
			 key_manager_connection.auth_policy_name.c_str());
        }

	LOG_DBG("Initialized");
}

ConfManager::~ConfManager(){
}

//Module configuration routine
void ConfManager::configure(ManagementAgent& agent)
{
	AppConnection ap_con;

	//Configure the AP name and instance ID
	agent.setAPInfo(app_name);

	//Configure Manager connections
        for (std::list<ManagerConnInfo>::iterator
                        it = manager_connections.begin();
                                it != manager_connections.end(); it++) {

                ap_con.flow_info.remoteAppName = it->manager_name;
                ap_con.flow_info.difName =
                        rina::ApplicationProcessNamingInformation(
                                        it->manager_dif, std::string());
                ap_con.auth_policy_name = it->auth_policy_name;
	        agent.addManagerConnection(ap_con);
        }

        if (key_manager_connection.manager_name.processName == std::string())
        	return;

        ap_con.flow_info.remoteAppName = key_manager_connection.manager_name;
        ap_con.flow_info.difName = rina::ApplicationProcessNamingInformation(key_manager_connection.manager_dif,
        								     std::string());
        ap_con.auth_policy_name = key_manager_connection.auth_policy_name;
	agent.setKeyManagerConnection(ap_con);
}

}; //namespace mad
}
;
//namespace rinad

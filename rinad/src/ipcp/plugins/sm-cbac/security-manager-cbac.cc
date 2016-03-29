#define IPCP_MODULE "security-manager-ps-cbac"
#include "../../ipcp-logging.h"

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <list>
#include "ipcp/components.h"
#include <librina/json/json.h>

using namespace std;
namespace rinad {

//----------------------------------

/* The profile of a RIB */
typedef struct RIBProfile {
	std::string rib_type;
	std::string rib_group;
} RIBProfile_t;

/* The profile of an IPC Process */
typedef struct IPCPProfile {

        rina::ApplicationProcessNamingInformation ipcp_name;
        rina::ApplicationProcessNamingInformation ipcp_difName;
	std::string ipcp_type;
	std::string ipcp_group;
	RIBProfile_t ipcp_rib_profile;
	//DIFProfile ipcp_dif_profile;
} IPCPProfile_t;


  /* The profile of a DIF */
typedef struct DIFProfile {
        rina::ApplicationProcessNamingInformation dif_name;
	std::string dif_type; //FIXME: merge/replace (?) with dif_type_ in DIFInformation
	std::string dif_group;
} DIFProfile_t;


//----------------------------------

class ProfileParser {
public:
        ProfileParser() {};
        bool parseProfile(const std::string fileName);
        virtual ~ProfileParser() {};
        bool getDIFProfileByName(const rina::ApplicationProcessNamingInformation&, DIFProfile_t&);
        bool getIPCPProfileByName(const rina::ApplicationProcessNamingInformation&, IPCPProfile_t&);
  
private:
  
        std::list<DIFProfile_t> difProfileList;
        std::list<IPCPProfile_t> ipcpProfileList;
        std::list<RIBProfile_t> ribProfileList;
};

/**
 * Parse AC config file with jsoncpp
 * **/
bool ProfileParser::parseProfile(const std::string fileName)
{
	
	Json::Value  root;
	Json::Reader reader;
	ifstream     file;

	file.open(fileName.c_str(), std::ifstream::in);
	if (file.fail()) {
		LOG_ERR("Failed to open AC profile file");
		return false;
	}

	if (!reader.parse(file, root, false)) {
		LOG_ERR("Failed to parse configuration");

		// FIXME: Log messages need to take string for this to work
		cout << "Failed to parse JSON" << endl
			  << reader.getFormatedErrorMessages() << endl;

		return false;
	}

	file.close();

	// really parse
	
	Json::Value dif_profiles = root["DIFProfiles"];
	for (unsigned int i = 0; i < dif_profiles.size(); i++) {
		DIFProfile_t dif;
		dif.dif_name = rina::ApplicationProcessNamingInformation(
                        dif_profiles[i].get("difName",    string()).asString(), string());
		dif.dif_type = dif_profiles[i].get("difType",    string()).asString();
		dif.dif_group = dif_profiles[i].get("difGroup",    string()).asString();
		difProfileList.push_back(dif);
	}
	
	
	Json::Value ipcp_profiles = root["IPCPProfiles"];
	for (unsigned int i = 0; i < ipcp_profiles.size(); i++) {
		IPCPProfile_t ipcp;
		ipcp.ipcp_name = rina::ApplicationProcessNamingInformation(ipcp_profiles[i].get("apName", 
                                                          string()).asString(), string());
		ipcp.ipcp_type = ipcp_profiles[i].get("ipcpType",    string()).asString();
		ipcp.ipcp_group = ipcp_profiles[i].get("ipcpGroup",    string()).asString();
		
		Json::Value rib_profile = ipcp_profiles[i]["ipcpRibProfile"];
		if (rib_profile != 0){
			//parse rib profile of current ipcp
			RIBProfile_t ribProfile;
			ribProfile.rib_group = rib_profile.get("ribGroup", string()).asString();
			ribProfile.rib_type = rib_profile.get("ribType", string()).asString();
			ipcp.ipcp_rib_profile = ribProfile;
			ribProfileList.push_back(ribProfile);
		}
		ipcpProfileList.push_back(ipcp);
				
	}
	return true;
	
}

bool ProfileParser::getDIFProfileByName(const rina::ApplicationProcessNamingInformation& difName,
					DIFProfile_t& result)
{
	for (list<DIFProfile_t>::const_iterator it = difProfileList.begin();
					it != difProfileList.end(); it++) {
		if (it->dif_name == difName) {
			result = *it;
			return true;
		}
	}

	return false;
}

bool ProfileParser::getIPCPProfileByName(const rina::ApplicationProcessNamingInformation& ipcpName,
					IPCPProfile_t&  result)
{
	for (list<IPCPProfile_t>::const_iterator it = ipcpProfileList.begin();
					it != ipcpProfileList.end(); it++) {
		if (it->ipcp_name == ipcpName) {
			result = *it;
			return true;
		}
	}

	return false;
}
//-----------------------------------

typedef enum{
	/// The enrollement AC was successful
	AC_ENR_SUCCESS = 0,

	/* List of errors */

	/// AC enrollement error
	AC_ENR_ERROR = -1,
	/// Group of IPCP and DIF are different
	//AC_ENR_DIFF_GROUP = -2,
	
} ac_code_t;

typedef struct ac_result_info {
	/// Result code of the operation
	ac_code_t code_;

	/// Result-Reason (string), optional in the responses, forbidden in the requests
	/// Additional explanation of the result_
	std::string reason_;

	ac_result_info() {};
} ac_res_info_t;

//-----------------------------------
class AccessControl{
public:
	AccessControl();
	bool checkJoinDIF(DIFProfile_t&, IPCPProfile_t&, ac_res_info_t&);
	void generateToken();
	virtual ~AccessControl() {}
	static const std::string IPCP_DIF_FROM_DIFFERENT_GROUPS;
};

AccessControl::AccessControl()
{
	
}

/** an example of AC check at the enrollment phase **/
bool AccessControl::checkJoinDIF(DIFProfile_t& difProfile, IPCPProfile_t& newMemberProfile, 
                                 ac_res_info_t& result)
{
	if ( newMemberProfile.ipcp_group == "PRIMARY_IPCP" ){
		result.code_ = AC_ENR_SUCCESS;
		return true;
	}
		
	if ( difProfile.dif_group != newMemberProfile.ipcp_group ){
		result.code_ = AC_ENR_ERROR;
		result.reason_ = IPCP_DIF_FROM_DIFFERENT_GROUPS;
		return false;
	}
	return true;
}

void AccessControl::generateToken()
{
  
}

//-----------------------------------
class SecurityManagerCBACPs: public ISecurityManagerPs {
public:
	SecurityManagerCBACPs(IPCPSecurityManager * dm);
	bool isAllowedToJoinDIF(const rina::Neighbor& newMember); 
                                //const rina::ApplicationProcessNamingInformation, std::string);
	bool acceptFlow(const configs::Flow& newFlow);
	int set_policy_set_param(const std::string& name,
			const std::string& value);
	virtual ~SecurityManagerCBACPs() {}

private:
	// Data model of the security manager component.
	IPCPSecurityManager * dm;
	int max_retries;
	AccessControl * access_control_;
};

SecurityManagerCBACPs::SecurityManagerCBACPs(IPCPSecurityManager * dm_)
						: dm(dm_)
{
	access_control_ = new AccessControl();
}


bool SecurityManagerCBACPs::isAllowedToJoinDIF(const rina::Neighbor& newMember)
{
    
        const rina::ApplicationProcessNamingInformation difName = 
                        dm->ipcp->get_dif_information().dif_name_;
        const std::string   profileFile = dm->ipcp->get_dif_information().
                    get_dif_configuration().sm_configuration_.policy_set_.
                    get_param_value_as_string("ACprofilestore");
	
        // Read Profile From AC Profile Store
	ProfileParser profileParser;
        
        if (!profileParser.parseProfile(profileFile)){
                LOG_IPCP_DBG("Error Parsing Profile file");
                return false;
        }

	IPCPProfile_t newMemberProfile;
	
	
	if (!profileParser.getIPCPProfileByName(newMember.name_, newMemberProfile)){
		LOG_IPCP_DBG("No Profile for this newMember %s; not allowing it to join DIF!" ,
		       newMember.name_.processName.c_str());
		return false;
	}
	

	DIFProfile_t difProfile; 
	if (!profileParser.getDIFProfileByName(difName, difProfile)){
		LOG_IPCP_DBG("No Profile for my DIF, not allowing IPCProcess %s to join DIF!",
				newMember.name_.processName.c_str());
		return false;
	}
	
	// Enrollment AC algorithm
	ac_res_info_t res;
	access_control_->checkJoinDIF(difProfile, newMemberProfile, res);
	if (res.code_ == 0){
		LOG_IPCP_DBG("Allowing IPC Process %s to join the DIF",
		     newMember.name_.processName.c_str());
		return true;
	}
	if (res.code_ != 0){
		LOG_IPCP_DBG("NOT Allowing IPC Process %s to join the DIF because of %s",
		     newMember.name_.processName.c_str(), res.reason_.c_str());
		return false;
	}
	
}

bool SecurityManagerCBACPs::acceptFlow(const configs::Flow& newFlow)
{
	LOG_IPCP_DBG("Accepting flow from remote application %s",
			newFlow.source_naming_info.getEncodedString().c_str());
	return true;
}

int SecurityManagerCBACPs::set_policy_set_param(const std::string& name,
						  const std::string& value)
{
	if (name == "max_retries") {
		int x;
		std::stringstream ss;

		ss << value;
		ss >> x;
		if (ss.fail()) {
			LOG_IPCP_ERR("Invalid value '%s'", value.c_str());
			return -1;
		}

		max_retries = x;
	} else {
		LOG_IPCP_ERR("Unknown parameter '%s'", name.c_str());
		return -1;
	}

	return 0;
}

}   // namespace rinad

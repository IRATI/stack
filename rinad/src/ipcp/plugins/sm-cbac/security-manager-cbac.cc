#define IPCP_MODULE "security-manager-ps-cbac"
#include "../../ipcp-logging.h"

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <list>
#include "ipcp/components.h"
#include <librina/json/json.h>
#include "security-manager-cbac.h"
//using namespace std;
namespace rinad {

/**
 * class ProfileParser: Parse AC config file with jsoncpp
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
/*** 
 * clacc AccessControl: effective AC algorithm
 * **/

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

std::list<Capability_t>& AccessControl::computeCapabilities(DIFProfile_t& difProfile, 
                                                            IPCPProfile_t& newMemberProfile)
{
        
        std::list<Capability_t> result;
        if ( newMemberProfile.ipcp_group == "PRIMARY_IPCP" || 
                    difProfile.dif_group == newMemberProfile.ipcp_group ){
                Capability_t cap = Capability_t("all", "all");
                result.push_back(cap); 
        }
        
        if (newMemberProfile.ipcp_type == "BORDER_ROUTER"){
                Capability_t cap = Capability_t("PDU_FWD_TABLE", "RW");
                result.push_back(cap); 
                cap = Capability_t("IPCP_ADDR_TABLE", "R");
                result.push_back(cap); 
            
        }
        return result;
    
}


void AccessControl::generateToken(unsigned short issuerIpcpId, DIFProfile_t& difProfile,
                                  IPCPProfile_t& newMemberProfile, Token_t& token)
{
        std::list<Capability_t> result = computeCapabilities(difProfile, newMemberProfile); 
        
        token.token_id = issuerIpcpId; // TODO name or id?
        token.ipcp_issuer_id = issuerIpcpId;
        token.ipcp_holder_name = newMemberProfile.ipcp_name; //TODO name or id?
        token.audience = "all";
        rina::Time currentTime;
        token.issued_time = currentTime.get_current_time_in_ms();
        token.token_nbf = currentTime.get_current_time_in_ms();
        token.token_exp = currentTime.get_current_time_in_ms() * 10000;
        token.token_cap = result; 
        token.token_sign = "signature";
        
}

//-----------------------------------
/**
 * class SecurityManagerCBACPs
 * **/

SecurityManagerCBACPs::SecurityManagerCBACPs(IPCPSecurityManager * dm_)
						: dm(dm_)
{
	access_control_ = new AccessControl();
        my_ipcp_id = dm->ipcp->get_id();
        my_dif_name = dm->ipcp->get_dif_information().dif_name_;
}


bool SecurityManagerCBACPs::isAllowedToJoinDIF(const rina::Neighbor& newMember)
{
    
       
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
	if (!profileParser.getDIFProfileByName(my_dif_name, difProfile)){
		LOG_IPCP_DBG("No Profile for my DIF, not allowing IPCProcess %s to join DIF!",
				newMember.name_.processName.c_str());
		return false;
	}
	
	// Enrollment AC algorithm
	ac_res_info_t res;
	access_control_->checkJoinDIF(difProfile, newMemberProfile, res);
	if (res.code_ == 0){
		LOG_IPCP_DBG("Allowing IPC Process %s to join the DIF. Going to generate token",
		     newMember.name_.processName.c_str());
                
                //access_control_->generateToken(my_ipcp_id, difProfile, newMemberProfile);
		return true;
	}
	if (res.code_ != 0){
		LOG_IPCP_DBG("NOT Allowing IPC Process %s to join the DIF because of %s",
		     newMember.name_.processName.c_str(), res.reason_.c_str());
		return false;
	}
	
}

bool SecurityManagerCBACPs::generateToken(const rina::Neighbor& newMember, Token_t& token){
    
        ProfileParser profileParser;
        
        DIFProfile_t difProfile; 
        if (!profileParser.getDIFProfileByName(my_dif_name, difProfile)){
                LOG_IPCP_DBG("No Profile for my DIF, not allowing IPCProcess %s to join DIF!",
                                newMember.name_.processName.c_str());
                return false;
        }
        
        IPCPProfile_t newMemberProfile;
        
        if (!profileParser.getIPCPProfileByName(newMember.name_, newMemberProfile)){
                LOG_IPCP_DBG("No Profile for this newMember %s; not allowing it to join DIF!" ,
                       newMember.name_.processName.c_str());
                return false;
        }
        
        access_control_->generateToken(my_ipcp_id, difProfile, newMemberProfile, token);
        return true;
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

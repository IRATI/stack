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
#include "sm-cbac.pb.h"
//using namespace std;
namespace rinad {
const std::string AC_CBAC = "AC_CBAC";
const std::string AC_CBAC_VERSION = "1";
const std::string AccessControl::IPCP_DIF_FROM_DIFFERENT_GROUPS = "IPCP_DIF_FROM_DIFFERENT_GROUPS";
const int VALIDITY_TIME_IN_HOURS = 2;
//----------------------------
//FIXME: merge/use the helpers in rinad/src/common/encoder.cc
    
namespace cbac_helpers {

void get_NewApplicationProcessNamingInfo_t(
                const rina::ApplicationProcessNamingInformation &name,
                rina::messages::newApplicationProcessNamingInfo_t &gpb)
{
        gpb.set_applicationprocessname(name.processName);
        gpb.set_applicationprocessinstance(name.processInstance);
        gpb.set_applicationentityname(name.entityName);
        gpb.set_applicationentityinstance(name.entityInstance);
}

rina::messages::newApplicationProcessNamingInfo_t* get_NewApplicationProcessNamingInfo_t(
                const rina::ApplicationProcessNamingInformation &name)
{
        rina::messages::newApplicationProcessNamingInfo_t *gpb =
                        new rina::messages::newApplicationProcessNamingInfo_t;
        get_NewApplicationProcessNamingInfo_t(name, *gpb);
        return gpb;
}

void get_NewApplicationProcessNamingInformation(
                const rina::messages::newApplicationProcessNamingInfo_t &gpf_app,
                rina::ApplicationProcessNamingInformation &app)
{
        app.processName = gpf_app.applicationprocessname();
        app.processInstance = gpf_app.applicationprocessinstance();
        app.entityName = gpf_app.applicationentityname();
        app.entityInstance = gpf_app.applicationentityinstance();
}

void toModelCap(const rina::messages::smCapability_t &gpb_cap, 
                            Capability_t &des_cap)
{
        des_cap.ressource = gpb_cap.ressource();
        des_cap.operation = gpb_cap.operation();
}

void toModelToken(const rina::messages::smCbacToken_t &gpb_token, 
                            Token_t &des_token)
{
    
        des_token.token_id = gpb_token.token_id();
        des_token.ipcp_issuer_id = gpb_token.ipcp_issuer_id();
        
        
        
        rina::ApplicationProcessNamingInformation app_name;
        cbac_helpers::get_NewApplicationProcessNamingInformation(gpb_token.ipcp_holder_name(),
                                                         app_name);
        
        des_token.ipcp_holder_name = app_name;
        
        des_token.audience = gpb_token.audience();
        des_token.issued_time = gpb_token.issued_time();
        des_token.token_nbf = gpb_token.token_nbf();
        des_token.token_exp = gpb_token.token_exp();
        
        
        for (int i = 0; i < gpb_token.token_cap_size(); i++)
        {
                Capability_t cap; // = new Capability_t();
                cbac_helpers::toModelCap(gpb_token.token_cap(i), cap);
                des_token.token_cap.push_back(cap);
        }
        
        des_token.token_sign = gpb_token.token_sign();
}       

} //cbac-helpers

//----------------------------    
   
void serializeToken(const Token_t &token, 
                        rina::ser_obj_t &result)
{

        LOG_IPCP_DBG("Serializing token..");
        rina::messages::smCbacToken_t gpbToken;
        
        //fill in the token message object
        gpbToken.set_token_id(token.token_id);
        gpbToken.set_ipcp_issuer_id(token.ipcp_issuer_id);
        
        gpbToken.set_allocated_ipcp_holder_name(cbac_helpers::get_NewApplicationProcessNamingInfo_t(
                                        token.ipcp_holder_name));
        
        gpbToken.set_audience(token.audience);
        gpbToken.set_issued_time(token.issued_time);
        gpbToken.set_token_nbf(token.token_nbf);
        gpbToken.set_token_exp(token.token_exp);
        
        // Token capability        
        
        std::list<Capability_t> tokenCapList = token.token_cap;
        for(std::list<Capability_t>::iterator it = tokenCapList.begin();
                it != tokenCapList.end(); ++it) {
                
                Capability_t c = *it;
                
                rina::messages::smCapability_t * allocatedCap = gpbToken.add_token_cap();
                allocatedCap->set_ressource(c.ressource);
                allocatedCap->set_operation(c.operation);
        }
    
        gpbToken.set_token_sign(token.token_sign);

        //serializing
        int size = gpbToken.ByteSize();
        result.message_ = new unsigned char[size];
        result.size_ = size;
        gpbToken.SerializeToArray(result.message_, size);
        
}

void deserializeToken(const rina::ser_obj_t &serobj,
                                Token_t &des_token)
{
    
        rina::messages::smCbacToken_t gpb;
        gpb.ParseFromArray(serobj.message_, serobj.size_);
        cbac_helpers::toModelToken(gpb, des_token);
        
}
//---------------------------
string ProfileParser::toString() const
{
        stringstream ss;

        for (list<DIFProfile_t>::const_iterator it =
                        difProfileList.begin();
                                it != difProfileList.end(); it++) {
                ss << "DIF Profile:" << endl;
                ss << "\tName: " << it->dif_name.toString() << endl;
                ss << "\tDIF Type: " << it->dif_type.c_str() << endl;
                ss << "\tDIF Group: " << it->dif_group.c_str() << endl;
                ss << "**" << endl;
        } 
        ss << "********" << endl;
        for (list<IPCPProfile_t>::const_iterator nit =
                        ipcpProfileList.begin();
                                nit != ipcpProfileList.end(); nit++) {
                ss << "\tIPCP Profile:" << endl;
                ss << "\t\tName: " << nit->ipcp_name.toString() << endl;
                ss << "\t\tType: " << nit->ipcp_type.c_str() << endl;
                ss << "\t\tGroup: " << nit->ipcp_group.c_str() << endl;
                //FIXME: not set
                //ss << "\t\tDIF: " << nit->ipcp_difName.name.c_str() << endl;
                ss << "\t\tRIB type: " <<
                        nit->ipcp_rib_profile.rib_type.c_str() << endl;
                ss << "\t\tRIB group: " <<
                        nit->ipcp_rib_profile.rib_group.c_str() << endl;
                ss << "**" << endl;
        }

        return ss.str();
}
//----------------------------
 /**
 * class ProfileParser: Parse AC config file with jsoncpp
 * **/

bool ProfileParser::parseProfile(const std::string fileName)
{
	
	Json::Value  root;
	Json::Reader reader;
	ifstream     file;
        
        LOG_IPCP_DBG("Parsing file %s", fileName.c_str());
        
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
	LOG_IPCP_DBG("Profiles parsed: %s", toString().c_str());
	return true;
	
}

bool ProfileParser::getDIFProfileByName(const rina::ApplicationProcessNamingInformation& difName,
					DIFProfile_t& result)
{
        //FIXME: check comparaison
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
                LOG_IPCP_DBG("Comparing [%s] and [%s]",
                    it->ipcp_name.toString().c_str(), ipcpName.processName.c_str());
		if (it->ipcp_name.processName == ipcpName.processName) {
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
	LOG_IPCP_DBG("Creating AccessControl Class");
}

/** an example of AC check at the enrollment phase **/
bool AccessControl::checkJoinDIF(DIFProfile_t& difProfile, IPCPProfile_t& newMemberProfile, 
                                 ac_res_info_t& result)
{
        LOG_IPCP_DBG("AC: Check to join DIF in progress...");
        
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

std::list<Capability_t> AccessControl::computeCapabilities(DIFProfile_t& difProfile, 
                                                            IPCPProfile_t& newMemberProfile)
{
        
        LOG_IPCP_DBG("AC: Compute capabilities...");
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
                                  IPCPProfile_t& newMemberProfile, rina::cdap_rib::auth_policy_t & auth)
{
        std::list<Capability_t> result = computeCapabilities(difProfile, newMemberProfile); 
        Token_t token;
        
        token.token_id = issuerIpcpId; // TODO name or id?
        token.ipcp_issuer_id = issuerIpcpId;
        token.ipcp_holder_name = newMemberProfile.ipcp_name; //TODO name or id? (.processName)
        token.audience = "all";
        rina::Time currentTime;
        int t = currentTime.get_current_time_in_ms();
        token.issued_time = t;
        token.token_nbf = t; //token valid immediately
        token.token_exp = VALIDITY_TIME_IN_HOURS;; // after this time, the token will be invalid
        token.token_cap = result; 
        token.token_sign = "signature";
        
        // token should be encoded as ser_obj_t
        rina::ser_obj_t options;
        serializeToken(token, options);
        
        // fill auth structure
        
        // cdap_rib::auth_policy_t auth;
        auth.name = AC_CBAC;
        auth.versions.push_back(AC_CBAC_VERSION);
        auth.options = options;
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
        LOG_IPCP_DBG("Creating SecurityManagerCBACPs class: my_ipcp_id %d", my_ipcp_id);
       
}


int SecurityManagerCBACPs::isAllowedToJoinDIF(const rina::Neighbor& newMember,
                                               rina::cdap_rib::auth_policy_t & auth)
{
    
        my_dif_name = dm->ipcp->get_dif_information().dif_name_;
        LOG_IPCP_DBG("SecurityManagerCBACPs class: isAllowedToJoinDIF my_dif_name %s", 
                     my_dif_name.processName.c_str());
        const std::string   profileFile = dm->ipcp->get_dif_information().
                    get_dif_configuration().sm_configuration_.policy_set_.
                    get_param_value_as_string("ACprofilestore");
	
        // Read Profile From AC Profile Store
	ProfileParser profileParser;
        
        if (!profileParser.parseProfile(profileFile)){
                LOG_IPCP_DBG("Error Parsing Profile file");
                return -1;
        }

	IPCPProfile_t newMemberProfile;
	
	LOG_IPCP_DBG("Parsing profile of newMember %s...." ,
                       newMember.name_.processName.c_str());
        
	if (!profileParser.getIPCPProfileByName(newMember.name_, newMemberProfile)){
		LOG_IPCP_DBG("No Profile for this newMember %s; not allowing it to join DIF!" ,
		       newMember.name_.processName.c_str());
		return -1;
	}
	

	DIFProfile_t difProfile; 
        LOG_IPCP_DBG("Parsing profile of DIF  %s...." ,
                       my_dif_name.processName.c_str());
        
	if (!profileParser.getDIFProfileByName(my_dif_name, difProfile)){
		LOG_IPCP_DBG("No Profile for my DIF, not allowing IPCProcess %s to join DIF!",
				newMember.name_.processName.c_str());
		return -1;
	}
	
	// Enrollment AC algorithm
	ac_res_info_t res;
	access_control_->checkJoinDIF(difProfile, newMemberProfile, res);
	if (res.code_ == 0){
		LOG_IPCP_DBG("Allowing IPC Process %s to join the DIF. Going to generate token",
		     newMember.name_.processName.c_str());
                
                access_control_->generateToken(my_ipcp_id, difProfile, newMemberProfile, auth);
		return 0;
	}
	if (res.code_ != 0){
		LOG_IPCP_DBG("NOT Allowing IPC Process %s to join the DIF because of %s",
		     newMember.name_.processName.c_str(), res.reason_.c_str());
		return -1;
	}
	
}

int SecurityManagerCBACPs::storeAccessControlCreds(const rina::cdap_rib::auth_policy_t & auth,
                                               const rina::cdap_rib::con_handle_t & con)
{
        LOG_IPCP_DBG("Storing AC Credentials of IPCP %s (token coming from %s)",
                     con.src_.ap_name_.c_str(), con.dest_.ap_name_.c_str());
        
        Token_t token;
        deserializeToken(auth.options, token);
        
        LOG_IPCP_DBG("Received token : %s", token.toString().c_str());
        token_per_ipcp[con.src_.ap_name_.c_str()] = & token;
        return 0;
}

int SecurityManagerCBACPs::getAccessControlCreds(rina::cdap_rib::auth_policy_t & auth,
                                             const rina::cdap_rib::con_handle_t & con)
{
        (void) auth;
        (void) con;

        return 0;
}

int SecurityManagerCBACPs::checkRIBOperation(const rina::cdap_rib::auth_policy_t & auth,
                                         const rina::cdap_rib::con_handle_t & con,
                                         const rina::cdap::cdap_m_t::Opcode opcode,
                                         const std::string obj_name)
{
        (void) auth;
        (void) con;
        (void) opcode;
        (void) obj_name;

        return 0;
}


#if 0
bool SecurityManagerCBACPs::getToken(const rina::Neighbor& newMember,
                                      rina::ser_obj_t &result)
{
         Token_t token;
         if (!generateToken(newMember, token)){
         
             return false;
         }
         serializeToken(token, result);
         return true;
         
}

bool SecurityManagerCBACPs::generateToken(const rina::Neighbor& newMember, Token_t& token)
{
    
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

#endif
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

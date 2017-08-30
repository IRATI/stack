//
// CBAC policy set for Security Manager
//
//    Ichrak Amdouni <ichrak.amdouni@gmail.com> // IMT-TSP
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA  02110-1301  USA

#define IPCP_MODULE "security-manager-ps-cbac"
#include "../../ipcp-logging.h"

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <list>
#include "ipcp/components.h"
#include <sys/time.h>
#include <librina/json/json.h>
#include "security-manager-cbac.h"
#include "sm-cbac.pb.h"
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/ssl.h>


//using namespace std;
namespace rinad {
const std::string AC_CBAC = "AC_CBAC";
const std::string AC_CBAC_VERSION = "1";
const std::string ABSENT_RPOFILE = "ABSENT_PROFILE";
const std::string ENROLLMENT_NOT_ALLOWED = "ENROLLMENT_NOT_ALLOWED";
const int VALIDITY_TIME_IN_HOURS = 2;
int FIRST_TIME_RCV_TOKEN = 0; // first time the token is received, used to validate times in the token
#define ACCESS_GRANTED          0

//----------------------------
    
namespace cbac_helpers {

std::string getStringParamFromConfig(std::string paramName, IPCPSecurityManager *dm)
{
        std::string result = dm->ipcp->get_dif_information().
                    get_dif_configuration().sm_configuration_.policy_set_.
                    get_param_value_as_string(paramName);
        return result;
}

int getTimeMs()
{
        timeval time_;
        gettimeofday(&time_, 0);
        int time_seconds = (int) time_.tv_sec;
        return (int) time_seconds * 1000 + (int) (time_.tv_usec / 1000);
}

void get_NewApplicationProcessNamingInfo_t(
                const rina::ApplicationProcessNamingInformation &name,
                rina::messages::applicationProcessNamingInfo_t &gpb)
{
        gpb.set_applicationprocessname(name.processName);
        gpb.set_applicationprocessinstance(name.processInstance);
        gpb.set_applicationentityname(name.entityName);
        gpb.set_applicationentityinstance(name.entityInstance);
}

rina::messages::applicationProcessNamingInfo_t* get_NewApplicationProcessNamingInfo_t(
                const rina::ApplicationProcessNamingInformation &name)
{
        rina::messages::applicationProcessNamingInfo_t *gpb =
                        new rina::messages::applicationProcessNamingInfo_t;
        get_NewApplicationProcessNamingInfo_t(name, *gpb);
        return gpb;
}

void get_NewApplicationProcessNamingInformation(
                const rina::messages::applicationProcessNamingInfo_t &gpf_app,
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
        
        rina::ApplicationProcessNamingInformation issuer_name;
        cbac_helpers::get_NewApplicationProcessNamingInformation(gpb_token.ipcp_issuer_name(),
                                                         issuer_name);
        
        des_token.ipcp_issuer_name = issuer_name;
        
        
        for (int i = 0; i < gpb_token.audience_size(); i++){
        
            des_token.audience.push_back(gpb_token.audience(i));
        }
        
        des_token.issued_time = gpb_token.issued_time();
        des_token.token_nbf = gpb_token.token_nbf();
        des_token.token_exp = gpb_token.token_exp();
        
        
        for (int i = 0; i < gpb_token.token_cap_size(); i++)
        {
                Capability_t cap;
                cbac_helpers::toModelCap(gpb_token.token_cap(i), cap);
                des_token.token_cap.push_back(cap);
        }
        
}       

void toModelTokenPlusSignature(const rina::messages::smCbacTokenPlusSign_t &gpb_token_sign, 
                            TokenPlusSignature_t &des_token_sign)
{
        
        Token_t &des_token = des_token_sign.token;
        
        rina::messages::smCbacToken_t gpb_token = gpb_token_sign.token();
        toModelToken(gpb_token, des_token);
                  
        des_token_sign.token_sign.data = new unsigned char[gpb_token_sign.token_sign().size()];
        memcpy(des_token_sign.token_sign.data, gpb_token_sign.token_sign().data(), 
               gpb_token_sign.token_sign().size());
        des_token_sign.token_sign.length = gpb_token_sign.token_sign().size();
}       


int hashToken(rina::UcharArray &input, rina::UcharArray &result, 
              std::string encrypt_alg)
{
        LOG_IPCP_DBG("Start Token hashing, encrypt_alg %s",
                    encrypt_alg.c_str());
        
        if (encrypt_alg == SSL_TXT_AES128) {
                result.length = 16;
                result.data = new unsigned char[16];
                MD5(input.data, input.length, result.data);

        } else if (encrypt_alg == SSL_TXT_AES256) {
                result.length = 32;
                result.data = new unsigned char[32];
                SHA256(input.data, input.length, result.data);
        }

        LOG_IPCP_DBG("Generated hash: input %d bytes [%s] output of length %d bytes [%s]",
                        input.length,
                        input.toString().c_str(),
                        result.length,
                        result.toString().c_str());
        
        return 0;
}

int encryptHashedTokenWithPrivateKey(rina::UcharArray &input, rina::UcharArray &result, 
                                     RSA * my_private_key)
{
        
        
        result.data = new unsigned char[RSA_size(my_private_key)];
        // as we use the private encrypt with RSA_PKCS1_PADDING passing, 
        // we need input to check: len(input) = RSA_size() - 11
        // hashed(token) is either 16 bytes or 32 bytes, so ok        
        assert(input.length < RSA_size(my_private_key) - 11);
        result.length = RSA_private_encrypt(input.length,
                                                   input.data,
                                                   result.data,
                                                   my_private_key,
                                                   RSA_PKCS1_PADDING);

        if (result.length == -1) {
                LOG_ERR("Error encrypting challenge with RSA private key: %s",
                        ERR_error_string(ERR_get_error(), NULL));
                return -1;
        }

        return 0;
}

std::string opcodeToString(rina::cdap::cdap_m_t::Opcode opCode)
{
        std::string result;

        switch(opCode) {
                case rina::cdap::cdap_m_t::M_CONNECT:
                        result = "0_M_CONNECT";
                        break;
                case rina::cdap::cdap_m_t::M_CONNECT_R:
                        result = "1_M_CONNECT_R";
                        break;
                case rina::cdap::cdap_m_t::M_RELEASE:
                        result = "2_M_RELEASE";
                        break;
                case rina::cdap::cdap_m_t::M_RELEASE_R:
                        result = "3_M_RELEASE_R";
                        break;
                case rina::cdap::cdap_m_t::M_CREATE:
                        result = "4_M_CREATE";
                        break;
                case rina::cdap::cdap_m_t::M_CREATE_R:
                        result = "5_M_CREATE_R";
                        break;
                case rina::cdap::cdap_m_t::M_DELETE:
                        result = "6_M_DELETE";
                        break;
                case rina::cdap::cdap_m_t::M_DELETE_R:
                        result = "7_M_DELETE_R";
                        break;
                case rina::cdap::cdap_m_t::M_READ:
                        result = "8_M_READ";
                        break;
                case rina::cdap::cdap_m_t::M_READ_R:
                        result = "9_M_READ_R";
                        break;
                case rina::cdap::cdap_m_t::M_CANCELREAD:
                        result = "10_M_CANCELREAD";
                        break;
                case rina::cdap::cdap_m_t::M_CANCELREAD_R:
                        result = "11_M_CANCELREAD_R";
                        break;
                case rina::cdap::cdap_m_t::M_WRITE:
                        result = "12_M_WRITE";
                        break;
                case rina::cdap::cdap_m_t::M_WRITE_R:
                        result = "13_M_WRITE_R";
                        break;
                case rina::cdap::cdap_m_t::M_START:
                        result = "14_M_START";
                        break;
                case rina::cdap::cdap_m_t::M_START_R:
                        result = "15_M_START_r";
                        break;
                case rina::cdap::cdap_m_t::M_STOP:
                        result = "16_M_STOP";
                        break;
                case rina::cdap::cdap_m_t::M_STOP_R:
                        result = "17_M_STOP_R";
                        break;
                case rina::cdap::cdap_m_t::NONE_OPCODE:
                        result = "18_NON_OPCODE";
                        break;
                default:
                        result = "Wrong operation code";
                }

        return result;
}

std::string operationToString(const rina::cdap_rib::con_handle_t & con,
                                const rina::cdap::cdap_m_t::Opcode opcode,
                                const std::string obj_name)
{
        stringstream ss;
        
        ss << "\n Opcode: " << opcodeToString(opcode).c_str() << endl;
        ss << "Object name: " << obj_name << endl;
        ss << "From: " << con.dest_.ap_name_.c_str() << endl;
        return ss.str();
}


} //cbac-helpers

//----------------------------

void serializeToken(const Token_t &token, 
                        rina::ser_obj_t &result)
{

        rina::messages::smCbacToken_t gpbToken;
        
        // fill in the token message object
        gpbToken.set_token_id(token.token_id);
        gpbToken.set_ipcp_issuer_id(token.ipcp_issuer_id);
        
        gpbToken.set_allocated_ipcp_holder_name(cbac_helpers::get_NewApplicationProcessNamingInfo_t(
                                        token.ipcp_holder_name));
        gpbToken.set_allocated_ipcp_issuer_name(cbac_helpers::get_NewApplicationProcessNamingInfo_t(
                                        token.ipcp_issuer_name));
         
        for (std::list<std::string>::const_iterator it =
                        token.audience.begin();
                        it != token.audience.end(); ++it)
        {
                gpbToken.add_audience(*it);
        }
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
        
        // serializing
        int size = gpbToken.ByteSize();
        result.message_ = new unsigned char[size];
        result.size_ = size;
        gpbToken.SerializeToArray(result.message_, size);
        
}

void serializeTokenPlusSignature(const TokenPlusSignature_t &tokenSign, 
                        rina::ser_obj_t &result)
{

        LOG_IPCP_DBG("Serializing token and signature...");
        
        rina::messages::smCbacTokenPlusSign_t gpbTokenSign;
        
        
        rina::messages::smCbacToken_t* gpbToken = 
                new rina::messages::smCbacToken_t();
        
        
        Token_t token = tokenSign.token;
        //fill in the token message object
        gpbToken->set_token_id(token.token_id);
        gpbToken->set_ipcp_issuer_id(token.ipcp_issuer_id);
        
        gpbToken->set_allocated_ipcp_holder_name(cbac_helpers::get_NewApplicationProcessNamingInfo_t(
                                        token.ipcp_holder_name));
        gpbToken->set_allocated_ipcp_issuer_name(cbac_helpers::get_NewApplicationProcessNamingInfo_t(
                                        token.ipcp_issuer_name));
        
        std::list<std::string> tokenAudience = token.audience;
        for(std::list<std::string>::iterator it = tokenAudience.begin();
                it != tokenAudience.end(); ++it) {
                gpbToken->add_audience(*it);
        }
        gpbToken->set_issued_time(token.issued_time);
        gpbToken->set_token_nbf(token.token_nbf);
        gpbToken->set_token_exp(token.token_exp);
        
        // Token capability        
        
        std::list<Capability_t> tokenCapList = token.token_cap;
        for(std::list<Capability_t>::iterator it = tokenCapList.begin();
                it != tokenCapList.end(); ++it) {
                
                Capability_t c = *it;
                
                rina::messages::smCapability_t * allocatedCap = gpbToken->add_token_cap();
                allocatedCap->set_ressource(c.ressource);
                allocatedCap->set_operation(c.operation);
        }
        
        gpbTokenSign.set_allocated_token(gpbToken);
        gpbTokenSign.set_token_sign(tokenSign.token_sign.data, tokenSign.token_sign.length);

        //serializing
        int size = gpbTokenSign.ByteSize();
        result.message_ = new unsigned char[size];
        result.size_ = size;
        gpbTokenSign.SerializeToArray(result.message_, size);
        
}

void deserializeTokenPlusSign(const rina::ser_obj_t &serobj,
                                TokenPlusSignature_t &des_token_sign)
{
    
        rina::messages::smCbacTokenPlusSign_t gpb;
        gpb.ParseFromArray(serobj.message_, serobj.size_);
        cbac_helpers::toModelTokenPlusSignature(gpb, des_token_sign);
        
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
        } 
        for (list<IPCPProfile_t>::const_iterator nit =
                        ipcpProfileList.begin();
                                nit != ipcpProfileList.end(); nit++) {
                ss << "IPCP Profile:" << endl;
                ss << "\tName: " << nit->ipcp_name.toString() << endl;
                ss << "\tType: " << nit->ipcp_type.c_str() << endl;
                ss << "\tGroup: " << nit->ipcp_group.c_str() << endl;

                std::list<RIBProfile_t> ribProfileList = nit->ipcp_rib_profiles;
                for (list<RIBProfile_t>::const_iterator it =
                        ribProfileList.begin();
                                it != ribProfileList.end(); it++) {
                        ss << "\tRIB type: " <<
                            it->rib_type.c_str() << endl;
                        ss << "\tRIB group: " <<
                            it->rib_group.c_str() << endl;   
                }
        }

        return ss.str();
}
//----------------------------
 /**
 * class ProfileParser: Parse AC files with jsoncpp
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
		LOG_ERR("Failed to parse AC profile file");

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
		
		Json::Value rib_profiles = ipcp_profiles[i]["ipcpRibProfile"];
                
                for (unsigned int i = 0; i < rib_profiles.size(); i++) {
                        RIBProfile_t ribProfile;
                        ribProfile.rib_group = rib_profiles[i].get("ribGroup", string()).asString();
                        ribProfile.rib_type = rib_profiles[i].get("ribType", string()).asString();
                        ipcp.ipcp_rib_profiles.push_back(ribProfile);
                        ribProfileList.push_back(ribProfile);
                }
		ipcpProfileList.push_back(ipcp);
				
	}
	LOG_IPCP_DBG("Profiles parsed: %s", toString().c_str());
	return true;
	
}

// to find the AC rule from policy file to be applied to the current profiles, 
// need to compare the current profiles with the 
// profiles defined in the rule

bool ProfileParser::compareRuleToProfile(Rule_t & r, std::string ipcp_type, std::string dif_type,
                        std::string member_ipcp_type)
{
        if (r.ipcp_type == ipcp_type &&
            r.dif_type == dif_type &&
            r.member_ipcp_type == member_ipcp_type)
                 return true;
        return false;
}

// This procedure checks if a rule relative to the current profile 
// (requestor and requestee) 
// is defined in the policy file, 
// in such case, enrollment is allowed and capability is computed,
// otherwise discard request

// NOTE: group attribute is not used in this policy, but useful to be here for
// future policies

bool ProfileParser::getCapabilityByProfile(const std::string fileName, 
                               std::list<Capability_t> & capList, 
                               std::string ipcp_type, std::string ipcp_group,
                               std::string dif_type, std::string dif_group, 
                               std::string member_ipcp_type, std::string member_ipcp_group)
{
        
        Json::Value  root;
        Json::Reader reader;
        ifstream     file;
        
        LOG_IPCP_DBG("Parsing file %s", fileName.c_str());
        
        file.open(fileName.c_str(), std::ifstream::in);
        if (file.fail()) {
                LOG_ERR("Failed to open AC policy file");
                return false;
        }

        if (!reader.parse(file, root, false)) {
                LOG_ERR("Failed to parse policy");
                cout << "Failed to parse JSON" << endl
                          << reader.getFormatedErrorMessages() << endl;

                return false;
        }

        file.close();
        
        // really parse
        bool shouldCopy = false;
        Json::Value info = root["enrollment"];
        for (unsigned int i = 0; i < info.size(); i++) {
                Json::Value ruleInfo = info[i]["rule"];
                for (int j = 0; j < ruleInfo.size(); j++){
                        Rule_t rule;
                        if (not ruleInfo[j].isNull()){
                                rule.ipcp_type = ruleInfo[j].get("ipcpType", string()).asString();
                                rule.dif_type = ruleInfo[j].get("difType", string()).asString();
                                rule.member_ipcp_type = ruleInfo[j].get("memberIpcpType", string()).asString();
                                rule.member_dif_type = ruleInfo[j].get("memberDifType", string()).asString();
                        }
                        
                        if (compareRuleToProfile(rule, ipcp_type,
                                dif_type, member_ipcp_type)){

                                LOG_IPCP_DBG("Rule found, copy capabilities");
                                shouldCopy = true;
                                break;
                        }
                }
                
                if (shouldCopy){
                        Json::Value capInfo = info[i]["capabilities"];
                        for (int j = 0; j < capInfo.size(); j++){
                                if (not capInfo[j].isNull()){
                                        Capability_t capability;
                                        capability.ressource = capInfo[j].get("ressource", string()).asString();
                                        capability.operation = capInfo[j].get("op", string()).asString();
                                        capList.push_back(capability);
                                }
                        }
                        //shouldCopy = false;
                        return true;
                }
        }
        LOG_IPCP_INFO("No rule found for the current profiles, deny enrollment");
        return false;
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
		if (it->ipcp_name.processName == ipcpName.processName) {
			result = *it;
			return true;
		}
	}

	return false;
}

bool ProfileParser::getAuthPolicyName(std::string fileName, std::string& name)
{
        Json::Value  root;
        Json::Reader reader;
        ifstream     file;
        
        LOG_IPCP_DBG("Parsing file %s", fileName.c_str());
        
        file.open(fileName.c_str(), std::ifstream::in);
        if (file.fail()) {
                LOG_ERR("Failed to open file");
                return false;
        }

        if (!reader.parse(file, root, false)) {
                LOG_ERR("Failed to parse file");
                cout << "Failed to parse JSON" << endl
                          << reader.getFormatedErrorMessages() << endl;

                return false;
        }

        file.close();
        
        // really parse
        Json::Value secManConf = root["securityManagerConfiguration"];
        if (secManConf != 0){
                Json::Value profiles = secManConf["authSDUProtProfiles"];
                
                if (profiles != Json::nullValue) {
                        Json::Value defaultProfile = profiles["default"];
                        if (defaultProfile != Json::nullValue) {
                                Json::Value authPolicy = defaultProfile["authPolicy"];
                                if (authPolicy != Json::nullValue){
                                        name = authPolicy.get("name", string()).asString();
                                        return true;
                                } else{
                                        name = "PSOC_authentication-none";
                                        return true;
                                }
                        }
                        else {
                                name = "PSOC_authentication-none";
                                return true;
                        }
                } else {
                        name = "PSOC_authentication-none";
                        return true;
                }
        }
        return false;
}


//----------------------

/*** 
 * class AccessControl: effective AC algorithm
 * **/

AccessControl::AccessControl()
{
	LOG_IPCP_DBG("Creating AccessControl Class");
}


bool AccessControl::checkJoinDIF(ProfileParser * parser,
                                 std::string policyFile, 
                                 const rina::ApplicationProcessNamingInformation & my_ipcp_name, 
                                 const rina::ApplicationProcessNamingInformation & my_dif_name, 
                                 const rina::ApplicationProcessNamingInformation & newMember_ipcp_name, 
                                 ac_res_info_t& result, std::list<Capability_t>& capList)
{
        LOG_IPCP_INFO("AC: Procedure for join DIF check");
        
        DIFProfile_t difProfile;
        IPCPProfile_t newMemberProfile;
        IPCPProfile_t myProfile;

        if (parser->getIPCPProfileByName(my_ipcp_name, myProfile) < 0) {
                LOG_IPCP_ERR("Error loading MY profile [%s]", 
                             my_ipcp_name.processName.c_str());
                result.code_ = AC_ENR_ERROR;
                result.reason_ = ABSENT_RPOFILE;
                return false;
            
        };
        
        if (parser->getDIFProfileByName(my_dif_name, difProfile) < 0) {
                LOG_IPCP_ERR("Error loading MY DIF profile [%s]", 
                             my_dif_name.processName.c_str());
                result.code_ = AC_ENR_ERROR;
                result.reason_ = ABSENT_RPOFILE;
                return false;
            
        }
        
        if (parser->getIPCPProfileByName(newMember_ipcp_name, newMemberProfile) < 0) {
                LOG_IPCP_ERR("Error loading New member profile [%s]", 
                             newMember_ipcp_name.processName.c_str());
                result.code_ = AC_ENR_ERROR;
                result.reason_ = ABSENT_RPOFILE;
                return false;
            
        }
        
        // Enrollment AC algorithm and capability generation
        ac_res_info_t res;
        if (parser->getCapabilityByProfile(policyFile, capList, 
                                  myProfile.ipcp_type, myProfile.ipcp_group,
                                  difProfile.dif_type, difProfile.dif_group,
                                  newMemberProfile.ipcp_type, newMemberProfile.ipcp_group) > 0 ){
        
                LOG_IPCP_INFO("Allow to access DIF and cap are:");
                result.code_ = AC_ENR_SUCCESS;
                return true;
        }else{
                result.code_ = AC_ENR_ERROR;
                result.reason_ = ENROLLMENT_NOT_ALLOWED;
                return false;
        }
        
}
        

void AccessControl::generateTokenSignature(Token_t &token, std::string encrypt_alg, 
                                   RSA * my_private_key,  rina::UcharArray &signature)
{
         // variable fitting
        LOG_IPCP_INFO("Generating Token Signature..");
        int t0 = cbac_helpers::getTimeMs();
        LOG_IPCP_DBG("START GENERATE_TOKEN_SIGN [%d] ms", t0);
       
        rina::ser_obj_t ser_token;
        serializeToken(token, ser_token);
        rina::UcharArray token_char(&ser_token);
        
        // hash then encrypt token
        rina::UcharArray result;
        cbac_helpers::hashToken(token_char, result, 
                                encrypt_alg);
        cbac_helpers::encryptHashedTokenWithPrivateKey(result, signature, 
                                     my_private_key);
        LOG_IPCP_DBG("Signature ready:  %d bytes: [%s]", 
                     result.length, result.toString().c_str());
        int t1 = cbac_helpers::getTimeMs();
        LOG_IPCP_DBG("END GENERATE_TOKEN_SIGN [%d] ms", t1);
        LOG_IPCP_DBG("TIME GENERATE_TOKEN_SIGN [%d] ms", t1-t0);
}

void AccessControl::generateToken(const rina::ApplicationProcessNamingInformation & ipcp_issuer_name, 
                                  unsigned short issuerIpcpId, DIFProfile_t& difProfile,
                                  IPCPProfile_t& newMemberProfile, rina::cdap_rib::auth_policy_t & auth,
                                  rina::SSH2SecurityContext *sc, std::string encryptAlgo, std::list<Capability_t>& capList)
{
        //rina::Time currentTime;
        int t0 = cbac_helpers::getTimeMs();
        LOG_IPCP_DBG("START GENERATE_TOKEN [%d] ms", t0);
        
        TokenPlusSignature_t tokenSign;
        
        Token_t token;
        LOG_IPCP_INFO("Generating Token...");
        token.token_id = issuerIpcpId;
        token.ipcp_issuer_id = issuerIpcpId;
        token.ipcp_issuer_name = ipcp_issuer_name;
        token.ipcp_holder_name = newMemberProfile.ipcp_name;
        token.audience.push_back("all");
        int t = cbac_helpers::getTimeMs();
        token.issued_time = t;
        token.token_nbf = 0; //token valid at reception time + token_nbf
        token.token_exp = VALIDITY_TIME_IN_HOURS; // after this time, the token will be invalid
        token.token_cap = capList; 
        
        //fill signature
        generateTokenSignature(token, encryptAlgo, 
            sc->auth_keypair, tokenSign.token_sign);
        tokenSign.token = token;
        // token should be encoded as ser_obj_t
        rina::ser_obj_t options;
        serializeTokenPlusSignature(tokenSign, options); 
        
        // fill auth structure
        
        auth.name = AC_CBAC;
        auth.versions.push_back(AC_CBAC_VERSION);
        auth.options = options;
        LOG_IPCP_DBG("Token generated: \n%s", tokenSign.toString().c_str());
        int t1 = cbac_helpers::getTimeMs();
        LOG_IPCP_DBG("END GENERATE_TOKEN [%d] ms", t1);
        LOG_IPCP_DBG("TIME GENERATE_TOKEN [%d] ms", t1-t0);
}
//-----------------------------------
/**
 * class SecurityManagerCBACPs
 * **/

SecurityManagerCBACPs::SecurityManagerCBACPs(IPCPSecurityManager * dm_)
						: dm(dm_)
{

        access_control_ = new AccessControl();
        profile_parser_ = new ProfileParser();
        my_ipcp_id = dm->ipcp->get_id();
        my_ipcp_name = dm->ipcp->get_name();
        my_dif_name = rina::ApplicationProcessNamingInformation(string(),
        						        string());
        max_retries = 0;
        LOG_IPCP_DBG("Creating SecurityManagerCBACPs: my_ipcp_id %d name %s", my_ipcp_id, 
                       my_ipcp_name.c_str());
}

int SecurityManagerCBACPs::loadProfilesByName(const rina::ApplicationProcessNamingInformation &ipcpProfileHolder,
					      IPCPProfile_t &requestedIPCPProfile,
                                              const rina::ApplicationProcessNamingInformation &difProfileHolder,
					      DIFProfile_t &requestedDIFProfile)
{
        
        const std::string profileFile = cbac_helpers::getStringParamFromConfig("ACprofilestore", dm);
        if (profileFile == std::string()){
                LOG_IPCP_ERR("Missing ProfileParser parameter configuration!");
                return -1;
        }
        
        // Read Profile From AC Profile Store
        ProfileParser profileParser;
        
        if (!profileParser.parseProfile(profileFile)){
                LOG_IPCP_DBG("Error Parsing Profile file");
                return -1;
        }

        LOG_IPCP_DBG("Parsing profile of %s" ,
                       ipcpProfileHolder.processName.c_str());
        
        if (!profileParser.getIPCPProfileByName(ipcpProfileHolder, requestedIPCPProfile)){
                LOG_IPCP_DBG("No Profile for this newMember [%s]!" ,
                              ipcpProfileHolder.processName.c_str());
                return -1;
        }
        
        LOG_IPCP_DBG("Parsing profile of DIF [%s]",
                       difProfileHolder.processName.c_str());
        
        if (!profileParser.getDIFProfileByName(difProfileHolder, requestedDIFProfile)){
                LOG_IPCP_DBG("No Profile for my DIF, not allowing IPCProcess %s to join DIF!",
                             ipcpProfileHolder.processName.c_str());
                return -1;
        }
        
        return 0;
}

int SecurityManagerCBACPs::loadProfiles()
{
        const std::string profileFile = cbac_helpers::getStringParamFromConfig("ACprofilestore", dm);
        if (profileFile == std::string()){
                LOG_IPCP_ERR("Missing ProfileParser parameter configuration!");
                return -1;
        }
        
        // Read Profile From AC profile Store
        if (!profile_parser_->parseProfile(profileFile)){
                LOG_IPCP_DBG("Error Parsing AC Profile file");
                return -1;
        }

        return 0;
}

int SecurityManagerCBACPs::isAllowedToJoinDAF(const rina::cdap_rib::con_handle_t &con, 
                                              const rina::Neighbor &newMember,
                                              rina::cdap_rib::auth_policy_t &auth)
{
	rina::ISecurityContext * sc;
	rina::SSH2SecurityContext * my_sc;

#if ACCESS_GRANTED
        LOG_IPCP_INFO("isAllowedToJoinDAF: ACCESS_GRANTED option set");
        return 0;
#endif
        
        if (my_dif_name.processName.c_str() == std::string()){
                my_dif_name = dm->ipcp->get_dif_information().dif_name_;
                LOG_IPCP_INFO("SecurityManagerCBACPs: isAllowedToJoinDAF my_dif_name %s", 
                     my_dif_name.processName.c_str());
        }

        sc = dm->get_security_context(con.port_id);
        if (!sc) {
                LOG_IPCP_ERR("Could not find security context for session_id %d",
                	     con.port_id);
                return -1;
        }
        std::string authPolicyName = sc->auth_policy_name;

        LOG_IPCP_DBG("SecurityManagerCBACPs: Joint AuthPolicyName is %s",
                authPolicyName.c_str());
        
        if (authPolicyName == rina::IAuthPolicySet::AUTH_NONE 
                || authPolicyName == rina::IAuthPolicySet::AUTH_PASSWORD){
                LOG_IPCP_DBG("SecurityManagerCBACPs: Auth policy is %s, then Positive AC",
                              authPolicyName.c_str());
                return 0;
        }
        
        if (loadProfiles() < 0){
                LOG_IPCP_ERR("Error loading profiles ");
                return -1;
        }
        
        my_sc = dynamic_cast<rina::SSH2SecurityContext *>(sc);
        if (!my_sc) {
                LOG_IPCP_ERR("Could not find security context for session_id %d",
                	     con.port_id);
                return -1;
        }

	// Enrollment AC algorithm
	ac_res_info_t res;
        const std::string policyFile = cbac_helpers::getStringParamFromConfig("ACPolicystore", dm);
        
        std::list<Capability_t> capList;
        access_control_->checkJoinDIF(profile_parser_,
        			      policyFile,
                                      rina::ApplicationProcessNamingInformation(my_ipcp_name,
                                		      	      	      	        string()),
                                      my_dif_name,
				      newMember.name_,
				      res,
				      capList);

	if (res.code_ == AC_ENR_SUCCESS) {
                    LOG_IPCP_INFO("Allowing IPC Process %s to join the DIF. Going to generate token",
                		  newMember.name_.processName.c_str());
                    std::string encryptAlgo = cbac_helpers::getStringParamFromConfig("EncryptAlgo", dm);
                    if (encryptAlgo == std::string()){
                            LOG_IPCP_ERR("Missing EncryptAlgo parameter configuration!");
                            return -1;
                    }
                    
                    IPCPProfile_t newMemberProfile;
                    DIFProfile_t difProfile; 
                    if (loadProfilesByName(newMember.name_,
                		    	   newMemberProfile,
					   my_dif_name,
					   difProfile) < 0){ //FIXME: is it needed?
                            LOG_IPCP_ERR("Error loading profiles ");
                            return -1;
                    }
        
                    access_control_->generateToken(rina::ApplicationProcessNamingInformation(my_ipcp_name,
                		    	    	    	    	    	    	    	     string()),
                		    	    	    	    	    	    	    	     my_ipcp_id,
											     difProfile,
											     newMemberProfile,
											     auth,
											     my_sc,
											     encryptAlgo,
											     capList);
                    return 0;
	} else {
		LOG_IPCP_ERR("NOT Allowing IPC Process %s to join the DIF because of %s",
		     newMember.name_.processName.c_str(), res.reason_.c_str());
		return -1;
	}
}

// In addition to securityContext, any IPCP needs to load 
// the public key of the token generator
RSA* SecurityManagerCBACPs::loadTokenGeneratorPublicKey(const std::string& tokenGenIpcpName,
							const std::string& keyStorePath)
{
        BIO * keystore;
        std::stringstream ss;
        //Read peer public key from keystore
        ss << keyStorePath << "/" << tokenGenIpcpName;
        keystore =  BIO_new_file(ss.str().c_str(), "r");
        if (!keystore) {
                LOG_ERR("Problems opening keystore file at: %s",
                        ss.str().c_str());
                return NULL;
        }
        
        RSA* token_generator_pub_key = PEM_read_bio_RSA_PUBKEY(keystore, NULL, 0, NULL);
        BIO_free(keystore);
        if (!token_generator_pub_key) {
                LOG_ERR("Problems reading RSA public key from keystore (%s): %s",
                        ss.str().c_str(), ERR_error_string(ERR_get_error(), NULL));
                return NULL;
        }
        return token_generator_pub_key;
}

int SecurityManagerCBACPs::checkTokenSignature(const Token_t &token,
					       const rina::UcharArray & signature,
                                               const std::string& encrypt_alg,
					       const std::string& keystorePath)
{
        
        //objective: hash(token) =? decrypt_public_key_issuer(signature)
        LOG_IPCP_DBG("Validating token signature...");
        rina::ser_obj_t ser_token;
        serializeToken(token, ser_token);
        
        rina::UcharArray token_char(&ser_token);
        // hash then encrypt token
        rina::UcharArray result;
        cbac_helpers::hashToken(token_char, result, encrypt_alg);
        std::string name = token.ipcp_issuer_name.toString();
        std::string tokenGenIpcpName = name.substr(0, name.find(":"));
        RSA* token_generator_pub_key = loadTokenGeneratorPublicKey(tokenGenIpcpName,
        							   keystorePath);
        if (token_generator_pub_key == NULL){
                LOG_IPCP_ERR("Signature verification failed, because of absent token generator public key");
                return -1;
        }
        
        rina::UcharArray decrypt_sign;
        decrypt_sign.data = new unsigned char[RSA_size(token_generator_pub_key)];
        decrypt_sign.length = RSA_public_decrypt(signature.length,
                                                 signature.data,
						 decrypt_sign.data,
						 token_generator_pub_key,
						 RSA_PKCS1_PADDING);

        if (decrypt_sign.length == -1) {
                LOG_IPCP_ERR("Error decrypting raw signature with peer public key: %s",
                             ERR_error_string(ERR_get_error(), NULL));
                return -1;
        }
        
        if (result == decrypt_sign){
                LOG_IPCP_INFO("Valid signature");
                return 0;
        } 
               
        LOG_IPCP_ERR("Invalid signature");
        return -1;
}

int SecurityManagerCBACPs::storeAccessControlCreds(const rina::cdap_rib::auth_policy_t & auth,
                                                   const rina::cdap_rib::con_handle_t & con)
{
	rina::ISecurityContext * sc;

#if ACCESS_GRANTED
        LOG_IPCP_INFO("storeAccessControlCreds: ACCESS_GRANTED option set");
        return 0;
#else
        
        if (my_dif_name.processName.c_str() == std::string()){
                my_dif_name = dm->ipcp->get_dif_information().dif_name_;
                LOG_IPCP_INFO("SecurityManagerCBACPs: storeAccessControlCreds my_dif_name %s", 
                     my_dif_name.processName.c_str());
        }
        
        sc = dm->get_security_context(con.port_id);
        if (!sc) {
                LOG_IPCP_ERR("Could not find security context for session_id %d",
                	     con.port_id);
                return -1;
        }
        std::string authPolicyName = sc->auth_policy_name;

        if (authPolicyName == rina::IAuthPolicySet::AUTH_NONE 
                || authPolicyName == rina::IAuthPolicySet::AUTH_PASSWORD){
                LOG_IPCP_INFO("SecurityManagerCBACPs: Auth policy is %s, then no token to store",
                              authPolicyName.c_str());
                return 0;
        }
        
        LOG_IPCP_DBG("Storing MY AC Credentials (token assigned from %s to %s)",
                     con.dest_.ap_name_.c_str(), con.src_.ap_name_.c_str());
        
        if(auth.options.size_ > 0){
                my_token = auth.options;
                TokenPlusSignature_t tokenSign;
                deserializeTokenPlusSign(auth.options, tokenSign);
                LOG_IPCP_INFO("Token stored: \n%s", tokenSign.toString().c_str());
                return 0;
        } else{
                LOG_IPCP_ERR("Nothing to store: Empty token field");
                return -1;
        }
#endif
}

int SecurityManagerCBACPs::generateTokenForTokenGenerator(rina::cdap_rib::auth_policy_t & auth, 
                                                          const rina::cdap_rib::con_handle_t & con)
{
	rina::SSH2SecurityContext * my_sc;

        std::string encryptAlgo = cbac_helpers::getStringParamFromConfig("EncryptAlgo", dm);
        if (encryptAlgo == std::string()){
                LOG_IPCP_ERR("Missing EncryptAlgo parameter configuration!");
                return -1;
        }

        if (my_dif_name.processName.c_str() == std::string()){
                my_dif_name = dm->ipcp->get_dif_information().dif_name_;
                LOG_IPCP_DBG("SecurityManagerCBACPs: generateTokenForTokenGenerator my_dif_name %s", 
                     my_dif_name.processName.c_str());
        }

        IPCPProfile_t myIPCPProfile;
        DIFProfile_t myDifProfile;
        if (loadProfilesByName(rina::ApplicationProcessNamingInformation(my_ipcp_name,
        								 string()),
        								 myIPCPProfile,
									 my_dif_name,
									 myDifProfile) < 0) {
                LOG_IPCP_ERR("Error loading profiles");
                return -1;
        }
        
        my_sc = dynamic_cast<rina::SSH2SecurityContext *>(dm->get_security_context(con.port_id));
        if (!my_sc) {
                LOG_IPCP_ERR("Could not find security context for session_id %d",
                	     con.port_id);
                return -1;
        }
        
        std::list<Capability_t> capList;
        capList.push_back(Capability_t("all", "all"));
        access_control_->generateToken(rina::ApplicationProcessNamingInformation(my_ipcp_name, string()),
        			       my_ipcp_id,
				       myDifProfile,
				       myIPCPProfile,
                                       auth,
				       my_sc,
				       encryptAlgo,
				       capList);
        return 0;
}

// called at the token generator, this function allows it to generate and store 
// its own token, return -1 if failed
// at any other node, if the token is not there, proceed (return 0) because
// this procedure can be invoked by authentication messages which are prior to token geenrator
// so it is expected that the token is not there, 
// but for the token generator it would be the occasion to generate it
int SecurityManagerCBACPs::getAccessControlCreds(rina::cdap_rib::auth_policy_t & auth,
                                                 const rina::cdap_rib::con_handle_t & con)
{
	rina::ISecurityContext * sc;

#if ACCESS_GRANTED
        LOG_IPCP_INFO("getAccessControlCreds: ACCESS_GRANTED option set");
        return 0;
#endif 
        
        sc = dm->get_security_context(con.port_id);
        if (!sc) {
                LOG_IPCP_ERR("Could not find security context for session_id %d",
                	     con.port_id);
                return -1;
        }
        std::string authPolicyName = sc->auth_policy_name;

        LOG_IPCP_DBG("getAccessControlCreds: Auth policy is %s",
        	     authPolicyName.c_str());
        if (authPolicyName == rina::IAuthPolicySet::AUTH_NONE 
                || authPolicyName == rina::IAuthPolicySet::AUTH_PASSWORD){
                
                LOG_IPCP_DBG("getAccessControlCreds: Auth policy is %s, then no token to return",
                              authPolicyName.c_str());
                return 0;
        }
        
        if (my_token.size_ <= 0){
                LOG_IPCP_DBG("Asked to get MY AC Credentials BUT token is not there");
                std::string tokenGenIpcpName = cbac_helpers::getStringParamFromConfig("TokenGenIPCPName", dm);
                if (tokenGenIpcpName == std::string()){
                        LOG_IPCP_ERR("Missing tokenGenIpcpName parameter configuration!");
                        return -1;
                }
                if (tokenGenIpcpName == my_ipcp_name){
                        LOG_IPCP_DBG("I am the token generator, I have the priviledge to generate my own token!");
                        //generate its own token using same algorithm
                        if (generateTokenForTokenGenerator(auth, con) < 0) {
                                LOG_IPCP_ERR("Failed to generate token!");
                                return -1;
                        }
                        //need to store token for future usage
                        my_token = auth.options;
                        TokenPlusSignature_t tokenSign;
                        deserializeTokenPlusSign(auth.options, tokenSign);
                        LOG_IPCP_INFO("Token stored: \n%s", tokenSign.toString().c_str());
                        return 0;
                    
                }else{
                        LOG_IPCP_DBG("And I am not the token generator, proceed anyway");
                        return 0;
                }
        }
        
        LOG_IPCP_DBG("Asked to get MY AC Credentials, token size: %d", 
                             my_token.size_);
        auth.options = my_token;
        return 0;
}

int SecurityManagerCBACPs::checkTokenValidity(const TokenPlusSignature_t &tokenSign,
                                              const std::string& requestor,
					      const std::string& keyStorePath)
{
        Token_t token = tokenSign.token;
        stringstream ss;
        ss << "Validating token..." << endl;
        
        //1. check audience
        if((std::find(token.audience.begin(), token.audience.end(), my_ipcp_name) != token.audience.end()) ||
            (std::find(token.audience.begin(), token.audience.end(), "all") != token.audience.end())){
                ss << "\t 1. I am in token audience, continue validation" << endl;
                LOG_IPCP_DBG("%s", ss.str().c_str());
                ss.str(std::string());
        }else{
                ss << "\n\t 1. I am not in the audience, failed token validation" << endl;
                LOG_IPCP_DBG("%s", ss.str().c_str());
                return -1;
        }
        
        //2. check token_holder == requestor
        if (token.ipcp_holder_name.processName == requestor){
                ss << "\n\t 2. Token holder is the requestor, continue validation" << endl;
                LOG_IPCP_DBG("%s", ss.str().c_str());
                ss.str(std::string());
        }else{
                ss << "\n\t 2. Token holder is NOT the requestor, failed token validation" << endl;
                LOG_IPCP_DBG("%s", ss.str().c_str());
                return -1;
        }
        
        //3. check nbf
        int now = cbac_helpers::getTimeMs();
        if (FIRST_TIME_RCV_TOKEN == 0)
                FIRST_TIME_RCV_TOKEN = now;
        if (FIRST_TIME_RCV_TOKEN + token.token_nbf <= now) {
                ss << "\n\t 3. Token nbf is valid, continue token validation"<< endl;
                LOG_IPCP_DBG("%s", ss.str().c_str());
                ss.str(std::string());
        }else{
                ss << "\n\t 3. Token cannot not be used yet (now:" << now
                   << ", nbf: "<< token.token_nbf << "), failed token validation" << endl;
                LOG_IPCP_DBG("%s", ss.str().c_str());
                return -1;
        }
        
        //4. check expiration time
        int until = token.token_exp * 3600 * 1000 + FIRST_TIME_RCV_TOKEN; //token.issued_time;
        if(until >= now) {
                ss << "\n\t 4. Token has not expired yet, continue token validation"<< endl;
                LOG_IPCP_DBG("%s", ss.str().c_str());
                ss.str(std::string());
        }else{
                ss << "\n\t 4. Token has expired, failed token validation" << endl;
                LOG_IPCP_DBG("%s", ss.str().c_str());
                return -1;
        }
        
        //5. check signature
        std::string encryptAlgo = cbac_helpers::getStringParamFromConfig("EncryptAlgo", dm);
        if (encryptAlgo == std::string()){
                LOG_IPCP_ERR("Missing EncryptAlgo parameter configuration!");
                return -1;
        }
        
        if (checkTokenSignature(token,
        			tokenSign.token_sign,
				encryptAlgo,
				keyStorePath) == -1) {
                ss << "\n\t 5. Token Signature invalid, failed token validation" << endl;
                LOG_IPCP_DBG("%s", ss.str().c_str());
                return -1; 
        } 
        ss << "\n\t 5. Token Signature valid"<< endl;
        LOG_IPCP_DBG("%s", ss.str().c_str());
        LOG_IPCP_INFO("Token is valid");
        return 0;
}

void SecurityManagerCBACPs::checkRIBOperation(const rina::cdap_rib::auth_policy_t & auth,
                                          const rina::cdap_rib::con_handle_t & con,
                                          const rina::cdap::cdap_m_t::Opcode opcode,
                                          const std::string obj_name,
                                          rina::cdap_rib::res_info_t& res)
{
	rina::ISecurityContext * sc;
	rina::SSH2SecurityContext * my_sc;

#if ACCESS_GRANTED
        LOG_IPCP_INFO("checkRIBOperation: ACCESS_GRANTED option set");
        res.code_ = rina::cdap_rib::CDAP_SUCCESS;
        return;
#endif
        LOG_IPCP_INFO("checkRIBOperation: Context = %s", 
                     cbac_helpers::operationToString(con, opcode, obj_name).c_str());
        
        sc = dm->get_security_context(con.port_id);
        if (!sc) {
                LOG_IPCP_ERR("Could not find security context for session_id %d",
                	     con.port_id);
                res.code_ = rina::cdap_rib::CDAP_ERROR;
                return;
        }
        std::string authPolicyName = sc->auth_policy_name;

        if (authPolicyName == rina::IAuthPolicySet::AUTH_NONE 
                || authPolicyName == rina::IAuthPolicySet::AUTH_PASSWORD){
                LOG_IPCP_DBG("checkRIBOperation: Auth policy is %s, then operation granted",
                              authPolicyName.c_str());
                res.code_ = rina::cdap_rib::CDAP_SUCCESS;
                return;
        }
        
        if (auth.options.size_ < 0){
                LOG_IPCP_ERR("Empty token field");
                res.code_ = rina::cdap_rib::CDAP_ERROR;
                return;
        }
        
        std::string requestor = con.dest_.ap_name_;
        if (requestor == std::string()){
                // con dest_ap_name is empty, need a hack: take requestor from token attached
                LOG_IPCP_DBG("Empty requestor name from con_handle, try to retrieve it from token as a hack");
        }
        
        my_sc = dynamic_cast<rina::SSH2SecurityContext *>(sc);
        if (!my_sc) {
                LOG_IPCP_ERR("Could not find security context for session_id %d",
                	     con.port_id);
                res.code_ = rina::cdap_rib::CDAP_ERROR;
                return;
        }
        
        //get token
        TokenPlusSignature_t tokenSign;
        deserializeTokenPlusSign(auth.options, tokenSign);
        if (requestor == std::string()){
                requestor = tokenSign.token.ipcp_holder_name.processName; 
                // NOTE: the requestor could be retrieved from con object, but to avoid con missing field bug, read it from the token
        }
        
        if (checkTokenValidity(tokenSign, requestor, my_sc->keystore_path) < 0){
                LOG_IPCP_ERR("Invalid Token, Deny request ");
                res.code_ = rina::cdap_rib::CDAP_ERROR;
                return; 
        }
        
        Token_t token = tokenSign.token;
        std::list<Capability_t> tokenCapList = token.token_cap;
            
        for(std::list<Capability_t>::iterator it = tokenCapList.begin();
                it != tokenCapList.end(); ++it) {
                if (it->compare(obj_name, cbac_helpers::opcodeToString(opcode)) ||
                    it->compare("all", "all") ||
                    (it->operation == cbac_helpers::opcodeToString(opcode) 
                        //s1.find(s2) != std::string::npos
                        and (obj_name.find(it->ressource) != std::string::npos))){
                        LOG_IPCP_INFO("Request is included in requestor capability; Grant access");
                        res.code_ = rina::cdap_rib::CDAP_SUCCESS;
                        return;
                }
        }
        
        LOG_IPCP_ERR("Request in NOT in requestor capability; Deny request");
        res.code_ = rina::cdap_rib::CDAP_ERROR;
        return;   
}

bool SecurityManagerCBACPs::acceptFlow(const configs::Flow& newFlow)
{
	LOG_IPCP_DBG("Accepting flow from remote application %s",
			newFlow.remote_naming_info.getEncodedString().c_str());
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

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
// #include <cstdlib>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/md5.h>
// #include <openssl/pem.h>
// #include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/ssl.h>


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
        for (int i = 0; i < gpb_token.audience_size(); i++){
        
            des_token.audience.push_back(gpb_token.audience(i));
        }
        
        des_token.issued_time = gpb_token.issued_time();
        des_token.token_nbf = gpb_token.token_nbf();
        des_token.token_exp = gpb_token.token_exp();
        
        
        for (int i = 0; i < gpb_token.token_cap_size(); i++)
        {
                Capability_t cap; // = new Capability_t();
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

std::string operationToString( //const rina::cdap_rib::auth_policy_t & auth,
                                const rina::cdap_rib::con_handle_t & con,
                                const rina::cdap::cdap_m_t::Opcode opcode,
                                const std::string obj_name)
{
        stringstream ss;
        
        ss << "\nOpcode: " << opcodeToString(opcode).c_str() << endl;
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
        
        //fill in the token message object
        gpbToken.set_token_id(token.token_id);
        gpbToken.set_ipcp_issuer_id(token.ipcp_issuer_id);
        
        gpbToken.set_allocated_ipcp_holder_name(cbac_helpers::get_NewApplicationProcessNamingInfo_t(
                                        token.ipcp_holder_name));
        
        for (std::list<std::string>::const_iterator it =
                        token.audience.begin();
                        it != token.audience.end(); ++it)
        {
                gpbToken.add_audience(*it);
        }
        //gpbToken.set_audience(token.audience);
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
        
        //serializing
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
        //gpbTokenSign.set_allocated_token(&gpbToken);
        
        gpbTokenSign.set_allocated_token(gpbToken);
        gpbTokenSign.set_token_sign(tokenSign.token_sign.data, tokenSign.token_sign.length);

        //serializing
        int size = gpbTokenSign.ByteSize();
        result.message_ = new unsigned char[size];
        result.size_ = size;
        gpbTokenSign.SerializeToArray(result.message_, size);
        
}
#if 0
void deserializeToken(const rina::ser_obj_t &serobj,
                                Token_t &des_token)
{
    
        rina::messages::smCbacToken_t gpb;
        gpb.ParseFromArray(serobj.message_, serobj.size_);
        cbac_helpers::toModelToken(gpb, des_token);
        
}
#endif
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
	result.code_ = AC_ENR_SUCCESS;
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
                Capability_t cap = Capability_t("PDU_FWD_TABLE", "READ");
                result.push_back(cap); 
                cap = Capability_t("PDU_FWD_TABLE", "WRITE");
                result.push_back(cap);
                cap = Capability_t("IPCP_ADDR_TABLE", "READ");
                result.push_back(cap);
                cap = Capability_t("flows", "READ");
                result.push_back(cap); 
                cap = Capability_t("flows", "WRITE");
                result.push_back(cap); 
        }
        Capability_t cap = Capability_t("watchdog", "READ");
        result.push_back(cap);
        return result;
    
}

int getTimeMs()
{
        timeval time_;
        gettimeofday(&time_, 0);
        int time_seconds = (int) time_.tv_sec;
        return (int) time_seconds * 1000 + (int) (time_.tv_usec / 1000);
}

void AccessControl::generateTokenSignature(Token_t &token, std::string encrypt_alg, 
                                   RSA * my_private_key,  rina::UcharArray &signature)
{
         // variable fitting
        //rina::Time currentTime;
        LOG_IPCP_DBG("Generating Token Signature..");
        int t0 = getTimeMs();
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
        int t1 = getTimeMs();
        LOG_IPCP_DBG("END GENERATE_TOKEN_SIGN [%d] ms", t1);
        LOG_IPCP_DBG("TIME GENERATE_TOKEN_SIGN [%d] ms", t1-t0);
}

void AccessControl::generateToken(unsigned short issuerIpcpId, DIFProfile_t& difProfile,
                                  IPCPProfile_t& newMemberProfile, rina::cdap_rib::auth_policy_t & auth,
                                  rina::SSH2SecurityContext *sc, std::string encryptAlgo)
{
        //rina::Time currentTime;
        int t0 = getTimeMs();
        LOG_IPCP_DBG("START GENERATE_TOKEN [%d] ms", t0);
        std::list<Capability_t> result = computeCapabilities(difProfile, newMemberProfile); 
        
        TokenPlusSignature_t tokenSign;
        
        Token_t token;
        LOG_IPCP_DBG("Generating Token...");
        token.token_id = issuerIpcpId; // TODO name or id?
        token.ipcp_issuer_id = issuerIpcpId;
        token.ipcp_holder_name = newMemberProfile.ipcp_name;
        token.audience.push_back("all");
        int t = getTimeMs();
        token.issued_time = t;
        token.token_nbf = t - 300; //token valid immediately, (-300 to avoid clock draft between peer ipcp)
        token.token_exp = VALIDITY_TIME_IN_HOURS; // after this time, the token will be invalid
        token.token_cap = result; 
        
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
        int t1 = getTimeMs();
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
        my_ipcp_id = dm->ipcp->get_id();
        my_ipcp_name = dm->ipcp->get_name();
        my_dif_name = rina::ApplicationProcessNamingInformation(
                        std::string(), string());
        //trusted_ap_name.push_back("B.IRATI");
        //trusted_ap_name.push_back("S0");
        //trusted_ap_name.push_back("E0");
        LOG_IPCP_DBG("Creating SecurityManagerCBACPs: my_ipcp_id %d name %s", my_ipcp_id, 
                       my_ipcp_name.c_str());
}


int SecurityManagerCBACPs::initialize_SC(const rina::cdap_rib::con_handle_t &con){
        
        
        //rina::ScopedLock sc_lock(lock);
        my_sc = dynamic_cast<rina::SSH2SecurityContext *>(dm->get_security_context(con.port_id));
        if (!my_sc) {
                LOG_IPCP_ERR("Could not find pending security context for session_id %d",
                        con.port_id);
                return -1;
        }
        return 0;
}

std::string getStringParamFromConfig(std::string paramName, IPCPSecurityManager *dm){
        std::string result = dm->ipcp->get_dif_information().
                    get_dif_configuration().sm_configuration_.policy_set_.
                    get_param_value_as_string(paramName);
        return result;
}
/*
int updateFromConfig(){
        
        my_config.profileFile = dm->ipcp->get_dif_information().
                    get_dif_configuration().sm_configuration_.policy_set_.
                    get_param_value_as_string("ACprofilestore");
        if (my_config.profileFile == std::string()){
            LOG_IPCP_ERR("Missing ProfileParser parameter configuration!");
                return -1;
        }
        my_config.tokenGenIpcpName = dm->ipcp->get_dif_information().
                    get_dif_configuration().sm_configuration_.policy_set_.
                    get_param_value_as_string("TokenGenIPCPName");
        if (my_config.tokenGenIpcpName == std::string()){
            LOG_IPCP_ERR("Missing tokenGenIpcpName parameter configuration!");
                return -1;
        }             
        my_config.encryptAlgo = dm->ipcp->get_dif_information().
                    get_dif_configuration().sm_configuration_.policy_set_.
                    get_param_value_as_string("EncryptAlgo");
        if (my_config.encryptAlgo == std::string()){
            LOG_IPCP_ERR("Missing encryptAlgo parameter configuration!");
                return -1;
        }
        return 0;
}
*/
int SecurityManagerCBACPs::loadProfilesByName(const rina::ApplicationProcessNamingInformation &ipcpProfileHolder, IPCPProfile_t &requestedIPCPProfile,
                                             const rina::ApplicationProcessNamingInformation &difProfileHolder, DIFProfile_t &requestedDIFProfile)
{
        
        const std::string profileFile = getStringParamFromConfig("ACprofilestore", dm);
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
        
        LOG_IPCP_DBG("Parsing profile of DIF  [%s]",
                       difProfileHolder.processName.c_str());
        
        if (!profileParser.getDIFProfileByName(difProfileHolder, requestedDIFProfile)){
                LOG_IPCP_DBG("No Profile for my DIF, not allowing IPCProcess %s to join DIF!",
                                ipcpProfileHolder.processName.c_str());
                return -1;
        }
        
        return 0;
}

int SecurityManagerCBACPs::isAllowedToJoinDAF(const rina::cdap_rib::con_handle_t &con, 
                                              const rina::Neighbor &newMember,
                                               rina::cdap_rib::auth_policy_t &auth)
{
        
#if ACCESS_GRANTED
        return 0;
#endif
        
        if (my_dif_name.processName.c_str() == std::string()){
                my_dif_name = dm->ipcp->get_dif_information().dif_name_;
                LOG_IPCP_DBG("SecurityManagerCBACPs: isAllowedToJoinDAF my_dif_name %s", 
                     my_dif_name.processName.c_str());
        }
        //my_dif_name = dm->ipcp->get_dif_information().dif_name_;
        
        LOG_IPCP_DBG("SecurityManagerCBACPs: isAllowedToJoinDAF my_dif_name %s", 
                     my_dif_name.processName.c_str());
        
        
        std::string authPolicyName = con.auth_.name;
        if (authPolicyName == rina::IAuthPolicySet::AUTH_NONE 
                || authPolicyName == rina::IAuthPolicySet::AUTH_PASSWORD){
                LOG_IPCP_DBG("SecurityManagerCBACPs: Auth policy is %s, then Positive AC",
                              authPolicyName.c_str());
                // return 0;
        }
        
        LOG_IPCP_DBG("SecurityManagerCBACPs: Joint AuthPloicyName is %s",
                authPolicyName.c_str());
        
        //here authPolicyName should be either SSH2 (or TLS in the future)
        if (initialize_SC(con) != 0){
            LOG_IPCP_ERR("Error initializing CBAC SC, return ...");
            return -1;
        }
        assert(my_sc);
        
        LOG_IPCP_DBG("SecurityManagerCBACPs: Initialized Security Context");
        
        IPCPProfile_t newMemberProfile;
        DIFProfile_t difProfile; 
        if (loadProfilesByName(newMember.name_, newMemberProfile, my_dif_name, difProfile) < 0){
                LOG_IPCP_ERR("Error loading profiles ");
                return -1;
        }
        
	// Enrollment AC algorithm
	ac_res_info_t res;
	access_control_->checkJoinDIF(difProfile, newMemberProfile, res);
	if (res.code_ == 0){
                    LOG_IPCP_DBG("Allowing IPC Process %s to join the DIF. Going to generate token",
                        newMember.name_.processName.c_str());
                    std::string encryptAlgo = getStringParamFromConfig("EncryptAlgo", dm);
                    if (encryptAlgo == std::string()){
                            LOG_IPCP_ERR("Missing EncryptAlgo parameter configuration!");
                            return -1;
                    }
                    access_control_->generateToken(my_ipcp_id, difProfile, newMemberProfile, 
                    auth, my_sc, encryptAlgo) ;
                    return 0;
	}
	if (res.code_ != 0){
		LOG_IPCP_ERR("NOT Allowing IPC Process %s to join the DIF because of %s",
		     newMember.name_.processName.c_str(), res.reason_.c_str());
		return -1;
	}
	
}

// In addition to securityContext, any IPCP needs to load 
// the public key of the token generator
RSA* SecurityManagerCBACPs::loadTokenGeneratorPublicKey()
{
        BIO * keystore;
        std::stringstream ss;
        //Read peer public key from keystore
        std::string tokenGenIpcpName = getStringParamFromConfig("TokenGenIPCPName", dm);
        if (tokenGenIpcpName == std::string()){
            LOG_IPCP_ERR("Missing tokenGenIpcpName parameter configuration!");
                return NULL;
        }
                    
        ss << my_sc->keystore_path << "/" << tokenGenIpcpName;
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

int SecurityManagerCBACPs::checkTokenSignature(const Token_t &token, const rina::UcharArray & signature, 
                                               std::string encrypt_alg)
{
        
        //objective: hash(token) =? decrypt_public_key_issuer(signature)
        LOG_IPCP_DBG("Validating token signature...");
        assert(my_sc);
        rina::ser_obj_t ser_token;
        serializeToken(token, ser_token);
        
        rina::UcharArray token_char(&ser_token);
        // hash then encrypt token
        rina::UcharArray result;
        cbac_helpers::hashToken(token_char, result, encrypt_alg);
        RSA* token_generator_pub_key = loadTokenGeneratorPublicKey();
        if (token_generator_pub_key == NULL){
                LOG_IPCP_ERR("Signature verification failed, because of absent public key");
                return -1;
        }
        
        rina::UcharArray decrypt_sign;
        decrypt_sign.data = new unsigned char[RSA_size(token_generator_pub_key/*my_sc->auth_peer_pub_key*/)];
        decrypt_sign.length = RSA_public_decrypt(signature.length,
                                               signature.data,
                                               decrypt_sign.data,
                                               token_generator_pub_key/*my_sc->auth_peer_pub_key*/,
                                               RSA_PKCS1_PADDING);

        if (decrypt_sign.length == -1) {
                LOG_IPCP_ERR("Error decrypting raw signature with peer public key: %s",
                        ERR_error_string(ERR_get_error(), NULL));
                return -1;
        }
        
        if (result == decrypt_sign){
                LOG_IPCP_DBG("Valid signature");
                return 0;
        } 
               
        LOG_IPCP_DBG("Invalid signature");
        return -1;
        
}

int SecurityManagerCBACPs::storeAccessControlCreds(const rina::cdap_rib::auth_policy_t & auth,
                                               const rina::cdap_rib::con_handle_t & con)
{
#if ACCESS_GRANTED
        return 0;
#else
        LOG_IPCP_DBG("Storing MY AC Credentials (token assigned from %s to %s)",
                     con.dest_.ap_name_.c_str(), con.src_.ap_name_.c_str());
        
        if (my_dif_name.processName.c_str() == std::string()){
                my_dif_name = dm->ipcp->get_dif_information().dif_name_;
                LOG_IPCP_DBG("SecurityManagerCBACPs: storeAccessControlCreds my_dif_name %s", 
                     my_dif_name.processName.c_str());
        }
        
        if(!my_sc){
                //here authPolicyName should be either SSH2, (or TLS in the future)
                if (initialize_SC(con) != 0){
                        LOG_IPCP_ERR("Error initializing CBAC SC, return ...");
                        return -1;
                }
        }
        
        /*assert(my_sc);
        if (checkTokenSignature(tokenSign.token, tokenSign.token_sign, 
                                                ENCRYPT_ALG) == -1){
            LOG_IPCP_DBG("Failed token signature validation");
                
        }
        */
        
        if(auth.options.size_ > 0){
            my_token = auth.options;
            TokenPlusSignature_t tokenSign;
            deserializeTokenPlusSign(auth.options, tokenSign);
            LOG_IPCP_DBG("Token stored: \n%s", tokenSign.toString().c_str());
            return 0;
        } else{
            LOG_IPCP_ERR("Nothing to store: Empty token field");
            return -1;
        }
#endif
}

int SecurityManagerCBACPs::generateTokenForTokenGenerator(rina::cdap_rib::auth_policy_t & auth, 
                                                          const rina::cdap_rib::con_handle_t & con){
    
        std::string encryptAlgo = getStringParamFromConfig("EncryptAlgo", dm);
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
        if (loadProfilesByName(rina::ApplicationProcessNamingInformation(
                        my_ipcp_name, string()), myIPCPProfile, my_dif_name, myDifProfile) < 0){
                LOG_IPCP_ERR("Error loading profiles");
                return -1;
        }
        
        if (!my_sc) {
                initialize_SC(con);
        }
        assert(my_sc);
        access_control_->generateToken(my_ipcp_id, myDifProfile, myIPCPProfile, 
                                       auth, my_sc, encryptAlgo);
        return 0;
                    
}

// called at the token generator, this function allows it to generate and store 
// its own token, return -1 if failed
// at any other node, if the token is not there, proceed (return 0) becasue
// this can be invoked by authentication messages awhich are priori to token geenrator
// so it is expected that the token is not there
int SecurityManagerCBACPs::getAccessControlCreds(rina::cdap_rib::auth_policy_t & auth,
                                             const rina::cdap_rib::con_handle_t & con)
{
        if (my_token.size_ <= 0){
                LOG_IPCP_DBG("Asked to get MY AC Credentials BUT token is not there");
                std::string tokenGenIpcpName = getStringParamFromConfig("TokenGenIPCPName", dm);
                if (tokenGenIpcpName == std::string()){
                        LOG_IPCP_ERR("Missing tokenGenIpcpName parameter configuration!");
                        return -1;
                }
                if (tokenGenIpcpName == my_ipcp_name){
                        LOG_IPCP_DBG("I am the token generator, I have the priviledge to generate my own token!");
                        //generate its own token using same algorithm
                        if (generateTokenForTokenGenerator(auth, con) < 0){
                                LOG_IPCP_ERR("Failed to generate token!");
                                return -1;
                        }
                        //need to store token for future usage
                        my_token = auth.options;
                        TokenPlusSignature_t tokenSign;
                        deserializeTokenPlusSign(auth.options, tokenSign);
                        LOG_IPCP_DBG("Token stored: \n%s", tokenSign.toString().c_str());
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

int SecurityManagerCBACPs::checkTokenValidity(const TokenPlusSignature_t &tokenSign,//const rina::cdap_rib::auth_policy_t & auth,
                                              std::string requestor)
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
                //return -1; //FIXME: need to return
        }
        
        //3. check nbf
        rina::Time currentTime;
        int now = currentTime.get_current_time_in_ms();
        if(token.token_nbf <= now){
                ss << "\n\t 3. Token nbf is valid, continue token validation"<< endl;
                LOG_IPCP_DBG("%s", ss.str().c_str());
                ss.str(std::string());
        }else{
                ss << "\n\t 3. Token cannot not be used yet (now:" << now << ", nbf: "<< token.token_nbf << "), failed token validation" << endl;
                LOG_IPCP_DBG("%s", ss.str().c_str());
                return -1;
        }
        
        //4. check expiration time
        int until = token.token_exp * 3600 * 1000 + token.issued_time;
        if(until >= now){
                ss << "\n\t 4. Token has not expired yet, continue token validation"<< endl;
                LOG_IPCP_DBG("%s", ss.str().c_str());
                ss.str(std::string());
        }else{
                ss << "\n\t 4. Token has expired, failed token validation" << endl;
                LOG_IPCP_DBG("%s", ss.str().c_str());
                return -1;
        }
        
        //5. check signature
        std::string encryptAlgo = getStringParamFromConfig("EncryptAlgo", dm);
        if (encryptAlgo == std::string()){
                LOG_IPCP_ERR("Missing EncryptAlgo parameter configuration!");
                return -1;
        }
        
        if (checkTokenSignature(token, tokenSign.token_sign, 
                                                encryptAlgo) == -1){
                ss << "\n\t 5. Token Signature invalid, failed token validation" << endl;
                LOG_IPCP_DBG("%s", ss.str().c_str());
                return -1; 
        } 
        ss << "\n\t 5. Token Signature valid, token is valid"<< endl;
        LOG_IPCP_DBG("%s", ss.str().c_str());
        return 0;
        
}

void SecurityManagerCBACPs::checkRIBOperation(const rina::cdap_rib::auth_policy_t & auth,
                                          const rina::cdap_rib::con_handle_t & con,
                                          const rina::cdap::cdap_m_t::Opcode opcode,
                                          const std::string obj_name,
                                          rina::cdap_rib::res_info_t& res)
{
#if ACCESS_GRANTED
        res.code_ = rina::cdap_rib::CDAP_SUCCESS;
        return;
#endif
        LOG_IPCP_DBG("checkRIBOperation: Context = %s", 
                     cbac_helpers::operationToString(con, opcode, obj_name).c_str());
        
        std::string requestor = con.dest_.ap_name_;
        if (requestor == std::string()){
                // con dest_ap_name is empty, need a hack: take requestor from token attached
                LOG_IPCP_DBG("Empty requestor name from con_handle, try to retrieve it from token");
        }
        
        if (auth.options.size_ < 0){
                LOG_IPCP_ERR("Empty token field");
                res.code_ = rina::cdap_rib::CDAP_ERROR;
                return;
        }
        //get token
        TokenPlusSignature_t tokenSign;
        deserializeTokenPlusSign(auth.options, tokenSign);
        requestor = tokenSign.token.ipcp_holder_name.processName; // NOTE: better to remove after fixing con_handle empty fields bug
        
        
        /*if (std::find(trusted_ap_name.begin(), trusted_ap_name.end(), requestor) 
                != trusted_ap_name.end()){
                LOG_IPCP_DBG("Requestor is in Trusted list, grant access for any operation");
                res.code_ = rina::cdap_rib::CDAP_SUCCESS;
                return;
        }
        */
        if (checkTokenValidity(tokenSign, requestor) < 0){
                LOG_IPCP_ERR("Invalid Token, Deny request ");
                res.code_ = rina::cdap_rib::CDAP_ERROR;
                return; 
        }
        
        Token_t token = tokenSign.token;
        std::list<Capability_t> tokenCapList = token.token_cap;
            
        for(std::list<Capability_t>::iterator it = tokenCapList.begin();
                it != tokenCapList.end(); ++it) {
                if (it->compare(obj_name, cbac_helpers::opcodeToString(opcode)) ||
                    it->compare("all", "all")){
                        LOG_IPCP_DBG("Request is included in requestor capability; Grant access");
                        res.code_ = rina::cdap_rib::CDAP_SUCCESS;
                        return;
                }
        }
        
        LOG_IPCP_DBG("Request in NOT in requestor capability; Deny request");
        res.code_ = rina::cdap_rib::CDAP_ERROR;
        return;   
}

#if 0
void SecurityManagerCBACPs::checkRIBOperationOld(const rina::cdap_rib::auth_policy_t & auth,
                                          const rina::cdap_rib::con_handle_t & con,
                                          const rina::cdap::cdap_m_t::Opcode opcode,
                                          const std::string obj_name,
                                          rina::cdap_rib::res_info_t& res)
{
        /*(void) auth;
        (void) con;
        (void) opcode;
        (void) obj_name;
        typedef struct connection_handler {
        unsigned int port_id;
        cdap_dest_t cdap_dest;
        int abs_syntax;
        ep_info_t src_;
        ep_info_t dest_;
        auth_policy_t auth_;
        vers_info_t version_;

        //if the message was forwarded by the IPCM
        //this is the sequence number (otherwise 0)
        unsigned int fwd_mgs_seqn;

        connection_handler() {
                cdap_dest = CDAP_DEST_PORT;
                abs_syntax = 0;
                port_id = 0;
                fwd_mgs_seqn = 0;
        };
        } con_handle_t;

        */
#if ACCESS_GRANTED
        res.code_ = rina::cdap_rib::CDAP_SUCCESS;
#else
        LOG_IPCP_DBG("checkRIBOperation: Context = %s", 
                     cbac_helpers::operationToString(con, opcode, obj_name).c_str());
        
        std::string requestor = con.dest_.ap_name_;
        if (requestor == std::string()){
                LOG_IPCP_ERR("Empty requestor name field! hack this for the moment");
                if (auth.options.size_ < 0){
                        LOG_IPCP_ERR("neither con_handle information nor token, hack cannot work");
                        res.code_ = rina::cdap_rib::CDAP_ERROR;
                        return;
                }
                //get token
                TokenPlusSignature_t tokenSign;
                deserializeTokenPlusSign(auth.options, tokenSign);
                requestor = tokenSign.token.ipcp_holder_name.processName; // FIXME: to remove after fixing con_handle empty fields bug
                
                // res.code_ = rina::cdap_rib::CDAP_ERROR;
                //return; //FIXME: need to return, to fix after fixing con_handle empty fields bug
        }
        
        if (std::find(trusted_ap_name.begin(), trusted_ap_name.end(), requestor) 
                != trusted_ap_name.end()){
                LOG_IPCP_DBG("Requestor is in Trusted list, grant access for any operation");
                res.code_ = rina::cdap_rib::CDAP_SUCCESS;
                return;
        }
        
        //FIXME: clean up with the previous hack
        TokenPlusSignature_t tokenSign;
        deserializeTokenPlusSign(auth.options, tokenSign);
        if (checkTokenValidity(tokenSign, requestor) < 0){
                LOG_IPCP_ERR("Invalid Token, NO access");
                res.code_ = rina::cdap_rib::CDAP_ERROR;
                return;   
        }
        
        Token_t token = tokenSign.token;    
        std::list<Capability_t> tokenCapList = token.token_cap;
            
        for(std::list<Capability_t>::iterator it = tokenCapList.begin();
                it != tokenCapList.end(); ++it) {
                if (it->compare(obj_name, cbac_helpers::opcodeToString(opcode)) ||
                    it->compare("all", "all")){
                        LOG_IPCP_DBG("Request is included in requestor capability; Grant access");
                        res.code_ = rina::cdap_rib::CDAP_SUCCESS;
                        return;
                }
        }
        
        LOG_IPCP_DBG("Request in NOT in requestor capability; Continue check");
        LOG_IPCP_DBG("Loading profiles...");

        IPCPProfile_t myProfile;
        DIFProfile_t myDIFProfile; 
        loadProfilesByName(rina::ApplicationProcessNamingInformation(
                        my_ipcp_name, string()), myProfile, my_dif_name, myDIFProfile);
        
        IPCPProfile_t peerProfile;
        DIFProfile_t peerDIFProfile;
        loadProfilesByName(tokenSign.token.ipcp_holder_name, peerProfile, my_dif_name, peerDIFProfile);
        
//#if 1 //hack_paper
//        if (requestor == "E1"){
                //res.code_ = rina::cdap_rib::CDAP_SUCCESS;
                //return;
        //}
//#endif
        stringstream ss;
        switch(opcode) {
                case rina::cdap::cdap_m_t::M_START:
                        if (auth.options.size_ == 0){
                                ss << "START operation : empty token field; NO access" << endl;
                                LOG_IPCP_ERR("%s", ss.str().c_str());
                                res.code_ = rina::cdap_rib::CDAP_ERROR;
                        }
                        else{
                                ss << "START operation : token field here; Grant access" << endl;
                                LOG_IPCP_DBG("%s", ss.str().c_str());
                                res.code_ = rina::cdap_rib::CDAP_SUCCESS;
                        }                        
                        break;
                case rina::cdap::cdap_m_t::M_STOP:
                       if (auth.options.size_ == 0){
                                ss << "STOP operation : empty token field; NO access" << endl;
                                LOG_IPCP_ERR("%s", ss.str().c_str());
                                res.code_ = rina::cdap_rib::CDAP_ERROR;
                        }
                        else{
                                ss << "STOP operation : token field here; Grant access" << endl;
                                LOG_IPCP_DBG("%s", ss.str().c_str());
                                res.code_ = rina::cdap_rib::CDAP_SUCCESS;
                        }                        
                        break;
                case rina::cdap::cdap_m_t::M_CONNECT:
                        ss << "OK!"<< endl; 
                        LOG_IPCP_DBG("%s", ss.str().c_str());
                        break;
                case rina::cdap::cdap_m_t::M_WRITE:
                        ss << "OK!"<< endl;
                        LOG_IPCP_DBG("%s", ss.str().c_str());
                        break;
                case rina::cdap::cdap_m_t::M_READ:
                        ss << "OK!"<< endl;
                        LOG_IPCP_DBG("%s", ss.str().c_str());
                        break;
                case rina::cdap::cdap_m_t::M_CREATE:
                        ss << "OK!"<< endl;
                        LOG_IPCP_DBG("%s", ss.str().c_str());
                        break;
                default:
                        ss << "Unrecognized operation %s!"<< cbac_helpers::opcodeToString(opcode).c_str() << endl;
                        LOG_IPCP_ERR("%s", ss.str().c_str());
                        break;
        }
        //return;
            
        
        /*
        if (auth.options.size_ == 0){
                LOG_IPCP_ERR("Check RIB operation: empty token field");
                res.code_ = rina::cdap_rib::CDAP_SUCCESS; //CDAP_ERROR;
                return;
        }
        
        TokenPlusSignature_t tokenSign;
        deserializeTokenPlusSign(auth.options, tokenSign);
        
        LOG_IPCP_DBG("Check RIB operation: %s",
                        tokenSign.toString().c_str());
                     
        
        LOG_IPCP_DBG("Check RIB operation: OK");
        res.code_ = rina::cdap_rib::CDAP_SUCCESS;
        */
#endif
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

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

#include "../../ipcp-logging.h"

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <list>
#include "ipcp/components.h"
#include <librina/json/json.h>
// #include <librina/timer.h>

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
        std::string ipcp_type;
        std::string ipcp_group;
        std::list<RIBProfile_t> ipcp_rib_profiles;
} IPCPProfile_t;


  /* The profile of a DIF */
typedef struct DIFProfile {
        rina::ApplicationProcessNamingInformation dif_name;
        std::string dif_type; 
        std::string dif_group;
} DIFProfile_t;

//----------------------------------

/* One AC rule */
typedef struct Rule {
        std::string ipcp_type;
        std::string dif_type;
        std::string member_ipcp_type;
        std::string member_dif_type;
       
} Rule_t;

//----------------------------------

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

//----------------------------------
//-----------------------------------

typedef struct Capability{
        std::string ressource;
        std::string operation;
        Capability() {}
        Capability(std::string res, std::string oper) : ressource(res), operation(oper) {}
        string toString() const
        {
                stringstream ss;
                
                ss << "\t Ressource :" << ressource.c_str() << endl; 
                ss << "\t Operation :" << operation.c_str() << endl;
                return ss.str();
        }
        bool compare(std::string objname, std::string oper){
            if(objname == ressource && oper == operation)
                return true;
            std::size_t foundObj = ressource.find(objname);
            std::size_t foundOper = operation.find(oper);
            if (foundObj != std::string::npos && foundOper != std::string::npos)
                return true;
            return false;
        }
        
} Capability_t;

typedef struct Token{
        unsigned short token_id;
        unsigned short ipcp_issuer_id;
        rina::ApplicationProcessNamingInformation ipcp_holder_name; 
        rina::ApplicationProcessNamingInformation ipcp_issuer_name;
        std::list<std::string> audience;
        int issued_time;
        int token_nbf;
        int token_exp;
        std::list<Capability> token_cap;
        //std::string token_sign;
        string toString() const
        {
                stringstream ss;
                
                ss << "\nToken id:" << token_id << endl;
                ss << "Token issuer_id:" << ipcp_issuer_id << endl;
                ss << "Token ipcp_issuer_name:" << ipcp_issuer_name.toString() << endl;
                ss << "Token ipcp_holder_name:" << ipcp_holder_name.toString() << endl;
                ss << "Token audience:" << endl;
                for (list<std::string>::const_iterator it =
                                audience.begin(); 
                                it != audience.end(); it++) {
                        ss << "\t IPCP: "<< it->c_str() << endl;
                }
                ss << "Token issued_time:" << issued_time << endl;
                ss << "Token token_nbf:" << token_nbf << endl;
                ss << "Token token_exp:" << token_exp << endl;
                int i = 1;
                for (list<Capability_t>::const_iterator it =
                                token_cap.begin(); 
                                it != token_cap.end(); it++) {
                        ss << "Capability " << i << ":"<< endl;
                        ss << it->toString() << endl;
                        i ++;
                                    
                }
                return ss.str();
        }
} Token_t;

typedef struct TokenPlusSignature{

        Token_t token;
        rina::UcharArray token_sign;
        string toString()
        {
                stringstream ss;
                ss << "\nToken :"<< token.toString() << endl;
                ss << "Token Signature: " << token_sign.toString().c_str() << endl;
                return ss.str();
                
        }
} TokenPlusSignature_t;

//------------------------


class ProfileParser {
public:
        ProfileParser() {};
        bool parseProfile(const std::string fileName);
        virtual ~ProfileParser() {};
        bool getDIFProfileByName(const rina::ApplicationProcessNamingInformation&, DIFProfile_t&);
        bool getIPCPProfileByName(const rina::ApplicationProcessNamingInformation&, IPCPProfile_t&);
        bool compareRuleToProfile(Rule_t &, std::string, std::string, std::string);
        bool getCapabilityByProfile(const std::string fileName, std::list<Capability_t> & capList, 
                               std::string ipcp_type, std::string ipcp_group,
                               std::string dif_type, std::string dif_group,
                               std::string member_ipcp_type, std::string member_ipcp_group);
        bool getAuthPolicyName(std::string fileName, std::string& name);
        string toString() const; 
private:
  
        std::list<DIFProfile_t> difProfileList;
        std::list<IPCPProfile_t> ipcpProfileList;
        std::list<RIBProfile_t> ribProfileList;
        std::list<Rule_t> ruleList;
};


//--------------------------------
class ACRulesManager{
private:
        ProfileParser parser_;
        ACRulesManager();
        void compareRules();
};

//--------------------------------
class AccessControl{
public:
        AccessControl();
        bool checkJoinDIF(ProfileParser  *, 
                          std::string fileName, 
                          const rina::ApplicationProcessNamingInformation& my_ipcp_name, 
                          const rina::ApplicationProcessNamingInformation& my_dif_name, 
                          const rina::ApplicationProcessNamingInformation& newMember_ipcp_name,  
                          ac_res_info_t&, std::list<Capability_t>&);
                                 
        void generateToken(const rina::ApplicationProcessNamingInformation &,
                           unsigned short, DIFProfile_t&, IPCPProfile_t&, 
                           rina::cdap_rib::auth_policy_t &, rina::SSH2SecurityContext*,
                           std::string, std::list<Capability_t>&);
        void generateTokenSignature(Token_t &token, std::string encrypt_alg, 
                                   RSA * my_private_key,  rina::UcharArray &signature);
        virtual ~AccessControl() {}
        static const std::string IPCP_DIF_FROM_DIFFERENT_GROUPS;
};

//-----------------------------------
class SecurityManagerCBACPs: public IPCPSecurityManagerPs {
public:
        SecurityManagerCBACPs(IPCPSecurityManager * dm);
        int loadProfiles();
        int loadProfilesByName(const rina::ApplicationProcessNamingInformation &ipcpProfileHolder,
                               IPCPProfile_t &requestedIPCPProfile,
                               const rina::ApplicationProcessNamingInformation &difProfileHolder, 
                               DIFProfile_t &requestedDIFProfile);
        int isAllowedToJoinDAF(const rina::cdap_rib::con_handle_t & con,
                               const rina::Neighbor& newMember,
                               rina::cdap_rib::auth_policy_t & auth);
        int storeAccessControlCreds(const rina::cdap_rib::auth_policy_t & auth,
                                    const rina::cdap_rib::con_handle_t & con);
        int getAccessControlCreds(rina::cdap_rib::auth_policy_t & auth,
                                  const rina::cdap_rib::con_handle_t & con);
        int checkTokenValidity(const TokenPlusSignature_t &tokenSign,
        		       const std::string& requestor,
			       const std::string& keyStorePath);
        int checkTokenSignature(const Token_t &token, 
        			const rina::UcharArray & signature,
				const std::string& encrypt_alg,
				const std::string& keystorePath);
        RSA* loadTokenGeneratorPublicKey(const std::string& tokenGenIPCPName,
        				 const std::string& keyStorePath);
        void checkRIBOperation(const rina::cdap_rib::auth_policy_t & auth,
                              const rina::cdap_rib::con_handle_t & con,
                              const rina::cdap::cdap_m_t::Opcode opcode,
                              const std::string obj_name,
                              rina::cdap_rib::res_info_t& res);
        bool acceptFlow(const configs::Flow& newFlow);
        int set_policy_set_param(const std::string& name,
                        	 const std::string& value);
        virtual ~SecurityManagerCBACPs() {}
        
private:
        // Data model of the security manager component.
        IPCPSecurityManager * dm;
        int max_retries;
        AccessControl * access_control_;
        ProfileParser * profile_parser_;
        unsigned short my_ipcp_id;
        std::string my_ipcp_name;
        rina::ApplicationProcessNamingInformation my_dif_name;
        std::list<std::string> trusted_ap_name;
        std::map<int, std::string> authPolicyPerCon;
        //std::map<std::string, rina::ser_obj_t> token_sign_per_ipcp;
        rina::ser_obj_t my_token;
        //std::map<rina::cdap_rib::con_handle_t, TokenPlusSignature_t*> token_sign_per_ipcp;
        rina::Lockable lock;
        int generateTokenForTokenGenerator(rina::cdap_rib::auth_policy_t &, const rina::cdap_rib::con_handle_t &);
};


}   // namespace rinad

//#define IPCP_MODULE "security-manager-ps-cbac"
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
        rina::ApplicationProcessNamingInformation ipcp_difName; //XXX check if used
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

class ProfileParser {
public:
        ProfileParser() {};
        bool parseProfile(const std::string fileName);
        virtual ~ProfileParser() {};
        bool getDIFProfileByName(const rina::ApplicationProcessNamingInformation&, DIFProfile_t&);
        bool getIPCPProfileByName(const rina::ApplicationProcessNamingInformation&, IPCPProfile_t&);
        string toString() const; 
private:
  
        std::list<DIFProfile_t> difProfileList;
        std::list<IPCPProfile_t> ipcpProfileList;
        std::list<RIBProfile_t> ribProfileList;
};

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
        
} Capability_t;

typedef struct Token{
        unsigned short token_id;
        unsigned short ipcp_issuer_id;
        rina::ApplicationProcessNamingInformation ipcp_holder_name; // TODO: may be replace by ipcp id?/*unsigned short*/ 
        std::string audience;
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
                ss << "Token ipcp_holder_name:" << ipcp_holder_name.toString() << endl;
                ss << "Token audience:" << audience.c_str() << endl;
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
                //ss << "Token token_sign:" << token_sign << endl;                      
                return ss.str();
        }
} Token_t;

typedef struct TokenPlusSignature{

        Token_t token;
        std::string token_sign;
        string toString() const
        {
                stringstream ss;
                ss << "\nToken :"<< token.toString() << endl;
                ss << "Token Signature: " << token_sign << endl;
                return ss.str();
                
        }
} TokenPlusSignature_t;

//--------------------------------
class AccessControl{
public:
        AccessControl();
        bool checkJoinDIF(DIFProfile_t&, IPCPProfile_t&, ac_res_info_t&);
        std::list<Capability_t> computeCapabilities(DIFProfile_t&, IPCPProfile_t&);
        void generateToken(unsigned short, DIFProfile_t&, IPCPProfile_t&, rina::cdap_rib::auth_policy_t & auth);
        void generateTokenSignature(Token_t &token, std::string encrypt_alg, 
                                   RSA * my_private_key,  rina::UcharArray &signature);
        virtual ~AccessControl() {}
        static const std::string IPCP_DIF_FROM_DIFFERENT_GROUPS;
};





//-----------------------------------
class SecurityManagerCBACPs: public IPCPSecurityManagerPs {
public:
        SecurityManagerCBACPs(IPCPSecurityManager * dm);
//         bool isAllowedToJoinDIF(const rina::Neighbor& newMember); 
                                //const rina::ApplicationProcessNamingInformation, std::string);
        int isAllowedToJoinDAF(const rina::Neighbor& newMember,
                               rina::cdap_rib::auth_policy_t & auth);
        int storeAccessControlCreds(const rina::cdap_rib::auth_policy_t & auth,
                                    const rina::cdap_rib::con_handle_t & con);
        int getAccessControlCreds(rina::cdap_rib::auth_policy_t & auth,
                                  const rina::cdap_rib::con_handle_t & con);
        void checkRIBOperation(const rina::cdap_rib::auth_policy_t & auth,
                              const rina::cdap_rib::con_handle_t & con,
                              const rina::cdap::cdap_m_t::Opcode opcode,
                              const std::string obj_name,
                              rina::cdap_rib::res_info_t& res);
        bool acceptFlow(const configs::Flow& newFlow);
        int set_policy_set_param(const std::string& name,
                        const std::string& value);
        // bool generateToken(const rina::Neighbor&, Token_t &);
        // bool getToken(const rina::Neighbor&, rina::ser_obj_t&);
        virtual ~SecurityManagerCBACPs() {}
        
private:
        // Data model of the security manager component.
        IPCPSecurityManager * dm;
        int max_retries;
        AccessControl * access_control_;
        unsigned short my_ipcp_id;
        rina::ApplicationProcessNamingInformation my_dif_name;
        std::map<std::string, TokenPlusSignature_t*> token_sign_per_ipcp;
};


}   // namespace rinad

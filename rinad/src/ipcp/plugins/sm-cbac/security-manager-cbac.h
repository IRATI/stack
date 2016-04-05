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
        std::string token_sign;
} Token_t;


class AccessControl{
public:
        AccessControl();
        bool checkJoinDIF(DIFProfile_t&, IPCPProfile_t&, ac_res_info_t&);
        std::list<Capability_t> computeCapabilities(DIFProfile_t&, IPCPProfile_t&);
        void generateToken(unsigned short, DIFProfile_t&, IPCPProfile_t&, Token_t &);
        virtual ~AccessControl() {}
        static const std::string IPCP_DIF_FROM_DIFFERENT_GROUPS;
};





//-----------------------------------
class SecurityManagerCBACPs: public ISecurityManagerPs {
public:
        SecurityManagerCBACPs(IPCPSecurityManager * dm);
        bool isAllowedToJoinDIF(const rina::Neighbor& newMember); 
                                //const rina::ApplicationProcessNamingInformation, std::string);
        bool acceptFlow(const configs::Flow& newFlow);
        int set_policy_set_param(const std::string& name,
                        const std::string& value);
        bool generateToken(const rina::Neighbor&, Token_t &);
        bool getToken(const rina::Neighbor&, rina::ser_obj_t&);
        virtual ~SecurityManagerCBACPs() {}
        
private:
        // Data model of the security manager component.
        IPCPSecurityManager * dm;
        int max_retries;
        AccessControl * access_control_;
        unsigned short my_ipcp_id;
        rina::ApplicationProcessNamingInformation my_dif_name;
};


}   // namespace rinad

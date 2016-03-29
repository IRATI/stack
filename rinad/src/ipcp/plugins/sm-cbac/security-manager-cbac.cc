#define IPCP_MODULE "security-manager-ps-cbac"
#include "../../ipcp-logging.h"

#include <string>
#include <sstream>

#include "ipcp/components.h"


namespace rinad {

//----------------------------------
  
enum IPCPType{
  
}

enum DIFType{
}

enum DIFGroup{
}

enum RIBType{
}

enum RIBGroup{
}


/* The profile of a RIB */
struct RIBProfile {
	enum RIBType rib_type;
	enum RIBGroup rib_group;
};

/* The profile of an IPC Process */
struct IPCProfile {

        rina::ApplicationProcessNamingInformation ipcp_name;
        rina::ApplicationProcessNamingInformation ipcp_difName;
	enum IPCPType ipcp_type;
	enum IPCPGroup ipcp_group;
	RIBProfile ipcp_rib_profile;
	//DIFProfile ipcp_dif_profile;
};


  /* The profile of a DIF */
struct DIFProfile {
        rina::ApplicationProcessNamingInformation difName;
	enum DIFType dif_type; //FIXME: merge/replace (?) with dif_type_ in DIFInformation
	enum DIFGroup dif_group;
};


//----------------------------------

class ProfileParser {
public:
  ProfileParser();
  std::string parseProfile(const std::string fileName);
  virtual ~ProfileParser() {}
  DIFProfile getDIFProfileByName(rina::ApplicationProcessNamingInformation difName);
  IPCPProfile getIPCPProfileByName(rina::ApplicationProcessNamingInformation ipcpName);
  
private:
  
  std::list<DIFProfile> difProfileList;
  std::list<IPCPProfile> ipcpProfileList;
  std::list<RIBProfile> ribProfileList;
}

ProfileParser()::ProfileParser()
{
  
}

ProfileParser:parseProfile(const std::string fileName)
{
	// Parse AC config file with jsoncpp
	Json::Value  root;
	Json::Reader reader;
	ifstream     file;

	file.open(fileName.c_str(), std::ifstream::in);
	if (file.fail()) {
		LOG_ERR("Failed to open AC profile file");
		return 0;
	}

	if (!reader.parse(file, root, false)) {
		LOG_ERR("Failed to parse configuration");

		// FIXME: Log messages need to take string for this to work
		cout << "Failed to parse JSON" << endl
			  << reader.getFormatedErrorMessages() << endl;

		return 0;
	}

	file.close();

	// really parse
	
	Json::Value dif_profiles = root["DIFProfiles"];
	for (unsigned int i = 0; i < dif_profiles.size(); i++) {
		DIFProfile dif;
		dif.dif_name = dif_profiles|i].get("difName",    string()).asString();
		dif.dif_type = dif_profiles|i].get("difType",    string()).asString();
		dif.dif_group = dif_profiles|i].get("difGroup",    string()).asString();
		difProfileList.push_back(dif);
	}
	
	
	Json::Value ipcp_profiles = root["IPCPProfiles"];
	for (unsigned int i = 0; i < ipcp_profiles.size(); i++) {
		IPCProfile ipc;
		ipc.ipc_name = ipcp_profiles|i].get("apName",    string()).asString();
		ipc.ipc_type = ipcp_profiles|i].get("ipcpType",    string()).asString();
		ipc.ipc_group = ipcp_profiles|i].get("ipcpGroup",    string()).asString();
		
		Json::Value rib_profile = ipcp_profiles|i]["ipcpRibProfile"]
		if (rib_profile != 0){
			//parse rib profile of current ipcp
			RIBProfile ribProfile;
			ribProfile.ribName = rib_profile.get("ribName", string()).asString();
			ribProfile.ribGroup = rib_profile.get("ribGroup", string()).asString();
			ribProfile.ribType = rib_profile.get("ribType", string()).asString();
			ipcp.ipcp_rib_profile = ribProfile;
			ribProfileList.push_back(rib);
		}
		ipcpProfileList.push_back(ipcp);
				
	}
	
}

bool getDIFProfileByName(rina::ApplicationProcessNamingInformation difName,
					DIFProfile&  result)
{
	for (list<DIFProfile>::const_iterator it = difProfileList.begin();
					it != difProfileList.end(); it++) {
		if (it->difName == difName) {
			result = *it;
			return true;
		}
	}

	return false;
}

bool getIPCPProfileByName(rina::ApplicationProcessNamingInformation ipcpName,
					IPCPProfile&  result)
{
	for (list<IPCPProfile>::const_iterator it = ipcpProfileList.begin();
					it != ipcpProfileList.end(); it++) {
		if (it->apNname == ipcpName) {
			result = *it;
			return true;
		}
	}

	return false;
}

//-----------------------------------

class SecurityManagerCBACPs: public ISecurityManagerPs {
public:
	SecurityManagerCBACPs(IPCPSecurityManager * dm);
	bool isAllowedToJoinDIF(const rina::Neighbor& newMember);
	bool acceptFlow(const configs::Flow& newFlow, std::string profileFile);
	int set_policy_set_param(const std::string& name,
			const std::string& value);
	virtual ~SecurityManagerCBACPs() {}

private:
	// Data model of the security manager component.
	IPCPSecurityManager * dm;

	int max_retries;
};

SecurityManagerCBACPs::SecurityManagerCBACPs(IPCPSecurityManager * dm_)
						: dm(dm_)
{
}


bool SecurityManagerCBACPs::isAllowedToJoinDIF(const rina::Neighbor& newMember, 
						 rina::ApplicationProcessNamingInformation difName,
						 std::string profileFile)
{
	
	// Read Profile From AC Profile Store
	ProfileParser profileParser = new ProfileParser();
	profileParser.parseProfile(profileFile);

	IPCPProfile newMemberProfile;
	
	
	if (!profileParser.getIPCPProfileByName(newMember.name, &newMemberProfile)){
	  LOG_IPCP_DBG("No Profile for this newMember %s; not allowing it to join DIF!" ,
		       newMember.name-.processName.c_str);
	  return false;
	}
	

	DIFProfile difProfile; 
	if (!profileParser.getDIFProfileByName(difName, &difProfile)){
	  LOG_IPCP_DBG("No Profile for my DIF, not allowing IPCProcess %s to join DIF!",
	    newMember.name_.processName.c_str());
	  return false;
	}
	
	
	// AC join algorithm
	
	LOG_IPCP_DBG("Allowing IPC Process %s to join the DIF",
		     newMember.name_.processName.c_str());
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

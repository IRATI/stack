/*
 * DIF Allocator
 *
 *    Eduard Grasa     <eduard.grasa@i2cat.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

#include <dirent.h>
#include <errno.h>
#include <poll.h>
#include <fstream>
#include <sstream>

#define RINA_PREFIX     "ipcm.dif-allocator"

#include <librina/logs.h>
#include <librina/json/json.h>
#include <librina/rib_v2.h>
#include <librina/security-manager.h>
#include <rina/api.h>

#include "common/encoder.h"
#include "configuration.h"
#include "dif-allocator.h"

using namespace std;

namespace rinad {

// Class DIF Allocator
const std::string DIFAllocator::STATIC_DIF_ALLOCATOR_FILE_NAME = "da.map";

void DIFAllocator::parse_config(DIFAllocatorConfig& da_config,
		  	        const rinad::RINAConfiguration& config)
{
	Json::Reader    reader;
	Json::Value     root;
        Json::Value     da_json_conf;
	std::ifstream   fin;

	fin.open(config.configuration_file.c_str());
	if (fin.fail()) {
		LOG_ERR("Failed to open config file");
		return populate_with_default_conf(da_config,
						  config.configuration_file);
	}

	if (!reader.parse(fin, root, false)) {
		LOG_ERR("Failed to parse JSON configuration %s",
			 reader.getFormatedErrorMessages().c_str());
		return populate_with_default_conf(da_config,
						  config.configuration_file);
	}

	fin.close();

	da_json_conf = root["difallocator"];
        if (da_json_conf == 0) {
        	LOG_INFO("No specific DIF Allocator config found, using default");
        	return populate_with_default_conf(da_config,
        					  config.configuration_file);
        }

        da_config.type = da_json_conf.get("type", da_config.type).asString();
        if (da_config.type != StaticDIFAllocator::TYPE &&
        		da_config.type != DynamicDIFAllocator::TYPE) {
        	LOG_ERR("Unrecognized DIF Allocator type: %s, using default",
        		 da_config.type.c_str());
        	return populate_with_default_conf(da_config,
        				  	  config.configuration_file);
        }

        Json::Value daf_name = da_json_conf["dafName"];
        if (daf_name != 0) {
        	da_config.daf_name.processName = daf_name.get("processName",
        						       da_config.daf_name.processName).asString();
        }

        Json::Value dap_name = da_json_conf["dapName"];
        if (dap_name != 0) {
        	da_config.dap_name.processName = dap_name.get("processName",
        						       da_config.dap_name.processName).asString();
        	da_config.dap_name.processInstance = dap_name.get("processInstance",
        							   da_config.dap_name.processInstance).asString();
        }

        Json::Value enrollments = da_json_conf["enrollments"];
        if (enrollments != 0) {
        	for (unsigned i = 0; i< enrollments.size(); i++) {
        		rina::Neighbor neigh;
        		neigh.name_.processName = enrollments[i].get("processName",
        							      neigh.name_.processName).asString();
        		neigh.name_.processInstance = enrollments[i].get("processInstance",
        								 neigh.name_.processInstance).asString();
        		neigh.supporting_dif_name_.processName = enrollments[i].get("difName",
        									    neigh.supporting_dif_name_.processName).asString();
        		da_config.enrollments.push_back(neigh);
        	}
        }

        Json::Value joinableDIFs = da_json_conf["joinableDIFs"];
        if (joinableDIFs != 0) {
        	for (unsigned i = 0; i< joinableDIFs.size(); i++) {
        		NeighborData neigh;

        		neigh.difName.processName = joinableDIFs[i].get("difName",
        							        neigh.difName.processName).asString();
        		neigh.apName.processName = joinableDIFs[i].get("ipcpPn",
        							       neigh.apName.processName).asString();
        		neigh.apName.processInstance = joinableDIFs[i].get("ipcpPi",
        								   neigh.apName.processInstance).asString();
        		da_config.joinable_difs.push_back(neigh);
        	}
        }

        Json::Value parameters = da_json_conf["parameters"];
        if (parameters != 0) {
        	for (unsigned i = 0; i< parameters.size(); i++) {
        		rina::Parameter param;

        		param.name = parameters[i].get("name", param.name).asString();
        		param.value = parameters[i].get("value", param.value).asString();
        		da_config.parameters.push_back(param);
        	}
        }

        //TODO, parse security configuration
}

void DIFAllocator::populate_with_default_conf(DIFAllocatorConfig& da_config,
					      const std::string& config_file)
{
	rina::Parameter folder_name_parm;

	da_config.type = StaticDIFAllocator::TYPE;
	da_config.daf_name.processName = "da-default";
	da_config.dap_name.processName = "test-system";
	da_config.dap_name.processInstance = "1";

	folder_name_parm.value = config_file;
	da_config.parameters.push_back(folder_name_parm);

	return;
}

DIFAllocator * DIFAllocator::create_instance(const rinad::RINAConfiguration& config,
					     IPCManager_ * ipcm)
{
	DIFAllocator * result;
	DIFAllocatorConfig da_config;

	DIFAllocator::parse_config(da_config, config);

	if (da_config.type == StaticDIFAllocator::TYPE) {
		result = new StaticDIFAllocator();
		result->set_config(da_config);
	} else if (da_config.type == DynamicDIFAllocator::TYPE) {
		result = new DynamicDIFAllocator(da_config.dap_name, ipcm);
		result->set_config(da_config);
	} else {
		result = NULL;
	}

	return result;
}

void DIFAllocator::get_ipcp_name(rina::ApplicationProcessNamingInformation& ipcp_name,
	   	   	   	 const std::string& dif_name,
				 const std::list<NeighborData> joinable_difs)
{
	std::list<NeighborData>::const_iterator itr;

	for (itr = joinable_difs.begin();
			itr != joinable_difs.end(); ++itr) {
		if (itr->difName.processName == dif_name) {
			ipcp_name.processName = itr->apName.processName;
			ipcp_name.processInstance = itr->apName.processInstance;

			return;
		}
	}
}

int DIFAllocator::generate_ipcp_name(rina::ApplicationProcessNamingInformation& ipcp_name,
		                     const std::string& dif_name)
{
	std::stringstream ss;

	if (sys_name.processName == "") {
		LOG_DBG("No system name provided, cannot generate IPCP name");
		return -1;
	}

	ss << sys_name.processName << "." << dif_name;

	ipcp_name.processName = ss.str();
	ipcp_name.processInstance = "1";

	return 0;
}

int DIFAllocator::generate_vpn_name(rina::ApplicationProcessNamingInformation& app_name,
		                    const std::string& vpn_name)
{
	std::stringstream ss;

	if (sys_name.processName == "") {
		LOG_DBG("No system name provided, cannot generate VPN app name");
		return -1;
	}

	ss << sys_name.processName << "." << vpn_name;

	app_name.processName = ss.str();
	app_name.entityName = RINA_IP_FLOW_ENT_NAME;

	return 0;
}

const std::string StaticDIFAllocator::TYPE = "static-dif-allocator";

StaticDIFAllocator::StaticDIFAllocator() : DIFAllocator()
{
}

StaticDIFAllocator::~StaticDIFAllocator()
{
}

int StaticDIFAllocator::set_config(const DIFAllocatorConfig& da_config)
{
	std::string folder_name;
	rina::Parameter folder_name_parm;
	stringstream ss;
	std::string::size_type pos;

	if (da_config.parameters.size() == 0) {
		LOG_ERR("Could not find config folder name, wrong config");
		return -1;
	}

	folder_name_parm = da_config.parameters.front();
	pos = folder_name_parm.value.rfind("/");
	if (pos == std::string::npos) {
		ss << ".";
	} else {
		ss << folder_name_parm.value.substr(0, pos);
	}
	folder_name = ss.str();
	ss << "/" << STATIC_DIF_ALLOCATOR_FILE_NAME;
	fq_file_name = ss.str();

	LOG_INFO("DIF Directory file: %s", fq_file_name.c_str());

	joinable_difs = da_config.joinable_difs;

	//load current mappings
	if (!parse_app_to_dif_mappings(fq_file_name, dif_directory)) {
		LOG_ERR("Problems loading initial directory");
		return -1;
	}

	print_directory_contents();

	return 0;
}

da_res_t StaticDIFAllocator::lookup_dif_by_application(const rina::ApplicationProcessNamingInformation& app_name,
                			     	       rina::ApplicationProcessNamingInformation& result,
						       std::list<std::string>& supporting_difs)
{
	rina::ReadScopedLock g(directory_lock);
        string encoded_name = app_name.getEncodedString();
        std::list< std::pair<std::string, std::string> >::iterator it;
        std::list<std::string>::const_iterator jt;

        for (it = dif_directory.begin(); it != dif_directory.end(); ++it) {
        	if (it->first == encoded_name) {
        		result.processName = it->second;
        		find_supporting_difs(supporting_difs, it->second);
        		return DA_SUCCESS;
        	}
        }

        return DA_FAILURE;
}

void StaticDIFAllocator::find_supporting_difs(std::list<std::string>& supporting_difs,
        				      const std::string& dif_name)
{
	std::list< std::pair<std::string, std::string> >::iterator it;

	for (it = dif_directory.begin(); it != dif_directory.end(); ++it) {
		if (it->first.compare(0, dif_name.size(), dif_name) == 0)
			supporting_difs.push_back(it->second);
	}
}

void StaticDIFAllocator::app_registered(const rina::ApplicationProcessNamingInformation & app_name,
			    	        const std::string& dif_name)
{
	//Ignore
}

void StaticDIFAllocator::app_unregistered(const rina::ApplicationProcessNamingInformation & app_name,
			      	          const std::string& dif_name)
{
	//Ignore
}

void StaticDIFAllocator::assigned_to_dif(const std::string& dif_name)
{
	//Ignore
}

void StaticDIFAllocator::update_directory_contents()
{
	rina::WriteScopedLock g(directory_lock);
	dif_directory.clear();
	if (!parse_app_to_dif_mappings(fq_file_name, dif_directory)) {
	    LOG_ERR("Problems while updating DIF Allocator Directory!");
        } else {
	    LOG_DBG("DIF Allocator Directory updated!");
	    print_directory_contents();
        }
}

void StaticDIFAllocator::list_da_mappings(std::ostream& os)
{
	std::list< std::pair<std::string, std::string> >::iterator it;

	rina::ReadScopedLock g(directory_lock);

	for (it = dif_directory.begin(); it != dif_directory.end(); ++it) {
		os << "Application name: " << it->first
		   << "; DIF name: " << it->second << std::endl;
	}
}

void StaticDIFAllocator::get_ipcp_name_for_dif(rina::ApplicationProcessNamingInformation& ipcp_name,
			   	   	       const std::string& dif_name)
{
	get_ipcp_name(ipcp_name, dif_name, joinable_difs);
}

void StaticDIFAllocator::print_directory_contents()
{
	std::list< std::pair<std::string, std::string> >::iterator it;
	std::stringstream ss;

	ss << "Application to DIF mappings" << std::endl;
	for (it = dif_directory.begin(); it != dif_directory.end(); ++it) {
		ss << "Application name: " << it->first
		   << "; DIF name: " << it->second << std::endl;
	}

	LOG_DBG("%s", ss.str().c_str());
}

// Class AppToDIFEntriesRIBObject
class AppToDIFEntriesRIBObject: public rina::rib::RIBObj {
public:
	AppToDIFEntriesRIBObject(DynamicDIFAllocator * dda_);
	const std::string& get_class() const {
		return class_name;
	};

	void create(const rina::cdap_rib::con_handle_t &con,
		    const std::string& fqn,
		    const std::string& class_,
		    const rina::cdap_rib::filt_info_t &filt,
		    const int invoke_id,
		    const rina::ser_obj_t &obj_req,
		    rina::ser_obj_t &obj_reply,
		    rina::cdap_rib::res_info_t& res);

	const static std::string class_name;
	const static std::string object_name;

private:
	DynamicDIFAllocator * dda;
};

const std::string AppToDIFEntriesRIBObject::class_name = "AppDIFEntries";
const std::string AppToDIFEntriesRIBObject::object_name = "/app_dif_entries";
AppToDIFEntriesRIBObject::AppToDIFEntriesRIBObject(DynamicDIFAllocator * dda_)
	: rina::rib::RIBObj(class_name)
{
	dda = dda_;
}

void AppToDIFEntriesRIBObject::create(const rina::cdap_rib::con_handle_t &con,
				      const std::string& fqn,
				      const std::string& class_,
				      const rina::cdap_rib::filt_info_t &filt,
				      const int invoke_id,
				      const rina::ser_obj_t &obj_req,
				      rina::ser_obj_t &obj_reply,
				      rina::cdap_rib::res_info_t& res)
{
	std::list<AppToDIFMapping> mappings;
	std::list<int> neighs_to_exclude;
	encoders::AppDIFMappingListEncoder encoder;

	neighs_to_exclude.push_back(con.port_id);

	encoder.decode(obj_req, mappings);

	if (mappings.size() == 0) {
		LOG_DBG("No new mappings to add");
		return;
	}

	dda->register_app(mappings, true, neighs_to_exclude);
}

// Class AppToDIFEntryRIBObject
class AppToDIFEntryRIBObject: public rina::rib::RIBObj {
public:
	AppToDIFEntryRIBObject(DynamicDIFAllocator * dda_,
			       const rina::ApplicationProcessNamingInformation& appn,
			       const std::string& dn);

	const std::string& get_class() const {
		return class_name;
	};

	const static std::string class_name;
	const static std::string object_name;

	bool delete_(const rina::cdap_rib::con_handle_t &con,
		     const std::string& fqn,
		     const std::string& class_,
		     const rina::cdap_rib::filt_info_t &filt,
		     const int invoke_id,
		     rina::cdap_rib::res_info_t& res);

private:

	rina::ApplicationProcessNamingInformation app_name;
	std::string dif_name;
	DynamicDIFAllocator * dda;
};

const std::string AppToDIFEntryRIBObject::class_name = "AppDIFEntry";
const std::string AppToDIFEntryRIBObject::object_name = "/app_dif_entries/id=";

AppToDIFEntryRIBObject::AppToDIFEntryRIBObject(DynamicDIFAllocator * dda_,
					       const rina::ApplicationProcessNamingInformation& appn,
					       const std::string& dn)
		: rina::rib::RIBObj(class_name)
{
	dda = dda_;
	app_name = appn;
	dif_name = dn;
}

bool AppToDIFEntryRIBObject::delete_(const rina::cdap_rib::con_handle_t &con,
				     const std::string& fqn,
				     const std::string& class_,
				     const rina::cdap_rib::filt_info_t &filt,
				     const int invoke_id,
				     rina::cdap_rib::res_info_t& res)
{
	std::list<int> exc_neighs;
	exc_neighs.push_back(con.port_id);
	dda->unregister_app(app_name, dif_name,
			       true, exc_neighs);
	return true;
}

// Class Dynamic DIF Allocator RIB Daemon
class DDARIBDaemon : public rina::rib::RIBDaemonAE
{
public:
	DDARIBDaemon(rina::cacep::AppConHandlerInterface *app_con_callback);
	~DDARIBDaemon();

	rina::rib::RIBDaemonProxy * getProxy();
        const rina::rib::rib_handle_t & get_rib_handle();
        int64_t addObjRIB(const std::string& fqn, rina::rib::RIBObj** obj);
        void removeObjRIB(const std::string& fqn);

private:
	//Handle to the RIB
	rina::rib::rib_handle_t rib;
	rina::rib::RIBDaemonProxy* ribd;
};

DDARIBDaemon::~DDARIBDaemon() {
	rina::rib::fini();
}

DDARIBDaemon::DDARIBDaemon(rina::cacep::AppConHandlerInterface *app_con_callback)
{
	rina::cdap_rib::cdap_params params;
	rina::cdap_rib::vers_info_t vers;
	rina::rib::RIBObj * robj = 0;

	//Initialize the RIB library and cdap
	params.ipcp = false;
	rina::rib::init(app_con_callback, params);
	ribd = rina::rib::RIBDaemonProxyFactory();

	//Create schema
	vers.version_ = 0x1ULL;
	ribd->createSchema(vers);

	//Create RIB
	rib = ribd->createRIB(vers);
	ribd->associateRIBtoAE(rib, "");

	LOG_INFO("RIB Daemon Initialized");
}

rina::rib::RIBDaemonProxy * DDARIBDaemon::getProxy()
{
	if (ribd)
		return ribd;

	return NULL;
}

const rina::rib::rib_handle_t & DDARIBDaemon::get_rib_handle()
{
	return rib;
}

int64_t DDARIBDaemon::addObjRIB(const std::string& fqn,
			        rina::rib::RIBObj** obj)
{
	return ribd->addObjRIB(rib, fqn, obj);
}

void DDARIBDaemon::removeObjRIB(const std::string& fqn)
{
	ribd->removeObjRIB(rib, fqn);
}

struct DAPeer
{
	std::string supporting_dif;
	rina::cdap_rib::con_handle_t con;
	rina::IAuthPolicySet * auth_ps_;
	int invoke_id;
	da_res_t status;
};

// Class DDAEnrollmentTask
class DDAEnrollmentTask: public rina::cacep::AppConHandlerInterface,
			 public rina::ApplicationEntity
{
public:
	DDAEnrollmentTask();
	~DDAEnrollmentTask();

	void set_application_process(rina::ApplicationProcess * ap);
	void connect(const rina::cdap::CDAPMessage& message,
		     rina::cdap_rib::con_handle_t &con);
	void connectResult(const rina::cdap::CDAPMessage& message,
			   rina::cdap_rib::con_handle_t &con);
	void release(int invoke_id,
		     const rina::cdap_rib::con_handle_t &con);
	void releaseResult(const rina::cdap_rib::res_info_t &res,
			   const rina::cdap_rib::con_handle_t &con);
	void process_authentication_message(const rina::cdap::CDAPMessage& message,
				            const rina::cdap_rib::con_handle_t &con);

	void initiateEnrollment(const rina::ApplicationProcessNamingInformation& peer_name,
				const std::string& dif_name, int port_id);

private:
	rina::Lockable lock;
	DynamicDIFAllocator * dda;
	std::map<std::string, DAPeer *> enrolled_das;
};

DDAEnrollmentTask::DDAEnrollmentTask() :
		ApplicationEntity(ApplicationEntity::ENROLLMENT_TASK_AE_NAME)
{
	dda = NULL;
}

DDAEnrollmentTask::~DDAEnrollmentTask()
{
	std::map<std::string, DAPeer *>::iterator itr;
	DAPeer * peer;

	itr = enrolled_das.begin();
	while (itr != enrolled_das.end()) {
		peer = itr->second;
		enrolled_das.erase(itr++);
		delete peer;
	}
}

void DDAEnrollmentTask::set_application_process(rina::ApplicationProcess * ap)
{
	if (!ap) {
		LOG_ERR("Bogus app passed");
		return;
	}

	dda = static_cast<DynamicDIFAllocator *>(ap);
}

void DDAEnrollmentTask::process_authentication_message(const rina::cdap::CDAPMessage& message,
						       const rina::cdap_rib::con_handle_t &con)
{
	//Ignore for now
}

void DDAEnrollmentTask::release(int invoke_id, const rina::cdap_rib::con_handle_t &con)
{
	//Ignore
}

void DDAEnrollmentTask::releaseResult(const rina::cdap_rib::res_info_t &res,
				      const rina::cdap_rib::con_handle_t &con)
{
	//Ignore
}

void DDAEnrollmentTask::initiateEnrollment(const rina::ApplicationProcessNamingInformation& peer_name,
					   const std::string& dif_name, int port_id)
{
	std::map<std::string, DAPeer *>::iterator it;
	DAPeer * peer;

	LOG_INFO("Starting enrollment with %s over port-id %d",
		  peer_name.getEncodedString().c_str(), port_id);

	for (it = enrolled_das.begin(); it != enrolled_das.end(); ++it) {
		if (it->second->con.dest_.ap_name_ == peer_name.processName ||
				it->second->con.src_.ap_name_ == peer_name.processName) {
			LOG_INFO("Already enrolled to DA peer %s",
				 peer_name.getEncodedString().c_str());
			return;
		}
	}

	peer = new DAPeer();
	peer->con.dest_.ap_name_ = peer_name.processName;
	peer->con.dest_.ap_inst_ = peer_name.processInstance;
	peer->con.dest_.ae_name_ = peer_name.entityName;
	peer->con.dest_.ae_inst_ = peer_name.entityInstance;
	peer->con.src_.ap_name_ = dda->dap_name.processName;
	peer->con.src_.ap_inst_ = dda->dap_name.processInstance;
	peer->con.port_id = port_id;
	peer->con.version_.version_ = 0x01;
	peer->supporting_dif = dif_name;
	peer->status = DA_PENDING;

	try {
		rina::cdap_rib::auth_policy_t auth;
		auth.name = "default";
		auth.versions.push_back("1");
		dda->ribd->getProxy()->remote_open_connection(peer->con.version_,
							      peer->con.src_,
							      peer->con.dest_,
							      auth,
							      peer->con.port_id);
	} catch (rina::Exception &e) {
		LOG_ERR("Problems opening CDAP connection to %s: %s",
				peer_name.getEncodedString().c_str(),
				e.what());
	}

	enrolled_das[peer_name.processName] = peer;
}

void DDAEnrollmentTask::connect(const rina::cdap::CDAPMessage& message,
				rina::cdap_rib::con_handle_t &con)
{
	std::map<std::string, DAPeer *>::iterator it;
	DAPeer * peer = 0;
	rina::cdap_rib::res_info_t res;

	//1 Find out if the sender is really connecting to us
	if(con.src_.ap_name_.compare(dda->dap_name.processName)!= 0){
		LOG_ERR("an M_CONNECT message whose destination was not this DA instance, ignoring it");
		dda->disconnect_from_peer(con.port_id);
		return;
	}

	//2 Check we are not enrolled yet
	lock.lock();
	it = enrolled_das.find(con.dest_.ap_name_);
	if (it != enrolled_das.end()) {
		lock.unlock();
		LOG_ERR("I am already enrolled to DIF Allocator instance %s",
			con.dest_.ap_name_.c_str());
		dda->disconnect_from_peer(con.port_id);
		return;
	}

	//3 Send connectResult
	LOG_INFO("Authenticating DIF Allocator %s-%s ...",
		  con.dest_.ap_name_.c_str(),
		  con.dest_.ap_inst_.c_str());

	//TODO authenticate
	LOG_INFO("Authentication successful!");

	//Send M_CONNECT_R
	try{
		res.code_ = rina::cdap_rib::CDAP_SUCCESS;
		rina::cdap::getProvider()->send_open_connection_result(con,
								       res,
								       con.auth_,
								       message.invoke_id_);
	}catch(rina::Exception &e){
		lock.unlock();
		LOG_ERR("Problems sending CDAP message: %s", e.what());
		dda->disconnect_from_peer(con.port_id);
		return;
	}

	peer = new DAPeer();
	peer->con = con;
	peer->invoke_id = message.invoke_id_;
	peer->status = DA_SUCCESS;
	enrolled_das[con.dest_.ap_name_] = peer;

	lock.unlock();

	dda->enrollment_completed(con);
}

void DDAEnrollmentTask::connectResult(const rina::cdap::CDAPMessage& message,
		   	   	      rina::cdap_rib::con_handle_t &con)
{
	std::map<std::string, DAPeer *>::iterator it;
	DAPeer * peer;

	lock.lock();
	it = enrolled_das.find(con.dest_.ap_name_);
	if (it == enrolled_das.end()) {
		lock.unlock();
		LOG_ERR("Enrollment with %s not in process, ignoring M_CONNECT_R",
				con.src_.ap_name_.c_str());
		dda->disconnect_from_peer(con.port_id);
		return;
	}

	peer = it->second;
	if (message.result_ != 0) {
		lock.unlock();
		LOG_ERR("Application connection rejected");
		enrolled_das.erase(it);
		dda->disconnect_from_peer(con.port_id);
		delete peer;
		return;
	}

	peer->status = DA_SUCCESS;

	lock.unlock();

	dda->enrollment_completed(con);
}

//DDAEnrollmentWorker
class DDAEnrollerWorker : public rina::SimpleThread
{
public:
	DDAEnrollerWorker(DynamicDIFAllocator * dda,
			    const std::list<rina::Neighbor>& enrollments);
	~DDAEnrollerWorker() throw() {};
	int run();

private:
	DynamicDIFAllocator * dda;
	std::list<rina::Neighbor> peers;

	int allocate_flow(const rina::FlowRequestEvent& alloc_event);
};

DDAEnrollerWorker::DDAEnrollerWorker(DynamicDIFAllocator * dda_,
					 const std::list<rina::Neighbor>& enrollments)
			: SimpleThread(std::string("dda-enroller-worker"), false)
{
	dda = dda_;
	peers = enrollments;
}

int DDAEnrollerWorker::allocate_flow(const rina::FlowRequestEvent& alloc_event)
{
	std::stringstream ss;
	std::string local_app_name;
	std::string remote_app_name;
	std::string dif_name;

	ss << alloc_event.localApplicationName.processName << "|"
	   << alloc_event.localApplicationName.processInstance << "||";
	local_app_name = ss.str();
	ss.str("");

	ss << alloc_event.remoteApplicationName.processName << "|"
	   << alloc_event.remoteApplicationName.processInstance << "||";
	remote_app_name = ss.str();
	ss.str("");

	dif_name = alloc_event.DIFName.processName;

	return rina_flow_alloc(dif_name.c_str(), local_app_name.c_str(),
	                       remote_app_name.c_str(), NULL, 0);
}

int DDAEnrollerWorker::run()
{
	int pending_peers;
	std::list<rina::Neighbor>::iterator it;
	rina::FlowRequestEvent event;
	rina::Sleep sleep;
	int fd;

	event.localRequest = true;
	event.localApplicationName = dda->dap_name;
	event.flowSpecification.maxAllowableGap = -1;
	event.flowSpecification.orderedDelivery = false;
	pending_peers = peers.size();
	while (pending_peers != 0) {
		LOG_DBG("DIF Allocator enroller sleeping for 5 seconds before attempting to connect to %d peers",
				pending_peers);
		sleep.sleepForMili(5000);

		pending_peers = 0;
		for (it = peers.begin(); it != peers.end(); ++it) {
			if (it->enrolled_)
				continue;

			event.remoteApplicationName = it->name_;
			event.DIFName = it->supporting_dif_name_;

			fd = allocate_flow(event);
			if(fd < 0) {
				it->enrolled_ = false;
				pending_peers ++;
			} else {
				it->enrolled_ = true;
				dda->n1_flow_allocated(*it, fd);
			}
		}
	}

	LOG_DBG("All peers connected, DIF Allocator enroller terminating");

	return 0;
}

//Class DDARegistrar
class DDARegistrar : public rina::SimpleThread
{
public:
	DDARegistrar(int fd, const std::string& dif_name,
		     const std::string& app_name);
	~DDARegistrar() throw() {};
	int run();

private:
	std::string dif_name;
	std::string app_name;
	int cfd;
};

DDARegistrar::DDARegistrar(int fd, const std::string& dn,
			   const std::string& an)
		: SimpleThread(std::string("dda-registrar"), false)
{
	cfd = fd;
	dif_name = dn;
	app_name = an;
}

int DDARegistrar::run()
{
	int ret;
	rina::Sleep sleep;

	sleep.sleepForMili(500);

	LOG_DBG("Registering DIF Allocator at DIF %s", dif_name.c_str());
	ret = rina_register(cfd, dif_name.c_str(), app_name.c_str(), 0);
	if (ret < 0) {
		LOG_ERR("Error registering DIF allocator to DIF %s",
			dif_name.c_str());
		return -1;
	}
	LOG_DBG("DIF Allocator registered at DIF %s", dif_name.c_str());

	return 0;
}

//Class DDAFlowAcceptor
class DDAFlowAcceptor : public rina::SimpleThread
{
public:
	DDAFlowAcceptor(DynamicDIFAllocator * dda_, int cfd);
	~DDAFlowAcceptor() throw() {};
	int run();

private:
	DynamicDIFAllocator * dda;
	int cfd;
};

DDAFlowAcceptor::DDAFlowAcceptor(DynamicDIFAllocator * dda_, int cfd_)
	: SimpleThread(std::string("dda-flow-acceptor"), false)
{
	dda = dda_;
	cfd = cfd_;
}

int DDAFlowAcceptor::run()
{
	int ret = 0;
	char *incomingapn = NULL;
	struct rina_flow_spec fspec;

	while (true) {
		fspec.version = RINA_FLOW_SPEC_VERSION;
		ret = rina_flow_accept(cfd, &incomingapn, &fspec, 0);
		if (ret < 0)
			break;

		dda->n1_flow_accepted(incomingapn, ret);
		free(incomingapn);
	}

	LOG_DBG("DIF Allocator flow acceptor exiting");
}

//Class SDUReader
class SDUReader : public rina::SimpleThread
{
public:
	SDUReader(int port_id, int fd_);
	~SDUReader() throw() {};
	int run();

private:
	int portid;
	int fd;
};

SDUReader::SDUReader(int port_id, int fd_)
	: SimpleThread(std::string("sdu-reader"), false)
{
	portid = port_id;
	fd = fd_;
}

int SDUReader::run()
{
	rina::ser_obj_t message;
	message.message_ = new unsigned char[5000];
	int bytes_read = 0;

	LOG_DBG("SDU reader of port-id %d starting", portid);

	while(true) {
		LOG_DBG("Going to read from file descriptor %d", fd);
		bytes_read = read(fd, message.message_, 5000);
		if (bytes_read <= 0) {
			LOG_ERR("Read error or EOF: %d", bytes_read);
			break;
		}

		LOG_DBG("Read %d bytes", bytes_read);
		message.size_ = bytes_read;

		//Instruct CDAP provider to process the CACEP message
		try{
			rina::cdap::getProvider()->process_message(message,
								   portid);
		} catch(rina::Exception &e){
			LOG_ERR("Problems processing message from port-id %d",
				portid);
		}
	}

	LOG_DBG("SDU Reader of port-id %d terminating", portid);

	return 0;
}

//Class Dynamic DIF Allocator
const std::string DynamicDIFAllocator::TYPE = "dynamic-dif-allocator";

DynamicDIFAllocator::DynamicDIFAllocator(const rina::ApplicationProcessNamingInformation& app_name,
		                         IPCManager_ * ipc_manager) :
		DIFAllocator(), rina::ApplicationProcess(app_name.processName, app_name.processInstance)
{
	ribd = NULL;
	et = NULL;
	ipcm = ipc_manager;
	dda_enroller = NULL;

	cfd = rina_open();
	if (cfd < 0) {
		LOG_ERR("DIF Allocator: could not open file descriptor");
		return;
	}

	facc = new DDAFlowAcceptor(this, cfd);
	facc->start();
}

DynamicDIFAllocator::~DynamicDIFAllocator()
{
	void * status;
	std::map<int, SDUReader *>::iterator itr;
	SDUReader * reader;

	if (ribd)
		delete ribd;

	if (et)
		delete et;

	if (dda_enroller) {
		dda_enroller->join(&status);
		delete dda_enroller;
	}

	itr = sdu_readers.begin();
	while (itr != sdu_readers.end()) {
		close(itr->first);
		reader = itr->second;
		reader->join(&status);
		sdu_readers.erase(itr++);
		delete reader;
	}

	close(cfd);
}

void DynamicDIFAllocator::disconnect_from_peer(int fd)
{
	void * status;
	std::map<int, SDUReader *>::iterator itr;
	SDUReader * reader;

	rina::ScopedLock g(lock);

	itr = sdu_readers.find(fd);
	if (itr == sdu_readers.end())
		return;

	close(itr->first);
	reader = itr->second;
	reader->join(&status);
	sdu_readers.erase(itr);

	delete reader;
}

int DynamicDIFAllocator::set_config(const DIFAllocatorConfig& da_config)
{
	rina::rib::RIBObj* tmp;

	daf_name = da_config.daf_name;
	dap_name = da_config.dap_name;
	enrollments = da_config.enrollments;
	joinable_difs = da_config.joinable_difs;

	et = new DDAEnrollmentTask();
	ribd = new DDARIBDaemon(et);

	ribd->set_application_process(this);
	et->set_application_process(this);

	try {
		tmp = new AppToDIFEntriesRIBObject(this);
		ribd->addObjRIB(AppToDIFEntriesRIBObject::object_name, &tmp);
	} catch (rina::Exception &e) {
		LOG_ERR("Problems adding object to the RIB : %s", e.what());
	}

	dda_enroller = new DDAEnrollerWorker(this, enrollments);
	dda_enroller->start();

	return 0;
}

void DynamicDIFAllocator::assigned_to_dif(const std::string& dif_name)
{
	std::stringstream ss;
	DDARegistrar * ddar;

	rina::ScopedLock g(lock);

	ss << dap_name.processName << "|" << dap_name.processInstance << "||";

	ddar = new DDARegistrar(cfd, dif_name, ss.str());
	ddar->start();
}

void DynamicDIFAllocator::n1_flow_allocated(const rina::Neighbor& neighbor, int fd)
{
	std::stringstream ss;
	SDUReader * reader = 0;

	rina::ScopedLock g(lock);

	// Use fd as port-id
	reader = new SDUReader(fd, fd);
	reader->start();

	sdu_readers[fd] = reader;

	rina::cdap::add_fd_to_port_id_mapping(fd, fd);
	et->initiateEnrollment(neighbor.name_, neighbor.supporting_dif_name_.processName,
			       fd);
}

void DynamicDIFAllocator::n1_flow_accepted(const char * incoming_apn, int fd)
{
	std::stringstream ss;
	SDUReader * reader = 0;

	LOG_DBG("Accepted flow from peer DA: %s", incoming_apn);

	rina::ScopedLock g(lock);

	rina::cdap::add_fd_to_port_id_mapping(fd, fd);

	reader = new SDUReader(fd, fd);
	reader->start();

	sdu_readers[fd] = reader;
}

void DynamicDIFAllocator::enrollment_completed(const rina::cdap_rib::con_handle_t &con)
{
	std::list< std::list<AppToDIFMapping> > atdmap;
	rina::cdap_rib::obj_info_t obj;
	rina::cdap_rib::flags_t flags;
	rina::cdap_rib::filt_info_t filt;
	encoders::AppDIFMappingListEncoder encoder;

	LOG_DBG("Enrollment to peer DA %s %s completed",
			con.dest_.ap_name_.c_str(),
			con.dest_.ap_inst_.c_str());

	rina::ScopedLock g(lock);

	//Notify peer DA bout current App to DIF mappings I know
	getAllMappingsForPropagation(atdmap);
	if (atdmap.size() == 0)
		return;

	obj.class_ = AppToDIFEntriesRIBObject::class_name;
	obj.name_ = AppToDIFEntriesRIBObject::object_name;
	for (std::list< std::list<AppToDIFMapping> >::iterator it = atdmap.begin();
			it != atdmap.end(); ++it) {
		encoder.encode(*it, obj.value_);
		try {
			ribd->getProxy()->remote_create(con, obj, flags,
							filt, NULL);
		} catch (rina::Exception &e) {
			LOG_WARN("Problems sending create CDAP message: %s",
					e.what());
		}
	}
}

da_res_t DynamicDIFAllocator::lookup_dif_by_application(const rina::ApplicationProcessNamingInformation& app_name,
			       	   	   	        rina::ApplicationProcessNamingInformation& result,
							std::list<std::string>& supporting_difs)
{
	std::map<std::string, AppToDIFMapping *>::iterator itr;
	std::string dif_name;

	rina::ScopedLock g(lock);

	for(itr = app_dif_mappings.begin();
			itr != app_dif_mappings.end(); ++itr) {
		if (itr->second->app_name.processName == app_name.processName &&
				itr->second->app_name.processInstance == app_name.processInstance) {
			dif_name = itr->second->dif_names.front();
			result.processName = dif_name;
			find_supporting_difs(supporting_difs, dif_name);
			return DA_SUCCESS;
		}
	}

	return DA_FAILURE;
}

void DynamicDIFAllocator::find_supporting_difs(std::list<std::string>& supporting_difs,
			  	  	       const std::string& dif_name)
{
	std::map<std::string, AppToDIFMapping *>::iterator itr;
	std::list<std::string>::iterator sitr;

	for(itr = app_dif_mappings.begin();
				itr != app_dif_mappings.end(); ++itr) {
		if (itr->second->app_name.processName == dif_name) {
			for (sitr = itr->second->dif_names.begin();
					sitr != itr->second->dif_names.end(); ++sitr) {
				supporting_difs.push_back(*sitr);
			}

			return;
		}
	}
}

void DynamicDIFAllocator::getAllMappingsForPropagation(std::list< std::list<AppToDIFMapping> >& atdmap)
{
	std::list<AppToDIFMapping> atdlist;

	if (app_dif_mappings.empty())
		return;

	for (std::map<std::string, AppToDIFMapping*>::iterator it
			= app_dif_mappings.begin(); it != app_dif_mappings.end();++it)
	{
		if (atdlist.size() == MAX_OBJECTS_PER_UPDATE_DEFAULT) {
			atdmap.push_back(atdlist);
			atdlist.clear();
		}

		atdlist.push_back(*(it->second));
	}

	if (atdlist.size() != 0) {
		atdmap.push_back(atdlist);
	}
}

void DynamicDIFAllocator::list_da_mappings(std::ostream& os)
{
	std::map<std::string, AppToDIFMapping *>::iterator itr;
	std::list<std::string>::iterator sitr;

	rina::ScopedLock g(lock);

	for(itr = app_dif_mappings.begin();
			itr != app_dif_mappings.end(); ++itr) {
		os << "Application " << itr->second->app_name.getEncodedString()
		   << " available through DIFs: ";

		for (sitr = itr->second->dif_names.begin();
				sitr != itr->second->dif_names.end(); sitr++) {
			os << *sitr << ", ";
		}

		os << std::endl;
	}
}

void DynamicDIFAllocator::get_ipcp_name_for_dif(rina::ApplicationProcessNamingInformation& ipcp_name,
			   	   	   	const std::string& dif_name)
{
	get_ipcp_name(ipcp_name, dif_name, joinable_difs);
}

bool DynamicDIFAllocator::contains_entry(int candidate, const std::list<int>& elements)
{
	std::list<int>::const_iterator it;
	for (it = elements.begin(); it != elements.end(); ++it) {
		if (candidate == *it)
			return true;
	}

	return false;
}

void DynamicDIFAllocator::register_app(const std::list<AppToDIFMapping> & mappings,
		  	  	       bool notify_neighs,
				       std::list<int>& neighs_to_exclude)
{
	std::list<AppToDIFMapping>::const_iterator litr;
	std::map<std::string, AppToDIFMapping *>::iterator itr;
	std::map<int, SDUReader *>::iterator ritr;
	AppToDIFMapping * mapping;
	rina::rib::RIBObj * nrobj;
	std::string dif_name;
	rina::cdap_rib::obj_info_t obj;
	rina::cdap_rib::flags_t flags;
	rina::cdap_rib::filt_info_t filt;
	rina::cdap_rib::con_handle_t con;
	encoders::AppDIFMappingListEncoder encoder;

	lock.lock();

	// Iterate through entries, add them to the table and create RIB objects
	for(litr = mappings.begin();
			litr != mappings.end(); litr++) {
		itr = app_dif_mappings.find(litr->app_name.getEncodedString());
		if (itr == app_dif_mappings.end()) {
			mapping = new AppToDIFMapping();
			mapping->app_name = litr->app_name;
			app_dif_mappings[litr->app_name.getEncodedString()] = mapping;
		} else {
			mapping = itr->second;
		}

		dif_name = litr->dif_names.front();
		mapping->dif_names.push_back(dif_name);

		LOG_DBG("Added app to DIF mapping: %s to %s",
			 litr->app_name.getEncodedString().c_str(),
			 dif_name.c_str());

		try {
			std::stringstream ss;
			ss << AppToDIFEntryRIBObject::object_name
			   << litr->app_name.getEncodedString() << "#"
			   << litr->dif_names.front();

			nrobj = new AppToDIFEntryRIBObject(this, litr->app_name, dif_name);
			ribd->addObjRIB(ss.str(), &nrobj);
		} catch (rina::Exception &e) {
			LOG_ERR("Problems creating RIB object: %s",
					e.what());
		}
	}

	if (!notify_neighs) {
		lock.unlock();
		return;
	}

	//Notify neighbors
	obj.class_ = AppToDIFEntriesRIBObject::class_name;
	obj.name_ = AppToDIFEntriesRIBObject::object_name;
	encoder.encode(mappings, obj.value_);

	for (ritr = sdu_readers.begin();
			ritr != sdu_readers.end(); ritr++) {
		if (contains_entry(ritr->first,
				   neighs_to_exclude))
			continue;

		try {
			con.port_id = ritr->first;
			ribd->getProxy()->remote_create(con, obj, flags,
					     	        filt, NULL);
		} catch (rina::Exception &e) {
			LOG_WARN("Problems sending delete CDAP message: %s",
					e.what());
		}
	}

	lock.unlock();
}

void DynamicDIFAllocator::app_registered(const rina::ApplicationProcessNamingInformation & app_name,
			    	    	 const std::string& dif_name)
{
	std::list<AppToDIFMapping> mappings;
	AppToDIFMapping map;
	std::list<int> neighs_to_exclude;

	map.app_name = app_name;
	map.dif_names.push_back(dif_name);
	mappings.push_back(map);

	register_app(mappings, true, neighs_to_exclude);
}

void DynamicDIFAllocator::unregister_app(const rina::ApplicationProcessNamingInformation& app_name,
		  	  	         const std::string& dif_name,
				         bool notify_neighs,
				         std::list<int>& neighs_to_exclude)
{
	std::map<std::string, AppToDIFMapping *> ::iterator itr;
	std::list<std::string>::iterator ditr;
	std::map<int, SDUReader *>::iterator ritr;
	AppToDIFMapping * mapping;
	rina::cdap_rib::obj_info_t obj;
	rina::cdap_rib::flags_t flags;
	rina::cdap_rib::filt_info_t filt;
	rina::cdap_rib::con_handle_t con;
	std::stringstream ss;

	lock.lock();
	itr = app_dif_mappings.find(app_name.getEncodedString());
	if (itr == app_dif_mappings.end()) {
		lock.unlock();
		return;
	}

	mapping = itr->second;
	for (ditr = mapping->dif_names.begin(); ditr != mapping->dif_names.end(); ++ditr) {
		if (*ditr == dif_name) {
			mapping->dif_names.erase(ditr);
			break;
		}
	}

	if (mapping->dif_names.size() == 0) {
		app_dif_mappings.erase(itr);
		delete mapping;
	}

	LOG_DBG("Removed app to DIF mapping: %s to %s",
		 app_name.getEncodedString().c_str(),
		 dif_name.c_str());

	ss << AppToDIFEntryRIBObject::object_name << app_name.getEncodedString()
			<< "#" << dif_name;
	obj.class_ = AppToDIFEntryRIBObject::class_name;
	obj.name_ = ss.str();

	try {
		ribd->removeObjRIB(obj.name_);
	} catch (rina::Exception &e){
		LOG_ERR("Error removing object from RIB %s",
			 e.what());
	}

	if (!notify_neighs) {
		lock.unlock();
		return;
	}

	for (ritr = sdu_readers.begin();
			ritr != sdu_readers.end(); ritr++) {
		if (contains_entry(ritr->first,
				   neighs_to_exclude))
			continue;

		try {
			con.port_id = ritr->first;
			ribd->getProxy()->remote_delete(con, obj, flags,
							filt, NULL);
		} catch (rina::Exception &e) {
			LOG_WARN("Problems sending delete CDAP message: %s",
					e.what());
		}
	}

	lock.unlock();
}

void DynamicDIFAllocator::app_unregistered(const rina::ApplicationProcessNamingInformation & app_name,
					   const std::string& dif_name)
{
	std::list<int> excluded_neighs;

	unregister_app(app_name, dif_name,
		       true, excluded_neighs);
}

void DynamicDIFAllocator::update_directory_contents()
{
	//Ignore
}

unsigned int DynamicDIFAllocator::get_address() const
{
	return 0;
}

} //namespace rinad

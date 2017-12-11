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

#include "configuration.h"
#include "ipcm.h"

using namespace std;


namespace rinad {

/// Static DIF Allocator, reads Application to DIF mappings from a config file
class StaticDIFAllocator : public DIFAllocator {
public:
	static const std::string TYPE;

	StaticDIFAllocator();
	virtual ~StaticDIFAllocator(void);
	int set_config(const DIFAllocatorConfig& da_config,
		       rina::ApplicationProcessNamingInformation& da_name);
	void local_app_registered(const rina::ApplicationProcessNamingInformation& local_app_name,
			          const rina::ApplicationProcessNamingInformation& dif_name);
	void local_app_unregistered(const rina::ApplicationProcessNamingInformation& local_app_name,
			            const rina::ApplicationProcessNamingInformation& dif_name);
	da_res_t lookup_dif_by_application(const rina::ApplicationProcessNamingInformation& app_name,
        			       	   rina::ApplicationProcessNamingInformation& result,
					   const std::list<std::string>& supported_difs);
        void update_directory_contents();

private:
        void print_directory_contents();

        std::string folder_name;
        std::string fq_file_name;

	//The current DIF Directory
	std::list< std::pair<std::string, std::string> > dif_directory;

	rina::ReadWriteLockable directory_lock;
};

class DDARIBDaemon;
class DDAEnrollmentTask;

class DynamicDIFAllocator : public DIFAllocator, public rina::ApplicationProcess {
public:
	static const std::string TYPE;

	DynamicDIFAllocator(const rina::ApplicationProcessNamingInformation& ap_name,
			    IPCManager_ * ipcm);
	virtual ~DynamicDIFAllocator(void);
	int set_config(const DIFAllocatorConfig& da_config,
		       rina::ApplicationProcessNamingInformation& da_name);
	void local_app_registered(const rina::ApplicationProcessNamingInformation& local_app_name,
			          const rina::ApplicationProcessNamingInformation& dif_name);
	void local_app_unregistered(const rina::ApplicationProcessNamingInformation& local_app_name,
			            const rina::ApplicationProcessNamingInformation& dif_name);
	da_res_t lookup_dif_by_application(const rina::ApplicationProcessNamingInformation& app_name,
        			       	   rina::ApplicationProcessNamingInformation& result,
					   const std::list<std::string>& supported_difs);
        void update_directory_contents();
        unsigned int get_address() const;

	/// The name of the DIF Allocator DAP instance
	rina::ApplicationProcessNamingInformation dap_name;

	DDARIBDaemon * ribd;
	DDAEnrollmentTask * et;

private:
	IPCManager_ * ipcm;

	/// The name of the DIF Allocator DAF
	rina::ApplicationProcessNamingInformation daf_name;

	/// Peer DA instances to enroll to
	std::list<rina::Neighbor> enrollments;
};

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
					     rina::ApplicationProcessNamingInformation& da_name,
					     IPCManager_ * ipcm)
{
	DIFAllocator * result;
	DIFAllocatorConfig da_config;

	DIFAllocator::parse_config(da_config, config);

	if (da_config.type == StaticDIFAllocator::TYPE) {
		result = new StaticDIFAllocator();
		result->set_config(da_config, da_name);
	} else if (da_config.type == DynamicDIFAllocator::TYPE) {
		result = new DynamicDIFAllocator(da_config.dap_name, ipcm);
		result->set_config(da_config, da_name);
	} else {
		result = NULL;
	}

	return result;
}

const std::string StaticDIFAllocator::TYPE = "static-dif-allocator";

StaticDIFAllocator::StaticDIFAllocator() : DIFAllocator()
{
}

StaticDIFAllocator::~StaticDIFAllocator()
{
}

int StaticDIFAllocator::set_config(const DIFAllocatorConfig& da_config,
				   rina::ApplicationProcessNamingInformation& da_name)
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

	//load current mappings
	if (!parse_app_to_dif_mappings(fq_file_name, dif_directory)) {
		LOG_ERR("Problems loading initial directory");
		return -1;
	}

	print_directory_contents();

	return 0;
}

void StaticDIFAllocator::local_app_registered(const rina::ApplicationProcessNamingInformation& local_app_name,
		          	  	      const rina::ApplicationProcessNamingInformation& dif_name)
{
	//Ignore
}

void StaticDIFAllocator::local_app_unregistered(const rina::ApplicationProcessNamingInformation& local_app_name,
		            	    	    	const rina::ApplicationProcessNamingInformation& dif_name)
{
	//Ignore
}

da_res_t StaticDIFAllocator::lookup_dif_by_application(const rina::ApplicationProcessNamingInformation& app_name,
                			     	       rina::ApplicationProcessNamingInformation& result,
						       const std::list<std::string>& supported_difs)
{
	rina::ReadScopedLock g(directory_lock);
        string encoded_name = app_name.getEncodedString();
        std::list< std::pair<std::string, std::string> >::iterator it;
        std::list<std::string>::const_iterator jt;

        for (it = dif_directory.begin(); it != dif_directory.end(); ++it) {
        	if (it->first == encoded_name) {
        		for (jt = supported_difs.begin(); jt != supported_difs.end(); ++jt) {
        			if (it->second == *jt) {
        				result.processName = it->second;
        				return DA_SUCCESS;
        			}
        		}
        	}
        }

        return DA_FAILURE;
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
	AppToDIFEntriesRIBObject();
	const std::string& get_class() const {
		return class_name;
	};

	const static std::string class_name;
	const static std::string object_name;
};

const std::string AppToDIFEntriesRIBObject::class_name = "AppDIFEntries";
const std::string AppToDIFEntriesRIBObject::object_name = "/app_dif_entries";
AppToDIFEntriesRIBObject::AppToDIFEntriesRIBObject() : rina::rib::RIBObj(class_name)
{
}

// Class AppToDIFEntryRIBObject
class AppToDIFEntryRIBObject: public rina::rib::RIBObj {
public:
	AppToDIFEntryRIBObject();
	const std::string& get_class() const {
		return class_name;
	};

	const static std::string class_name;
	const static std::string object_name;

	static void create_cb(const rina::rib::rib_handle_t rib,
			      const rina::cdap_rib::con_handle_t &con,
			      const std::string& fqn,
			      const std::string& class_,
			      const rina::cdap_rib::filt_info_t &filt,
			      const int invoke_id,
			      const rina::ser_obj_t &obj_req,
			      rina::cdap_rib::obj_info_t &obj_reply,
			      rina::cdap_rib::res_info_t& res);
};

const std::string AppToDIFEntryRIBObject::class_name = "AppDIFEntry";
const std::string AppToDIFEntryRIBObject::object_name = "/app_dif_entries/id=";
AppToDIFEntryRIBObject::AppToDIFEntryRIBObject() : rina::rib::RIBObj(class_name)
{
}

void AppToDIFEntryRIBObject::create_cb(const rina::rib::rib_handle_t rib,
		      	      	      const rina::cdap_rib::con_handle_t &con,
				      const std::string& fqn,
				      const std::string& class_,
				      const rina::cdap_rib::filt_info_t &filt,
				      const int invoke_id,
				      const rina::ser_obj_t &obj_req,
				      rina::cdap_rib::obj_info_t &obj_reply,
				      rina::cdap_rib::res_info_t& res)
{
	//TODO
}

// Class Dynamic DIF Allocator RIB Daemon
class DDARIBDaemon : public rina::rib::RIBDaemonAE
{
public:
	DDARIBDaemon(rina::cacep::AppConHandlerInterface *app_con_callback);
	~DDARIBDaemon(){};

	rina::rib::RIBDaemonProxy * getProxy();
        const rina::rib::rib_handle_t & get_rib_handle();
        int64_t addObjRIB(const std::string& fqn, rina::rib::RIBObj** obj);
        void removeObjRIB(const std::string& fqn);

private:
	//Handle to the RIB
	rina::rib::rib_handle_t rib;
	rina::rib::RIBDaemonProxy* ribd;
};

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

	//TODO: Add create callbacks
	ribd->addCreateCallbackSchema(vers,
				      AppToDIFEntryRIBObject::class_name,
				      AppToDIFEntriesRIBObject::object_name,
				      AppToDIFEntryRIBObject::create_cb);

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

	rina::ScopedLock g(lock);

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
		//TODO deallocate flow con.port_id
		return;
	}

	rina::ScopedLock g(lock);

	//2 Check we are not enrolled yet
	it = enrolled_das.find(con.dest_.ap_name_);
	if (it != enrolled_das.end()) {
		LOG_ERR("I am already enrolled to DIF Allocator instance %s",
			con.dest_.ap_name_.c_str());
		//TODO deallocate flow con.port_id
		return;
	}

	//3 Send connectResult
	peer = new DAPeer();
	peer->con = con;
	peer->invoke_id = message.invoke_id_;

	LOG_INFO("Authenticating DIF Allocator %s-%s ...",
		  con.dest_.ap_name_.c_str(),
		  con.dest_.ap_inst_.c_str());

	//TODO authenticate

	LOG_INFO("Authentication successful!");

	peer->status = DA_SUCCESS;
	enrolled_das[con.dest_.ap_name_] = peer;

	//Send M_CONNECT_R
	try{
		res.code_ = rina::cdap_rib::CDAP_SUCCESS;
		rina::cdap::getProvider()->send_open_connection_result(peer->con,
								       res,
								       peer->con.auth_,
								       peer->invoke_id);
	}catch(rina::Exception &e){
		LOG_ERR("Problems sending CDAP message: %s", e.what());
		//TODO deallocate flow
		//ckm->irm->deallocate_flow(con.port_id);
		return;
	}

	//TODO send directory update
}

void DDAEnrollmentTask::connectResult(const rina::cdap::CDAPMessage& message,
		   	   	      rina::cdap_rib::con_handle_t &con)
{
	std::map<std::string, DAPeer *>::iterator it;
	DAPeer * peer;

	rina::ScopedLock g(lock);
	it = enrolled_das.find(con.dest_.ap_name_);
	if (it == enrolled_das.end()) {
		LOG_ERR("Enrollment with %s not in process, ignoring M_CONNECT_R",
				con.src_.ap_name_.c_str());
		//TODO deallocate flow con.port_id
		return;
	}

	peer = it->second;
	if (message.result_ != 0) {
		LOG_ERR("Application connection rejected");
		enrolled_das.erase(it);
		// TODO deallocate flow con.port_id
		delete peer;
		return;
	}

	peer->status = DA_SUCCESS;

	//TODO send directory entries to peer
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
}

DynamicDIFAllocator::~DynamicDIFAllocator()
{
	if (ribd)
		delete ribd;

	if (et)
		delete et;
}

int DynamicDIFAllocator::set_config(const DIFAllocatorConfig& da_config,
				    rina::ApplicationProcessNamingInformation& da_name)
{
	daf_name = da_config.daf_name;
	dap_name = da_config.dap_name;
	enrollments = da_config.enrollments;

	da_name.processName = da_config.dap_name.processName;
	da_name.processInstance = da_config.dap_name.processInstance;

	et = new DDAEnrollmentTask();
	ribd = new DDARIBDaemon(et);
}

void DynamicDIFAllocator::local_app_registered(const rina::ApplicationProcessNamingInformation& local_app_name,
		          	  	       const rina::ApplicationProcessNamingInformation& dif_name)
{
	//TODO
}

void DynamicDIFAllocator::local_app_unregistered(const rina::ApplicationProcessNamingInformation& local_app_name,
		            	    	    	 const rina::ApplicationProcessNamingInformation& dif_name)
{
	//Ignore registrations from the DIF Allocator itself
	if (local_app_name.processName == daf_name.processName &&
			local_app_name.processInstance == dap_name.processName) {
		return;
	}

	//TODO
}

da_res_t DynamicDIFAllocator::lookup_dif_by_application(const rina::ApplicationProcessNamingInformation& app_name,
			       	   	   	        rina::ApplicationProcessNamingInformation& result,
							const std::list<std::string>& supported_difs)
{
	//TODO
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

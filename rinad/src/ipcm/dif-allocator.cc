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

#include "configuration.h"
#include "dif-allocator.h"

using namespace std;


namespace rinad {

// Class DIF Allocator
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
					     rina::ApplicationProcessNamingInformation& da_name)
{
	DIFAllocator * result;
	DIFAllocatorConfig da_config;

	DIFAllocator::parse_config(da_config, config);

	if (da_config.type == StaticDIFAllocator::TYPE) {
		result = new StaticDIFAllocator();
		result->set_config(da_config, da_name);
	} else if (da_config.type == DynamicDIFAllocator::TYPE) {
		result = new DynamicDIFAllocator();
		result->set_config(da_config, da_name);
	} else {
		result = NULL;
	}

	return result;
}

//Class Static DIF Allocator
const std::string StaticDIFAllocator::DIF_DIRECTORY_FILE_NAME = "da.map";
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
	ss << "/" << DIF_DIRECTORY_FILE_NAME;
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

// Class DDAEnrollmentTask
/*class DDAEnrollmentTask: public rina::cacep::AppConHandlerInterface,
			 public rina::ApplicationEntity
{
public:
	DDAEnrollmentTask(const std::string& ckm_name);
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

private:
	void initiateEnrollment(const rina::NMinusOneFlowAllocatedEvent& event);

	rina::Lockable lock;
	std::string ckm_ap_name;
	DynamicDIFAllocator * dda;
	CKMEnrollmentState ckm_state;
	CKMEnrollmentState ma_state;
	int ma_invoke_id;
};

void DDAEnrollmentTask::set_application_process(rina::ApplicationProcess * ap)
{
	if (!ap) {
		LOG_ERR("Bogus app passed");
		return;
	}

	dda = static_cast<DynamicDIFAllocator *>(ap);
}

void DDAEnrollmentTask::initiateEnrollment(const rina::NMinusOneFlowAllocatedEvent& event)
{

	LOG_INFO("Starting enrollment with %s over port-id %d",
		  event.flow_information_.remoteAppName.getEncodedString().c_str(),
		  event.flow_information_.portId);

	rina::ScopedLock g(lock);

	ckm_state.con.dest_.ap_name_ = event.flow_information_.remoteAppName.processName;
	ckm_state.con.dest_.ap_inst_ = event.flow_information_.remoteAppName.processInstance;
	ckm_state.con.dest_.ae_name_ = event.flow_information_.remoteAppName.entityName;
	ckm_state.con.dest_.ae_inst_ = event.flow_information_.remoteAppName.entityInstance;
	ckm_state.con.src_.ap_name_ = event.flow_information_.localAppName.processName;
	ckm_state.con.src_.ap_inst_ = event.flow_information_.localAppName.processInstance;
	ckm_state.con.src_.ae_name_ = event.flow_information_.localAppName.entityName;
	ckm_state.con.src_.ae_inst_ = event.flow_information_.localAppName.entityInstance;
	ckm_state.con.port_id = event.flow_information_.portId;
	ckm_state.con.version_.version_ = 0x01;

	ckm_state.auth_ps_ = kma->secman->get_auth_policy_set(kma->secman->sec_profile.authPolicy.name_);
	if (!ckm_state.auth_ps_) {
		LOG_ERR("Could not %s authentication policy set, aborting",
			kma->secman->sec_profile.authPolicy.name_.c_str());
		kma->irm->deallocate_flow(event.flow_information_.portId);
		return;
	}

	try {
		rina::cdap_rib::auth_policy_t auth = ckm_state.auth_ps_->get_auth_policy(event.flow_information_.portId,
				  	  	  	  	  	  	  	 ckm_state.con.dest_,
											 kma->secman->sec_profile);
		kma->ribd->getProxy()->remote_open_connection(ckm_state.con.version_,
							      ckm_state.con.src_,
							      ckm_state.con.dest_,
							      auth,
							      ckm_state.con.port_id);
	} catch (rina::Exception &e) {
		LOG_ERR("Problems opening CDAP connection to %s: %s",
				event.flow_information_.remoteAppName.getEncodedString().c_str(),
				e.what());
	}
}*/


//Class Dynamic DIF Allocator
const std::string DynamicDIFAllocator::TYPE = "dynamic-dif-allocator";

DynamicDIFAllocator::DynamicDIFAllocator() : DIFAllocator()
{
}

DynamicDIFAllocator::~DynamicDIFAllocator()
{
}

int DynamicDIFAllocator::set_config(const DIFAllocatorConfig& da_config,
				    rina::ApplicationProcessNamingInformation& da_name)
{
	//TODO parse config
	daf_name = da_config.daf_name;
	dap_name = da_config.dap_name;

	da_name.processName = da_config.daf_name.processName;
	da_name.processInstance = da_config.dap_name.processName;
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

} //namespace rinad

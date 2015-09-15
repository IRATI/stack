#include <ipc_processes_obj.h>
#include <ipcp_obj.h>
#include <ribd_obj.cc>

#define RINA_PREFIX "ipcm.mad.ribd_v1"
#include <librina/logs.h>
#include <librina/exceptions.h>

#include "../../../ipcm.h"
#include "../agent.h"

namespace rinad {
namespace mad {
namespace rib_v1 {

//Instance generator
extern Singleton<rina::ConsecutiveUnsignedIntegerGenerator> inst_gen;

//Static class names
const std::string IPCProcessesObj::class_name = "OSApplicationProcess";

IPCProcessesObj::IPCProcessesObj(
		std::string name, long instance,
		rina::rib::RIBDNorthInterface* ribd)
		: rina::rib::EmptyRIBObject(class_name, name, instance,
						&encoder_) {

	ribd_ = ribd;
}

rina::cdap_rib::res_info_t* IPCProcessesObj::remoteCreate(
		const std::string& name, const std::string clas,
		const rina::cdap_rib::SerializedObject &obj_req,
		rina::cdap_rib::SerializedObject &obj_reply) {

	rina::cdap_rib::res_info_t* res = new rina::cdap_rib::res_info_t;

	if (clas == IPCPObj::class_name) {
		IPCPObj* ipcp;

		try {
			ipcp = new IPCPObj(name, inst_gen->next(), obj_req);
		} catch (...) {
			LOG_ERR("Unable to create an IPCP object '%s'; out of memory?",
				name.c_str());
			res->result_ = -1;
			return res;
		}

		mad_manager::structures::ipcp_config_t object;
		rinad::mad_manager::encoders::IPCPConfigEncoder().decode(
				obj_req, object);
		int ipcp_id = createIPCP(object);
		if (ipcp_id > 0) {
			res->result_ = 1;
			if (assignToDIF(object, ipcp_id)) {
				res->result_ = 1;
				if (!object.dif_to_register.empty()) {
					if (registerAtDIF(object, ipcp_id))
					{
						res->result_ = 1;
					}
					else {
						// TODO Implement destroy ipcp
						res->result_ = -1;
					}
				}
			} else {
				// TODO Implement destroy ipcp
				res->result_ = -1;
			}
		} else {
			res->result_ = -1;
		}
		if (res->result_ > 0) {
			ribd_->addRIBObject(ipcp);
			// TODO: create basic IPCP objects
			ribd_->addRIBObject(
					new RIBDaemonObj(
							name + ", RIBDaemon",
							inst_gen->next(),
							ipcp_id));
		}

	} else {
		LOG_ERR("Create object %s is not implemented as a child of OSApplicationProcess object",
			name.c_str());
		res->result_ = -1;
	}
	return res;
}

int IPCProcessesObj::createIPCP(
		rinad::mad_manager::structures::ipcp_config_t &object) {
	CreateIPCPPromise ipcp_promise;

	rina::ApplicationProcessNamingInformation ipcp_name(
			object.process_name, object.process_instance);

	if (IPCManager->create_ipcp(ManagementAgent::inst, &ipcp_promise,
					ipcp_name, object.process_type)
			== IPCM_FAILURE
			|| ipcp_promise.wait() != IPCM_SUCCESS) {
		LOG_ERR("Error while creating IPC process");
		return -1;
	}
	LOG_INFO("IPC process created successfully [id = %d, name= %s]",
			ipcp_promise.ipcp_id, object.process_name.c_str());
	return ipcp_promise.ipcp_id;

}
bool IPCProcessesObj::assignToDIF(
		rinad::mad_manager::structures::ipcp_config_t &object,
		int ipcp_id) {
	// ASSIGN TO DIF
	Promise assign_promise;
	bool found;
	rinad::DIFTemplateMapping template_mapping;
	rinad::DIFTemplate * dif_template;

	rina::ApplicationProcessNamingInformation dif_name(object.dif_to_assign,
								std::string());

	if (!IPCManager->ipcp_exists(ipcp_id)) {
		LOG_ERR("No such IPC process id %d", ipcp_id);
		return false;
	}

	found = IPCManager->getConfig().lookup_dif_template_mappings(dif_name, template_mapping);
	if (!found) {
		LOG_ERR("Could not find DIF template for DIF name %s",
				dif_name.processName.c_str());
		return false;
	}

	dif_template = IPCManager->dif_template_manager->get_dif_template(template_mapping.template_name);
	if (!dif_template) {
		LOG_ERR("Cannot find template called %s",
				template_mapping.template_name.c_str());
		return false;
	}

	if (IPCManager->assign_to_dif(ManagementAgent::inst, &assign_promise,
					ipcp_id, dif_template, dif_name) == IPCM_FAILURE
			|| assign_promise.wait() != IPCM_SUCCESS) {
		LOG_ERR("DIF assignment failed");
		return false;
	}

	LOG_INFO("DIF assignment completed successfully");

	return true;
}
bool IPCProcessesObj::registerAtDIF(
		mad_manager::structures::ipcp_config_t &object, int ipcp_id) {
	Promise promise;

	rina::ApplicationProcessNamingInformation dif_name(
			object.dif_to_register, std::string());

	if (!IPCManager->ipcp_exists(ipcp_id)) {
		LOG_ERR("No such IPC process id");
		return false;
	}

	if (IPCManager->register_at_dif(ManagementAgent::inst, &promise,
					ipcp_id, dif_name) == IPCM_FAILURE
			|| promise.wait() != IPCM_SUCCESS) {
		LOG_ERR("Registration failed");
		return false;
	}

	LOG_INFO("IPC process registration completed successfully");

	return true;
}
}//namespace rib_v1
}//namespace mad
}//namespace rinad

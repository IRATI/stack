#include <ipcp_obj.h>

#define RINA_PREFIX "ipcm.mad.ribd_v1"
#include <librina/logs.h>
#include <librina/exceptions.h>

#include "../../../ipcm.h"

namespace rinad {
namespace mad {
namespace rib_v1 {

//Instance generator
extern Singleton<rina::ConsecutiveUnsignedIntegerGenerator> inst_gen;

//Static class names
const std::string IPCPObj::class_name = "IPCProcess";
const std::string OSApplicationProcessObj::class_name = "OSApplicationProcess";



//Class
IPCPObj::IPCPObj(std::string name, long instance, int ipcp_id)
	: RIBObject<mad_manager::structures::ipcp_t>(class_name, instance, name,
				(mad_manager::structures::ipcp_t*) NULL,
				&encoder),
				processID_(ipcp_id){

}

IPCPObj::IPCPObj(std::string name, long instance,
		const rina::cdap_rib::SerializedObject &object_value)
	: RIBObject<mad_manager::structures::ipcp_t>(class_name, instance, name,
					(mad_manager::structures::ipcp_t*) NULL,
					&encoder){

	mad_manager::structures::ipcp_t object;
	encoder.decode(object_value, object);
	processID_ = object.process_id;
}

rina::cdap_rib::res_info_t* IPCPObj::remoteRead(
				const std::string& name,
				rina::cdap_rib::SerializedObject &obj_reply){

	(void) name;
	rina::cdap_rib::res_info_t* r = new rina::cdap_rib::res_info_t;
	r->result_ = 0;

	mad_manager::structures::ipcp_t info;
	info.process_id = processID_;
	info.name = IPCManager->get_ipcp_name(processID_);
	//TODO: Add missing stuff...

	encoder_->encode(info, obj_reply);
	return r;
}

//We only support deletion
rina::cdap_rib::res_info_t* IPCPObj::remoteDelete(const std::string& name){

	(void) name;
	rina::cdap_rib::res_info_t* r = new rina::cdap_rib::res_info_t;

	//Fill in the response
	r->result_ = 0;

	//Call the IPCManager and return
	if (IPCManager->destroy_ipcp(processID_) != IPCM_SUCCESS) {
		LOG_ERR("Unable to destroy IPCP with id %d", processID_);
		r->result_ = -1;
	}

	return r;
}

OSApplicationProcessObj::OSApplicationProcessObj(std::string name,
		long instance, rina::rib::RIBDNorthInterface* ribd)
	: rina::rib::EmptyRIBObject(class_name, name, instance, &encoder_){

	ribd_ = ribd;
}

rina::cdap_rib::res_info_t* OSApplicationProcessObj::remoteCreate(
		const std::string& name, const std::string clas,
		const rina::cdap_rib::SerializedObject &obj_req,
		rina::cdap_rib::SerializedObject &obj_reply){

	(void) obj_reply;
	rina::cdap_rib::res_info_t* res = new rina::cdap_rib::res_info_t;

	if (clas == IPCPObj::class_name) {

		IPCPObj* ipcp;

		try{
			ipcp = new IPCPObj(name, inst_gen->next(), obj_req);
		}catch(...){
			LOG_ERR("Unable to create an IPCP object '%s'; out of memory?",
								name.c_str());
			res->result_ = -1;
			return res;
		}

		ribd_->addRIBObject(ipcp);
		res->result_ = 1;
	} else {
		LOG_ERR("Create object %s is not implemented as a child of OSApplicationProcess object",
								name.c_str());
		res->result_ = -1;
	}

	return res;
}

};//namespace rib_v1
};//namespace mad
};//namespace rinad

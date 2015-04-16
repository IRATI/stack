#include <os_proc_obj.h>
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
const std::string OSApplicationProcessObj::class_name = "OSApplicationProcess";

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

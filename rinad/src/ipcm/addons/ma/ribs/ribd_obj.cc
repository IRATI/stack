#include <ribd_obj.h>

#define RINA_PREFIX "ipcm.mad.ribd_v1"
#include <librina/logs.h>
#include <librina/exceptions.h>

//Encoders and structs
#include "encoders_mad.h"

#include "../agent.h"
#include "../../../ipcm.h"

namespace rinad {
namespace mad {
namespace rib_v1 {

//Static class names
const std::string RIBDaemonObj::class_name = "RIBDaemon";

RIBDaemonObj::RIBDaemonObj(std::string name,
		long instance, int ipcp_id_)
	: rina::rib::EmptyRIBObject(class_name, name, instance, NULL),
	ipcp_id(ipcp_id_){
}

//Read
rina::cdap_rib::res_info_t* RIBDaemonObj::remoteRead(const std::string& name,
		rina::cdap_rib::SerializedObject &obj_reply){

	(void) name;
	QueryRIBPromise promise;
	rina::cdap_rib::res_info_t* r = new rina::cdap_rib::res_info_t;
	r->result_ = 0;

	//Perform the query
	if (IPCManager->query_rib(ManagementAgent::inst, &promise, ipcp_id) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS){
		r->result_ = -1;
		return r;
	}

	// FIXME cut the query rib to avoid oversizing the shim eth buffer. Erase when spliting is implemented
	std::string trunk = promise.serialized_rib.substr(0, 1000);

	//Serialize and return
	mad_manager::encoders::StringEncoder encoder;
	encoder.encode(trunk, obj_reply);

	return r;
}



};//namespace rib_v1
};//namespace mad
};//namespace rinad

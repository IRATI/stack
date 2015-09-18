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

RIBDaemonObj::RIBDaemonObj(int ipcp_id_)
	: rina::rib::RIBObj(class_name),
	ipcp_id(ipcp_id_){
}

//Read
void RIBDaemonObj::read(const rina::cdap_rib::con_handle_t &con,
				const std::string& fqn,
				const std::string& class_,
				const rina::cdap_rib::filt_info_t &filt,
				const int invoke_id,
				rina::cdap_rib::SerializedObject &obj_reply,
				rina::cdap_rib::res_info_t& res) {

	QueryRIBPromise promise;

	res.code_ = rina::cdap_rib::CDAP_SUCCESS;

	//Perform the query
	if (IPCManager->query_rib(ManagementAgent::inst, &promise, ipcp_id) == IPCM_FAILURE ||
			promise.wait() != IPCM_SUCCESS){
		res.code_ = rina::cdap_rib::CDAP_ERROR;
		return;
	}

	// FIXME cut the query rib to avoid oversizing the shim eth buffer. Erase when spliting is implemented
	std::string trunk = promise.serialized_rib.substr(0, 1000);

	//Serialize and return
	mad_manager::encoders::StringEncoder encoder;
	encoder.encode(trunk, obj_reply);
}



};//namespace rib_v1
};//namespace mad
};//namespace rinad

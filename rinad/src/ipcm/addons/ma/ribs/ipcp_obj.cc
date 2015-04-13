#include <ipcp_obj.h>

#define RINA_PREFIX "ipcm.mad.ribd_v1"
#include <librina/logs.h>
#include <librina/exceptions.h>

//Include the GPB
#include "common/encoders/MA-IPCP.pb.h"

#include "../../../ipcm.h"

namespace rinad {
namespace mad {
namespace rib_v1 {

//Instance generator
extern Singleton<rina::ConsecutiveUnsignedIntegerGenerator> inst_gen;

const std::string IPCPObj::class_name = "IPCProcess";

//Encoder
void IPCPEncoder::encode(const ipcp_msg_t &obj,
		rina::cdap_rib::SerializedObject& serobj){

	mad_manager::messages::ipcp m;
	m.set_processid(obj.process_id);
	//TODO add name

	//Allocate memory
	serobj.size_ = m.ByteSize();
	serobj.message_ = new char[serobj.size_];

	if (!serobj.message_)
		throw rina::Exception("out of memory");  //TODO improve this

	//Serialize and return
	m.SerializeToArray(serobj.message_, serobj.size_);
}

void IPCPEncoder::decode(const rina::cdap_rib::SerializedObject &serobj,
		ipcp_msg_t& des_obj){

	mad_manager::messages::ipcp m;
	if (!m.ParseFromArray(serobj.message_, serobj.size_))
		throw rina::Exception("Could not be parsed");

	des_obj.process_id = m.processid();
}

//Class
IPCPObj::IPCPObj(std::string name, long instance, int ipcp_id)
	: RIBObject<ipcp_msg_t>(class_name, instance, name, (ipcp_msg_t*) NULL,
			&encoder),
	processID_(ipcp_id){

}

IPCPObj::IPCPObj(std::string name, long instance,
		const rina::cdap_rib::SerializedObject &object_value)
	: RIBObject<ipcp_msg_t>(class_name, instance, name, (ipcp_msg_t*) NULL,
			&encoder){

	ipcp_msg_t object;
	encoder.decode(object_value, object);
	processID_ = object.process_id;
}

rina::cdap_rib::res_info_t* IPCPObj::remoteRead(
				const std::string& name,
				rina::cdap_rib::SerializedObject &obj_reply){

	(void) name;
	rina::cdap_rib::res_info_t* r = new rina::cdap_rib::res_info_t;
	r->result_ = 0;

	ipcp_msg_t info;
	info.process_id = processID_;
	//TODO add name

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

OSApplicationProcessObj::OSApplicationProcessObj(
		const std::string& clas, std::string name, long instance,
		rina::rib::RIBDNorthInterface* ribd)
	: rina::rib::EmptyRIBObject(clas, name, instance, &encoder_){

	ribd_ = ribd;
}

rina::cdap_rib::res_info_t* OSApplicationProcessObj::remoteCreate(
		const std::string& name, const std::string clas,
		const rina::cdap_rib::SerializedObject &obj_req,
		rina::cdap_rib::SerializedObject &obj_reply){

	rina::cdap_rib::res_info_t* res = new rina::cdap_rib::res_info_t;
	if (clas == IPCPObj::class_name) {
		ribd_->addRIBObject(new IPCPObj(name, inst_gen->next(),
								obj_req));
		res->result_ = 1;
		(void) obj_reply;
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

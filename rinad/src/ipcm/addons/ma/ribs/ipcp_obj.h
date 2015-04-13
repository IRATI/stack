/**
 * @file ribdv1.h
 * @author Bernat Gaston <bernat (dot) gaston (at) i2cat (dot) net>
 *
 * @brief Management Agent RIB daemon v1
 */


#ifndef __RINAD_IPCP_OBJ_H__
#define __RINAD_IPCP_OBJ_H__

#include <librina/rib_v2.h>
#include <librina/patterns.h>
#include <librina/common.h>

namespace rinad{
namespace mad{
namespace rib_v1{

//fwd decl
class IPCPObj;


//Struct containing the message
//It is a bit redundant, but hold GPB in the encoder only
typedef struct ipcp_msg{
	int32_t process_id;
	std::string name;
	//TODO: add missing info
}ipcp_msg_t;

/**
 * Encoder
 */
class IPCPEncoder : public rina::rib::Encoder<ipcp_msg_t>{
public:
	virtual void encode(const ipcp_msg_t &obj,
			rina::cdap_rib::SerializedObject& serobj);
	virtual void decode(const rina::cdap_rib::SerializedObject &serobj,
			ipcp_msg_t& des_obj);

	virtual std::string get_type() const{ return "ipcp"; };
};




/**
 * IPCP object
 */
class IPCPObj : public rina::rib::RIBObject<ipcp_msg_t>{

public:
	IPCPObj(std::string name, long instance, int ipcp_id);
	IPCPObj(std::string name, long instance,
			const rina::cdap_rib::SerializedObject &object_value);
	virtual ~IPCPObj(){};

	//Read
	rina::cdap_rib::res_info_t* remoteRead(const std::string& name,
			rina::cdap_rib::SerializedObject &obj_reply);

	//Deletion
	rina::cdap_rib::res_info_t* remoteDelete(
			const std::string& name);

	//Name of the class
	const static std::string class_name;

	//Process ID
	int processID_;

private:
	IPCPEncoder encoder;
};


/**
 * OSApplicationProcess object
 */
class OSApplicationProcessObj : public rina::rib::EmptyRIBObject{

public:
	OSApplicationProcessObj(const std::string& clas, std::string name,
			long instance, rina::rib::RIBDNorthInterface* ribd);
	rina::cdap_rib::res_info_t* remoteCreate(const std::string& name,
			const std::string clas,
			const rina::cdap_rib::SerializedObject &obj_req,
			rina::cdap_rib::SerializedObject &obj_reply);
private:
	rina::rib::RIBDNorthInterface* ribd_;
	rina::rib::EmptyEncoder encoder_;
};

}; //namespace rib_v1
}; //namespace mad
}; //namespace rinad


#endif  /* __RINAD_IPCP_OBJ_H__ */

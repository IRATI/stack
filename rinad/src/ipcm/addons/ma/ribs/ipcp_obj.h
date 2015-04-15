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

//Encoders and structs
#include "encoders_mad.h"

namespace rinad{
namespace mad{
namespace rib_v1{

//fwd decl
class IPCPObj;

/**
 * IPCP object
 */
class IPCPObj : public rina::rib::RIBObject
					<mad_manager::structures::ipcp_t>{

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
	mad_manager::encoders::IPCPEncoder encoder;
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

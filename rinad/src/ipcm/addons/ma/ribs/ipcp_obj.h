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
#include "configuration.h"
#include "encoder.h"

namespace rinad{
namespace mad{
namespace rib_v1{


/**
 * IPCP object
 */
class IPCPObj : public rina::rib::DelegationObj{

public:
	IPCPObj(int ipcp_id);
	virtual ~IPCPObj() throw() {};

	const std::string& get_class() const{
		return class_name;
	};

	const std::string get_displayable_value() const;

	//Read
	void read(const rina::cdap_rib::con_handle_t &con,
				const std::string& fqn,
				const std::string& class_,
				const rina::cdap_rib::filt_info_t &filt,
				const int invoke_id,
				rina::ser_obj_t &obj_reply,
				rina::cdap_rib::res_info_t& res);


	//Deletion
	bool delete_(const rina::cdap_rib::con_handle_t &con,
				const std::string& fqn,
				const std::string& class_,
				const rina::cdap_rib::filt_info_t &filt,
				const int invoke_id,
				rina::cdap_rib::res_info_t& res);

	void forward_object(const rina::cdap_rib::con_handle_t& con,
			    rina::cdap::cdap_m_t::Opcode op_code,
			    const std::string obj_name,
			    const std::string obj_class,
			    const rina::ser_obj_t &obj_value,
			    const rina::cdap_rib::flags_t &flags,
			    const rina::cdap_rib::filt_info_t &filt,
			    const int invoke_id);

	//Create callback
	static void create_cb(const rina::rib::rib_handle_t rib,
			const rina::cdap_rib::con_handle_t &con,
			const std::string& fqn,
			const std::string& class_,
			const rina::cdap_rib::filt_info_t &filt,
			const int invoke_id,
			const rina::ser_obj_t &obj_req,
			rina::cdap_rib::obj_info_t &obj_reply,
			rina::cdap_rib::res_info_t& res);


	//Name of the class
	const static std::string class_name;

	//Process ID
	int processID_;

protected:
	static int createIPCP(rinad::configs::ipcp_config_t &object);
	static bool assignToDIF(rinad::configs::ipcp_config_t &object, int ipcp_id);
	static bool registerAtDIFs(rinad::configs::ipcp_config_t &object, int ipcp_id);
	static bool enrollToDIFs(rinad::configs::ipcp_config_t &object, int ipcp_id);
	struct Params{
	        rina::cdap_rib::con_handle_t con;
	        rina::cdap_rib::obj_info_t obj;
                rina::cdap_rib::flags_t flags;
                int invoke_id;
	};
};

}; //namespace rib_v1
}; //namespace mad
}; //namespace rinad


#endif  /* __RINAD_IPCP_OBJ_H__ */

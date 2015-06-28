/**
 * @file ribdv1.h
 * @author Bernat Gaston <bernat (dot) gaston (at) i2cat (dot) net>
 *
 * @brief Management Agent RIB daemon v1 - RIBDaemon
 */


#ifndef __RINAD_RIBD_OBJ_H__
#define __RINAD_RIBD_OBJ_H__

#include <librina/rib_v2.h>
#include <librina/patterns.h>
#include <librina/common.h>

namespace rinad{
namespace mad{
namespace rib_v1{

/**
 * RIBDaemon object
 * XXX FIXME TODO: this is a temporally defined object. This object belongs
 * to IPCP's subRIB, and as such, should be delegated to it.
 *
 * Just adding it to support Query RIB temporally.
 */
class RIBDaemonObj : public rina::rib::RIBObj{

public:
	RIBDaemonObj(int ipcp_id);

	//Read
	void read(const rina::cdap_rib::con_handle_t &con,
				const std::string& fqn,
				const std::string& class_,
				const rina::cdap_rib::filt_info_t &filt,
				const int invoke_id,
				rina::cdap_rib::SerializedObject &obj_reply,
				rina::cdap_rib::res_info_t& res);

private:
	//Should not be necessary, could be parsed by the name
	int ipcp_id;

	static const std::string class_name;
};

}; //namespace rib_v1
}; //namespace mad
}; //namespace rinad


#endif  /* __RINAD_RIBD_OBJ_H__ */

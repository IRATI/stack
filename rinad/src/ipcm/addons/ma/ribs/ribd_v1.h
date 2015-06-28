/**
 * @file ribdv1.h
 * @author Bernat Gaston <bernat (dot) gaston (at) i2cat (dot) net>
 *
 * @brief Management Agent RIB daemon v1
 */

#ifndef __RINAD_RIBD_V1_H__
#define __RINAD_RIBD_V1_H__

#include <librina/rib_v2.h>
#include <librina/patterns.h>
#include <librina/common.h>

namespace rinad {
namespace mad {
namespace rib_v1 {

class RIBRespHandler_v1 : public rina::rib::RIBOpsRespHandlers {
	~RIBRespHandler_v1(){};

	void remoteCreateResult(const rina::cdap_rib::con_handle_t &con,
			const rina::cdap_rib::obj_info_t &obj,
			const rina::cdap_rib::res_info_t &res);
	void remoteDeleteResult(const rina::cdap_rib::con_handle_t &con,
			const rina::cdap_rib::res_info_t &res);
	void remoteReadResult(const rina::cdap_rib::con_handle_t &con,
			const rina::cdap_rib::obj_info_t &obj,
			const rina::cdap_rib::res_info_t &res);
	void remoteCancelReadResult(const rina::cdap_rib::con_handle_t &con,
			const rina::cdap_rib::res_info_t &res);
	void remoteWriteResult(const rina::cdap_rib::con_handle_t &con,
			const rina::cdap_rib::obj_info_t &obj,
			const rina::cdap_rib::res_info_t &res);
	void remoteStartResult(const rina::cdap_rib::con_handle_t &con,
			const rina::cdap_rib::obj_info_t &obj,
			const rina::cdap_rib::res_info_t &res);
	void remoteStopResult(const rina::cdap_rib::con_handle_t &con,
			const rina::cdap_rib::obj_info_t &obj,
			const rina::cdap_rib::res_info_t &res);
};

class RIBConHandler_v1 : public rina::cacep::AppConHandlerInterface {

	void connect(int message_id, const rina::cdap_rib::con_handle_t &con);
	void connectResult(const rina::cdap_rib::res_info_t &res,
				const rina::cdap_rib::con_handle_t &con);
	void release(int message_id, const rina::cdap_rib::con_handle_t &con);
	void releaseResult(const rina::cdap_rib::res_info_t &res,
				const rina::cdap_rib::con_handle_t &con);
};

///
/// Initialize the RIB (populate static objects)
///
void initRIB(const rina::rib::rib_handle_t& rib);

///
/// Add a new IPCP object due to an external creation (e.g. CLI)
///
void createIPCPObj(const rina::rib::rib_handle_t& rib, int ipcp_id);

///
/// Add a new IPCP object due to an external creation (e.g. CLI)
///
void destroyIPCPObj(const rina::rib::rib_handle_t& rib, int ipcp_id);

}  //namespace rib_v1
}  //namespace mad
}  //namespace rinad

#endif  /* __RINAD_RIBD_V1_H__ */

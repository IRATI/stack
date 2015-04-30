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


namespace rinad{
namespace mad{
namespace rib_v1{

class RIBRespHandler_v1 : public rina::rib::ResponseHandlerInterface
{
  void createResponse(const rina::cdap_rib::res_info_t &res,
                      const rina::cdap_rib::obj_info_t &obj,
                      const rina::cdap_rib::con_handle_t &con);
  void deleteResponse(const rina::cdap_rib::res_info_t &res,
                      const rina::cdap_rib::con_handle_t &con);
  void readResponse(const rina::cdap_rib::res_info_t &res,
                    const rina::cdap_rib::obj_info_t &obj,
                    const rina::cdap_rib::con_handle_t &con);
  void cancelReadResponse(const rina::cdap_rib::res_info_t &res,
                          const rina::cdap_rib::con_handle_t &con);
  void writeResponse(const rina::cdap_rib::res_info_t &res,
                     const rina::cdap_rib::obj_info_t &obj,
                     const rina::cdap_rib::con_handle_t &con);
  void startResponse(const rina::cdap_rib::res_info_t &res,
                     const rina::cdap_rib::obj_info_t &obj,
                     const rina::cdap_rib::con_handle_t &con);
  void stopResponse(const rina::cdap_rib::res_info_t &res,
                    const rina::cdap_rib::obj_info_t &obj,
                    const rina::cdap_rib::con_handle_t &con);
};

class RIBConHandler_v1 : public rina::cacep::AppConHandlerInterface{

	void connect(int message_id, const rina::cdap_rib::con_handle_t &con);
	void connectResponse(const rina::cdap_rib::res_info_t &res,
			const rina::cdap_rib::con_handle_t &con);
	void release(int message_id, const rina::cdap_rib::con_handle_t &con);
	void releaseResponse(const rina::cdap_rib::res_info_t &res,
			const rina::cdap_rib::con_handle_t &con);
};

//TODO: remove this
void initiateRIB(rina::rib::RIBDNorthInterface* ribd);
void createIPCPObject(rina::rib::RIBDNorthInterface &ribd, int ipcp_id);
void destroyIPCPObject(rina::rib::RIBDNorthInterface &ribd, int ipcp_id);
};//namespace rib_v1
};//namespace mad
};//namespace rinad

#endif  /* __RINAD_RIBD_V1_H__ */

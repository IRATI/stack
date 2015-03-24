/**
* @file ribdv1.h
* @author Bernat Gaston <bernat (dot) gaston (at) i2cat (dot) net>
*
* @brief Management Agent RIB daemon v1
*/


#ifndef __RINAD_RIBD_V1_H__
#define __RINAD_RIBD_V1_H__

#include "rib_provider.h"
#include <librina/patterns.h>

namespace rinad{
namespace mad{

class RespHandler : public rib::ResponseHandlerInterface
{
  void createResponse(const cdap_rib::res_info_t &res,
                      const cdap_rib::con_handle_t &con);
  void deleteResponse(const cdap_rib::res_info_t &res,
                      const cdap_rib::con_handle_t &con);
  void readResponse(const cdap_rib::res_info_t &res,
                    const cdap_rib::con_handle_t &con);
  void cancelReadResponse(const cdap_rib::res_info_t &res,
                          const cdap_rib::con_handle_t &con);
  void writeResponse(const cdap_rib::res_info_t &res,
                     const cdap_rib::con_handle_t &con);
  void startResponse(const cdap_rib::res_info_t &res,
                     const cdap_rib::con_handle_t &con);
  void stopResponse(const cdap_rib::res_info_t &res,
                    const cdap_rib::con_handle_t &con);
};

class ConHandler : public cacep::AppConHandlerInterface
{
  void connect(int message_id, const cdap_rib::con_handle_t &con);
  void connectResponse(const cdap_rib::res_info_t &res,
                       const cdap_rib::con_handle_t &con);
  void release(int message_id, const cdap_rib::con_handle_t &con);
  void releaseResponse(const cdap_rib::res_info_t &res,
                       const cdap_rib::con_handle_t &con);
};

class InstanceGenerator
{
 public:
  InstanceGenerator()
  {
    id_ = 0;
  }
  long get_id()
  {
    id_++;
    return id_;
  }
 private:
  long id_;
};


class RIBDaemonv1_
{
  friend class Singleton<RIBDaemonv1_>;
 public:
  rib::ResponseHandlerInterface* getRespHandler();
  cacep::AppConHandlerInterface* getConnHandler();
  void initiateRIB(rib::RIBDNorthInterface* ribd);
 private:
  RIBDaemonv1_();
  ~RIBDaemonv1_();
  rib::ResponseHandlerInterface* resp_handler_;
  cacep::AppConHandlerInterface* conn_handler_;
  InstanceGenerator intance_gen_;
};

//Singleton instance
extern Singleton<RIBDaemonv1_> RIBDaemonv1;

}; //namespace mad
}; //namespace rinad


#endif  /* __RINAD_RIBD_V1_H__ */

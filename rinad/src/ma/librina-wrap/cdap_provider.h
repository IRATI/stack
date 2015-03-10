/*
 * CDAP North bound API
 *
 *    Bernat Gastón <bernat.gaston@i2cat.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

#ifndef CDAP_PROVIDER_H_
#define CDAP_PROVIDER_H_
#include <string>
#include <librina/concurrency.h>
#include "cdap_rib_structures.h"

namespace cdap {

class CDAPProviderInterface
{
 public:
  virtual ~CDAPProviderInterface()
  {
  }
  ;
  virtual cdap_rib::con_handle_t open_connection(
      const cdap_rib::vers_info_t ver, const cdap_rib::src_info_t &src,
      const cdap_rib::dest_info_t &dest, const cdap_rib::auth_info &auth,
      int port) = 0;
  virtual int close_connection(cdap_rib::con_handle_t &con) = 0;
  virtual int remote_create(const cdap_rib::con_handle_t &con,
                            const cdap_rib::obj_info_t &obj,
                            const cdap_rib::flags_t &flags,
                            const cdap_rib::filt_info_t &filt) = 0;
  virtual int remote_delete(const cdap_rib::con_handle_t &con,
                            const cdap_rib::obj_info_t &obj,
                            const cdap_rib::flags_t &flags,
                            const cdap_rib::filt_info_t &filt) = 0;
  virtual int remote_read(const cdap_rib::con_handle_t &con,
                          const cdap_rib::obj_info_t &obj,
                          const cdap_rib::flags_t &flags,
                          const cdap_rib::filt_info_t &filt)= 0;
  virtual int remote_cancel_read(const cdap_rib::con_handle_t &con,
                                 const cdap_rib::flags_t &flags,
                                 int invoke_id) = 0;
  virtual int remote_write(const cdap_rib::con_handle_t &con,
                           const cdap_rib::obj_info_t &obj,
                           const cdap_rib::flags_t &flags,
                           const cdap_rib::filt_info_t &filt) = 0;
  virtual int remote_start(const cdap_rib::con_handle_t &con,
                           const cdap_rib::obj_info_t &obj,
                           const cdap_rib::flags_t &flags,
                           const cdap_rib::filt_info_t &filt) = 0;
  virtual int remote_stop(const cdap_rib::con_handle_t &con,
                          const cdap_rib::obj_info_t &obj,
                          const cdap_rib::flags_t &flags,
                          const cdap_rib::filt_info_t &filt) = 0;
  virtual void remote_create_response(const cdap_rib::con_handle_t &con,
                                      const cdap_rib::obj_info_t &obj,
                                      const cdap_rib::flags_t &flags,
                                      const cdap_rib::res_info_t &res,
                                      int message_id) = 0;
  virtual void remote_delete_response(const cdap_rib::con_handle_t &con,
                                      const cdap_rib::obj_info_t &obj,
                                      const cdap_rib::flags_t &flags,
                                      const cdap_rib::res_info_t &res,
                                      int message_id) = 0;
  virtual void remote_read_response(const cdap_rib::con_handle_t &con,
                                    const cdap_rib::obj_info_t &obj,
                                    const cdap_rib::flags_t &flags,
                                    const cdap_rib::res_info_t &res,
                                    int message_id) = 0;
  virtual void remote_cancel_read_response(const cdap_rib::con_handle_t &con,
                                           const cdap_rib::flags_t &flags,
                                           const cdap_rib::res_info_t &res,
                                           int message_id) = 0;
  virtual void remote_write_response(const cdap_rib::con_handle_t &con,
                                     const cdap_rib::flags_t &flags,
                                     const cdap_rib::res_info_t &res,
                                     int message_id) = 0;
  virtual void remote_start_response(const cdap_rib::con_handle_t &con,
                                     const cdap_rib::obj_info_t &obj,
                                     const cdap_rib::flags_t &flags,
                                     const cdap_rib::res_info_t &res,
                                     int message_id) = 0;
  virtual void remote_stop_response(const cdap_rib::con_handle_t &con,
                                    const cdap_rib::flags_t &flags,
                                    const cdap_rib::res_info_t &res,
                                    int message_id) = 0;
};

class IPCPCDAPProviderInterface: public CDAPProviderInterface{

};

class CDAPProviderFactory
{
 public:
  CDAPProviderInterface* create(const std::string &comm_protocol, long timeout, bool is_IPCP);
};

}

#endif /* CDAP_PROVIDER_H_ */

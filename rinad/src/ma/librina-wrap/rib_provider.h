/*
 * RIB South bound API
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
#ifndef RIB_PROVIDER_H_
#define RIB_PROVIDER_H_
#include <string>
#include "cdap_rib_structures.h"

namespace rib {

class RIBProviderInterface
{
 public:
  virtual ~RIBProviderInterface()
  {
  }
  ;
  virtual void open_connection_result(const cdap_rib::con_handle_t &con,
                                      const cdap_rib::result_info &res) = 0;
  virtual void close_connection_result(const cdap_rib::con_handle_t &con,
                                       const cdap_rib::result_info &res) = 0;

  virtual void remote_create_result(const cdap_rib::con_handle_t &con,
                                    const cdap_rib::obj_info_t &obj,
                                    const cdap_rib::res_info_t &res,
                                    int message_id) = 0;
  virtual void remote_delete_result(const cdap_rib::con_handle_t &con,
                                    const cdap_rib::obj_info_t &obj,
                                    const cdap_rib::res_info_t &res,
                                    int message_id) = 0;
  virtual void remote_read_result(const cdap_rib::con_handle_t &con,
                                  const cdap_rib::obj_info_t &obj,
                                  const cdap_rib::res_info_t &res,
                                  int message_id) = 0;
  virtual void remote_cancel_read_result(const cdap_rib::con_handle_t &con,
                                         const cdap_rib::obj_info_t &obj,
                                         const cdap_rib::res_info_t &res,
                                         int message_id);
  virtual void remote_write_result(const cdap_rib::con_handle_t &con,
                                   const cdap_rib::obj_info_t &obj,
                                   const cdap_rib::res_info_t &res,
                                   int message_id) = 0;
  virtual void remote_start_result(const cdap_rib::con_handle_t &con,
                                   const cdap_rib::obj_info_t &obj,
                                   const cdap_rib::res_info_t &res,
                                   int message_id) = 0;
  virtual void remote_stop_result(const cdap_rib::con_handle_t &con,
                                  const cdap_rib::obj_info_t &obj,
                                  const cdap_rib::res_info_t &res,
                                  int message_id) = 0;

  virtual void remote_create_request(const cdap_rib::con_handle_t &con,
                                     const cdap_rib::obj_info_t &obj,
                                     const cdap_rib::filt_info_t &filt) = 0;
  virtual void remote_delete_request(const cdap_rib::con_handle_t &con,
                                     const cdap_rib::obj_info_t &obj,
                                     const cdap_rib::filt_info_t &filt) = 0;
  virtual void remote_read_request(const cdap_rib::con_handle_t &con,
                                   const cdap_rib::obj_info_t &obj,
                                   const cdap_rib::filt_info_t &filt) = 0;
  virtual void remote_cancel_read_request(
      const cdap_rib::con_handle_t &con, const cdap_rib::obj_info_t &obj,
      const cdap_rib::filt_info_t &filt) = 0;
  virtual void remote_write_request(const cdap_rib::con_handle_t &con,
                                    const cdap_rib::obj_info_t &obj,
                                    const cdap_rib::filt_info_t &filt) = 0;
  virtual void remote_start_request(const cdap_rib::con_handle_t &con,
                                    const cdap_rib::obj_info_t &obj,
                                    const cdap_rib::filt_info_t &filt) = 0;
  virtual void remote_stop_request(const cdap_rib::con_handle_t &con,
                                   const cdap_rib::obj_info_t &obj,
                                   const cdap_rib::filt_info_t &filt) = 0;
};
}

#endif /* RIB_PROVIDER_H_ */

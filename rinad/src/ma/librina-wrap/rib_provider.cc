/*
 * RIB South bound API
 *
 *    Bernat Gastón <bernat.gaston@i2cat.net>
 *
 * This library is free software{} you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation{} either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY{} without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library{} if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

#include "rib_provider.h"

namespace rib {

void open_connection_result(const con_handle_t &con, const result_info &res)
{
  (void) con;
  (void) res;
}
void close_connection_result(const con_handle_t &con, const result_info &res)
{
  (void) con;
  (void) res;
}

void remote_create_result(const con_handle_t &con, const obj_info_t &obj,
                          const res_info_t &res, int message_id)
{
  (void) con;
  (void) obj;
  (void) res;
  (void) message_id;
}
void remote_delete_result(const con_handle_t &con, const obj_info_t &obj,
                          const res_info_t &res, int message_id)
{
  (void) con;
  (void) obj;
  (void) res;
  (void) message_id;
}
void remote_read_result(const con_handle_t &con, const obj_info_t &obj,
                        const res_info_t &res, int message_id)
{
  (void) con;
  (void) obj;
  (void) res;
  (void) message_id;
}
void remote_cancel_read_result(const con_handle_t &con, const obj_info_t &obj,
                               const res_info_t &res, int message_id)
{
  (void) con;
  (void) obj;
  (void) res;
  (void) message_id;
}
void remote_write_result(const con_handle_t &con, const obj_info_t &obj,
                         const res_info_t &res, int message_id)
{
  (void) con;
  (void) obj;
  (void) res;
  (void) message_id;
}
void remote_start_result(const con_handle_t &con, const obj_info_t &obj,
                         const res_info_t &res, int message_id)
{
  (void) con;
  (void) obj;
  (void) res;
  (void) message_id;
}
void remote_stop_result(const con_handle_t &con, const obj_info_t &obj,
                        const res_info_t &res, int message_id)
{
  (void) con;
  (void) obj;
  (void) res;
  (void) message_id;
}

void remote_create_request(const con_handle_t &con, const obj_info_t &obj,
                           const filt_info_t &filt)
{
  (void) con;
  (void) obj;
  (void) filt;
}
void remote_delete_request(const con_handle_t &con, const obj_info_t &obj,
                           const filt_info_t &filt)
{
  (void) con;
  (void) obj;
  (void) filt;
}
void remote_read_request(const con_handle_t &con, const obj_info_t &obj,
                         const filt_info_t &filt)
{
  (void) con;
  (void) obj;
  (void) filt;
}
void remote_cancel_read_request(const con_handle_t &con, const obj_info_t &obj,
                                const filt_info_t &filt)
{
  (void) con;
  (void) obj;
  (void) filt;
}
void remote_write_request(const con_handle_t &con, const obj_info_t &obj,
                          const filt_info_t &filt)
{
  (void) con;
  (void) obj;
  (void) filt;
}
void remote_start_request(const con_handle_t &con, const obj_info_t &obj,
                          const filt_info_t &filt)
{
  (void) con;
  (void) obj;
  (void) filt;
}
void remote_stop_request(const con_handle_t &con, const obj_info_t &obj,
                         const filt_info_t &filt)
{
  (void) con;
  (void) obj;
  (void) filt;
}
}


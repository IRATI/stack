/*
 * CDAP North bound API
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

#include "cdap_provider.h"

namespace cdap {
con_handle_t CDAPProvider::open_connection(const vers_info_t ver,
                                           const src_info_t &src,
                                           const dest_info_t &dest,
                                           const auth_info &auth)
{
  con_handle_t hand;
  (void) ver;
  (void) src;
  (void) dest;
  (void) auth;
  return hand;
}
int CDAPProvider::close_connection(con_handle_t &con)
{
  (void) con;
  return 0;
}
int CDAPProvider::remote_create(const con_handle_t &con, const obj_info_t &obj,
                                const flags_t &flags, const filt_info_t &filt)
{
  (void) con;
  (void) obj;
  (void) flags;
  (void) filt;
  return 0;
}
int CDAPProvider::remote_delete(const con_handle_t &con, const obj_info_t &obj,
                                const flags_t &flags, const filt_info_t &filt)
{
  (void) con;
  (void) obj;
  (void) flags;
  (void) filt;
  return 0;
}
int CDAPProvider::remote_read(const con_handle_t &con, const obj_info_t &obj,
                              const flags_t &flags, const filt_info_t &filt)
{
  (void) con;
  (void) obj;
  (void) flags;
  (void) filt;
  return 0;
}
int CDAPProvider::remote_cancel_read(const con_handle_t &con,
                                     const obj_info_t &obj,
                                     const flags_t &flags,
                                     const filt_info_t &filt)
{
  (void) con;
  (void) obj;
  (void) flags;
  (void) filt;
  return 0;
}
int CDAPProvider::remote_write(const con_handle_t &con, const obj_info_t &obj,
                               const flags_t &flags, const filt_info_t &filt)
{
  (void) con;
  (void) obj;
  (void) flags;
  (void) filt;
  return 0;
}
int CDAPProvider::remote_start(const con_handle_t &con, const obj_info_t &obj,
                               const flags_t &flags, const filt_info_t &filt)
{
  (void) con;
  (void) obj;
  (void) flags;
  (void) filt;
  return 0;
}
int CDAPProvider::remote_stop(const con_handle_t &con, const obj_info_t &obj,
                              const flags_t &flags, const filt_info_t &filt)
{
  (void) con;
  (void) obj;
  (void) flags;
  (void) filt;
  return 0;
}
void CDAPProvider::remote_create_response(const con_handle_t &con,
                                          const obj_info_t &obj,
                                          const flags_t &flags,
                                          const res_info_t &res, int message_id)
{
  (void) con;
  (void) obj;
  (void) flags;
  (void) res;
  (void) message_id;
}
void CDAPProvider::remote_delete_response(const con_handle_t &con,
                                          const obj_info_t &obj,
                                          const flags_t &flags,
                                          const res_info_t &res, int message_id)
{
  (void) con;
  (void) obj;
  (void) flags;
  (void) res;
  (void) message_id;
}
void CDAPProvider::remote_read_response(const con_handle_t &con,
                                        const obj_info_t &obj,
                                        const flags_t &flags,
                                        const res_info_t &res, int message_id)
{
  (void) con;
  (void) obj;
  (void) flags;
  (void) res;
  (void) message_id;
}
void CDAPProvider::remote_cancel_read_response(const con_handle_t &con,
                                               const obj_info_t &obj,
                                               const flags_t &flags,
                                               const res_info_t &res,
                                               int message_id)
{
  (void) con;
  (void) obj;
  (void) flags;
  (void) res;
  (void) message_id;
}
void CDAPProvider::remote_write_response(const con_handle_t &con,
                                         const obj_info_t &obj,
                                         const flags_t &flags,
                                         const res_info_t &res, int message_id)
{
  (void) con;
  (void) obj;
  (void) flags;
  (void) res;
  (void) message_id;
}
void CDAPProvider::remote_start_response(const con_handle_t &con,
                                         const obj_info_t &obj,
                                         const flags_t &flags,
                                         const res_info_t &res, int message_id)
{
  (void) con;
  (void) obj;
  (void) flags;
  (void) res;
  (void) message_id;
}
void CDAPProvider::remote_stop_response(const con_handle_t &con,
                                        const obj_info_t &obj,
                                        const flags_t &flags,
                                        const res_info_t &res, int message_id)
{
  (void) con;
  (void) obj;
  (void) flags;
  (void) res;
  (void) message_id;
}

}

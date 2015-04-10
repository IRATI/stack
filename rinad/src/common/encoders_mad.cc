/*
 * Common interfaces and classes for encoding/decoding RIB objects for MAD-Manager communication
 *
 *    Bernat Gaston <bernat.gaston@i2cat.net>
 *    Marc Suñe <marc.sune@bisdn.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "encoders_mad.h"
#include "common/encoders/IpcpConfig.pb.h"

namespace mad_manager {
namespace encoders {
/// Encoder of IPCPConfig
void IPCPConfigEncoder::encode (const structures::ipcp_config_t &obj, rina::cdap_rib::ser_obj_t& ser_obj)
{
  messages::ipcp_config gpf_obj;
  gpf_obj.set_process_name(obj.process_name);
  gpf_obj.set_process_instance(obj.process_instance);
  gpf_obj.set_process_type(obj.process_type);
  gpf_obj.set_dif_to_register(obj.dif_to_register);
  gpf_obj.set_dif_to_assign(obj.dif_to_assign);
  if (ser_obj.size_ != 0 && ser_obj.message_ != 0)
  {
    delete ser_obj.message_;
    ser_obj.size_ = 0;
  }
  gpf_obj.SerializeToArray(ser_obj.message_, ser_obj.size_);

}
void IPCPConfigEncoder::decode(const rina::cdap_rib::ser_obj_t &ser_obj,
                    structures::ipcp_config_t& obj)
{
  messages::ipcp_config gpf_obj;
  gpf_obj.ParseFromArray(ser_obj.message_, ser_obj.size_);
  obj.process_name = gpf_obj.process_name();
  obj.process_instance = gpf_obj.process_instance();
  obj.process_type = gpf_obj.process_type();
  obj.dif_to_register = gpf_obj.dif_to_register();
  obj.dif_to_assign = gpf_obj.dif_to_assign();
}
std::string IPCPConfigEncoder::get_type() const
{
  return "ipcp_config_t";
}

}
}

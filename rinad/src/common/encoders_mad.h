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
#ifndef ENCODERS_MAD_H_
#define ENCODERS_MAD_H_

#include <librina/rib_v2.h>

namespace mad_manager {
namespace structures{
typedef struct IPCPConfig{
  std::string process_name;
  unsigned int process_instance;
  std::string process_type;
}ipcp_config_t;
}

namespace encoders {
/// Encoder of IPCPConfig
class IPCPConfigEncoder: public rina::rib::Encoder<structures::ipcp_config_t> {
public:
  void encode (const structures::ipcp_config_t &obj, rina::cdap_rib::ser_obj_t& ser_obj);
  void decode(const rina::cdap_rib::ser_obj_t &ser_obj,
                      structures::ipcp_config_t& obj);
  std::string get_type() const;
};
}
}
#endif /* ENCODERS_MAD_H_ */

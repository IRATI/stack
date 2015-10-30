/*
 * Common encoders for RIB objects for MAD-Manager communication
 *
 *    Bernat Gaston <bernat.gaston@i2cat.net>
 *    Marc Sune <marc.sune@bisdn.de>
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

#include <librina/cdap_rib_structures.h>
#include <librina/common.h>
#include "structures_mad.h"

namespace rinad {
namespace mad_manager {

// Generic (simple) types

/// String encoder
class StringEncoder : public Encoder<std::string>{
public:
	void encode(const std::string &obj,
			rina::ser_obj_t& serobj);
	void decode(const rina::ser_obj_t &serobj,
			std::string &des_obj);

	std::string get_type() const{ return "string"; };
};


/// Encoder of IPCPConfig
class IPCPConfigEncoder: public Encoder<ipcp_config_t> {

public:
	void encode (const ipcp_config_t &obj,
					rina::ser_obj_t& ser_obj);
	void decode(const rina::ser_obj_t &ser_obj,
			ipcp_config_t& obj);
	std::string get_type() const{ return "ipcp-config"; };
};


/// Encoder IPCP
class IPCPEncoder : public Encoder<ipcp_t>{
public:
	void encode(const ipcp_t &obj,
			rina::ser_obj_t& serobj);
	void decode(const rina::ser_obj_t &serobj,
			ipcp_t& des_obj);

	std::string get_type() const{ return "ipcp"; };
};

} //namespace mad_manager
} //namespace rinad

#endif /* ENCODERS_MAD_H_ */

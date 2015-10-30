/*
 * Common structures for MAD-Manager communication
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
#ifndef STRUCTURES_MAD_H_
#define STRUCTURES_MAD_H_

#include <inttypes.h>
#include <librina/cdap_rib_structures.h>

namespace rinad {
namespace mad_manager {

template<class T>
class Encoder{
public:
	virtual ~Encoder(){}
	/// Converts an object to a byte array, if this object is recognized by the encoder
	/// @param object
	/// @throws exception if the object is not recognized by the encoder
	/// @return
	virtual void encode(const T &obj, rina::ser_obj_t& serobj) = 0;
	/// Converts a byte array to an object of the type specified by "className"
	/// @param byte[] serializedObject
	/// @param objectClass The type of object to be decoded
	/// @throws exception if the byte array is not an encoded in a way that the
	/// encoder can recognize, or the byte array value doesn't correspond to an
	/// object of the type "className"
	/// @return
	virtual void decode(const rina::ser_obj_t &serobj,
			T& des_obj) = 0;
};

/**
* IPCP (read) struct
*/
typedef struct {
	int32_t process_id;
	std::string name;
	//TODO: add missing info
}ipcp_t;

typedef struct {
	std::string neighbor_name;
	std::string neighbor_instance;
	std::string enr_dif;
	std::string enr_un_dif;
}enrollment_config_t;

/**
* IPCP configuration struct
*/
typedef struct {
	std::string process_name;
	std::string process_instance;
	std::string process_type;
	std::list<std::string> difs_to_register;
	std::string dif_to_assign;
	enrollment_config_t enr_conf;
}ipcp_config_t;

} //namespace mad_manager
} //namespace rinad
#endif /* STRUCTURES_MAD_H_ */
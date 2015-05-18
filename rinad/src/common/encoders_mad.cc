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

#include <librina/exceptions.h>
#include "encoders_mad.h"
#include "common/encoders/MA-IPCP.pb.h"

#include <iostream>

namespace rinad {
namespace mad_manager {
namespace encoders {

//
// Simple types
//
void StringEncoder::encode(const std::string& obj,
                           rina::cdap_rib::ser_obj_t& serobj)
{
        messages::str s;
        s.set_value(obj);

        //Allocate memory
        serobj.size_ = s.ByteSize();
        serobj.message_ = new char[serobj.size_];

        if (!serobj.message_)
                throw rina::Exception("out of memory");  //TODO improve this

        s.SerializeToArray(serobj.message_, serobj.size_);
}

void StringEncoder::decode(const rina::cdap_rib::ser_obj_t& ser_obj,
                           std::string& obj)
{
        messages::str s;
        s.ParseFromArray(ser_obj.message_, ser_obj.size_);

        obj = s.value();
}

//
// Encoder of IPCPConfig
//
void IPCPConfigEncoder::encode(const structures::ipcp_config_t& obj,
                               rina::cdap_rib::ser_obj_t& ser_obj)
{
        messages::ipcp_config gpf_obj;
        gpf_obj.set_process_name(obj.process_name);
        gpf_obj.set_process_instance(obj.process_instance);
        gpf_obj.set_process_type(obj.process_type);
        gpf_obj.set_dif_to_register(obj.dif_to_register);
        gpf_obj.set_dif_to_assign(obj.dif_to_assign);

        //Allocate memory
        ser_obj.size_ = gpf_obj.ByteSize();
        ser_obj.message_ = new char[ser_obj.size_];

        if (!ser_obj.message_)
                throw rina::Exception("out of memory");  //TODO improve this

        gpf_obj.SerializeToArray(ser_obj.message_, ser_obj.size_);
}

void IPCPConfigEncoder::decode(const rina::cdap_rib::ser_obj_t& ser_obj,
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

//
//IPCP encoder (read)
//
void IPCPEncoder::encode(const structures::ipcp_t& obj,
                         rina::cdap_rib::SerializedObject& serobj)
{

        messages::ipcp m;
        m.set_processid(obj.process_id);
        m.set_processname(obj.name);
        //TODO add name

        //Allocate memory
        serobj.size_ = m.ByteSize();
        serobj.message_ = new char[serobj.size_];

        if (!serobj.message_)
                throw rina::Exception("out of memory");  //TODO improve this

        //Serialize and return
        m.SerializeToArray(serobj.message_, serobj.size_);
}

void IPCPEncoder::decode(const rina::cdap_rib::SerializedObject& serobj,
                         structures::ipcp_t& des_obj)
{

        messages::ipcp m;
        if (!m.ParseFromArray(serobj.message_, serobj.size_))
                throw rina::Exception("Could not be parsed");

        des_obj.process_id = m.processid();
}

}  //namespace encoders
}  //namespace mad_manager
}  //namespace rinad

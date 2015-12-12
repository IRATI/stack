/*
 * Common interfaces and classes for encoding/decoding RIB objects
 *
 *    Bernat Gaston <bernat.gaston@i2cat.net>
 *    Eduard Grasa <eduard.grasa@i2cat.net>
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

#ifndef ENCODER_H_
#define ENCODER_H_
#ifdef __cplusplus

#include <librina/common.h>
#include <librina/configuration.h>
#include <librina/ipc-process.h>
#include <librina/cdap_v2.h>
#include "configuration.h"

#include <list>

namespace rinad {

/// Encoder of the DataTransferConstants object
class DataTransferConstantsEncoder: 
	public rina::Encoder<rina::DataTransferConstants> 
{
public:
	void encode(const rina::DataTransferConstants &obj, 
		rina::ser_obj_t& serobj);
	void decode(const rina::ser_obj_t &serobj, 
		rina::DataTransferConstants &des_obj);
};

/// Encoder of DirectoryForwardingTableEntry object
class DFTEEncoder: 
	public rina::Encoder<rina::DirectoryForwardingTableEntry> 
{
public:
	void encode(const rina::DirectoryForwardingTableEntry &obj, 
		rina::ser_obj_t& serobj);
	void decode(const rina::ser_obj_t &serobj, 
		rina::DirectoryForwardingTableEntry &des_obj);
};

/// Encoder of DirectoryForwardingTableEntryList object
class DFTEListEncoder: 
	public rina::Encoder< std::list<rina::DirectoryForwardingTableEntry> >
{
public:
	void encode(const std::list<rina::DirectoryForwardingTableEntry> &obj,
		rina::ser_obj_t& serobj);
	void decode(const rina::ser_obj_t &serobj, 
		std::list<rina::DirectoryForwardingTableEntry> &des_obj);
};

/// Encoder of QoSCube object
class QoSCubeEncoder: public rina::Encoder<rina::QoSCube> 
{
public:
	void encode(const rina::QoSCube &obj, rina::ser_obj_t& serobj);
	void decode(const rina::ser_obj_t &serobj, 
		rina::QoSCube &des_obj);
};

/// Encoder of QoSCube list object
class QoSCubeListEncoder: public rina::Encoder<std::list<rina::QoSCube> >
{
public:
	void encodePointers(const std::list<rina::QoSCube*> &obj,
			    rina::ser_obj_t& serobj);
	void encode(const std::list<rina::QoSCube> &obj, 
		rina::ser_obj_t& serobj);
	void decode(const rina::ser_obj_t &serobj, 
		std::list<rina::QoSCube> &des_obj);
};

/// Encoder of WhatevercastName object
class WhatevercastNameEncoder: public rina::Encoder<rina::WhatevercastName> 
{
public:
	void encode(const rina::WhatevercastName &obj, 
		rina::ser_obj_t& serobj);
	void decode(const rina::ser_obj_t &serobj, 
		rina::WhatevercastName &des_obj);
};

/// Encoder of WhatevercastName list object
class WhatevercastNameListEncoder: 
	public rina::Encoder<std::list<rina::WhatevercastName> >
{
public:
	void encode(const std::list<rina::WhatevercastName> &obj,
		rina::ser_obj_t& serobj);
	void decode(const rina::ser_obj_t &serobj, 
		std::list<rina::WhatevercastName> &des_obj);
};


/// Encoder of Neighbor object
class NeighborEncoder: public rina::Encoder<rina::Neighbor> 
{
public:
	void encode(const rina::Neighbor &obj, rina::ser_obj_t& serobj);
	void decode(const rina::ser_obj_t &serobj, 
		rina::Neighbor &des_obj);
};

/// Encoder of Neighbor list object
class NeighborListEncoder: public rina::Encoder<std::list<rina::Neighbor> >
{
public:
	void encode(const std::list<rina::Neighbor> &obj,
		rina::ser_obj_t& serobj);
	void decode(const rina::ser_obj_t &serobj, 
		std::list<rina::Neighbor> &des_obj);
};

/// Encoder of the AData object
class ADataObjectEncoder: public rina::Encoder<rina::cdap::ADataObject>
{
public:
	void encode(const rina::cdap::ADataObject &obj, rina::ser_obj_t& serobj);
	void decode(const rina::ser_obj_t &serobj, rina::cdap::ADataObject &des_obj);
};

/// Encoder of a list of EnrollmentInformationRequest
class EnrollmentInformationRequestEncoder: 
	public rina::Encoder<EnrollmentInformationRequest> {
public:
	void encode(const EnrollmentInformationRequest &obj, 
		rina::ser_obj_t& serobj);
	void decode(const rina::ser_obj_t &serobj, 
		EnrollmentInformationRequest &des_obj);
};

/// Encoder of the Flow
class FlowEncoder: public rina::Encoder<Flow> {
public:
	void encode(const Flow &obj, rina::ser_obj_t& serobj);
	void decode(const rina::ser_obj_t &serobj, Flow &des_obj);
};

/// Encoder of the RoutingTableEntry
class RoutingTableEntryEncoder: public rina::Encoder<rina::RoutingTableEntry> {
public:
	void encode(const rina::RoutingTableEntry &obj, rina::ser_obj_t& serobj);
	void decode(const rina::ser_obj_t &serobj, rina::RoutingTableEntry &des_obj);
};

/// Encoder of the PDUForwardingTableEntry
class PDUForwardingTableEntryEncoder: public rina::Encoder<rina::PDUForwardingTableEntry> {
public:
	void encode(const rina::PDUForwardingTableEntry &obj, rina::ser_obj_t& serobj);
	void decode(const rina::ser_obj_t &serobj, rina::PDUForwardingTableEntry &des_obj);
};

/// Encoder of the Connection
class DTPInformationEncoder: public rina::Encoder<rina::DTPInformation> {
public:
	void encode(const rina::DTPInformation &obj, rina::ser_obj_t& serobj);
	void decode(const rina::ser_obj_t &serobj, rina::DTPInformation &des_obj);
};

/// Encoder of the Connection
class DTCPInformationEncoder: public rina::Encoder<rina::DTCPConfig> {
public:
	void encode(const rina::DTCPConfig &obj, rina::ser_obj_t& serobj);
	void decode(const rina::ser_obj_t &serobj, rina::DTCPConfig &des_obj);
};

}

#endif
#endif /// ENCODER_H_////

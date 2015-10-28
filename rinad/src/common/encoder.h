/*
 * Common interfaces and classes for encoding/decoding RIB objects
 *
 *    Bernat Gaston <bernat.gaston@i2cat.net>
 *    Eduard Grasa <eduard.grasa@i2cat.net>
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

/// Definition of names of object names and object values
class EncoderConstants {
public:
	/// Partial names
	static const std::string ADDRESS;
	static const std::string APNAME;
	static const std::string CONSTANTS;
	static const std::string DATA_TRANSFER;
	static const std::string DAF;
	static const std::string DIF;
	static const std::string DIF_REGISTRATIONS;
	static const std::string DIRECTORY_FORWARDING_TABLE_ENTRIES;
	static const std::string ENROLLMENT;
	static const std::string FLOWS;
	static const std::string FLOW_ALLOCATOR;
	static const std::string IPC;
	static const std::string DIFMANAGEMENT;
	static const std::string DAFMANAGEMENT;
	static const std::string MANAGEMENT;
	static const std::string NEIGHBORS;
	static const std::string NAMING;
	static const std::string NMINUSONEFLOWMANAGER;
	static const std::string NMINUSEONEFLOWS;
	static const std::string OPERATIONAL_STATUS;
	static const std::string PDU_FORWARDING_TABLE;
	static const std::string QOS_CUBES;
	static const std::string RESOURCE_ALLOCATION;
	static const std::string ROOT;
	static const std::string SEPARATOR;
	static const std::string SYNONYMS;
	static const std::string WHATEVERCAST_NAMES;
	static const std::string ROUTING;
	static const std::string FLOWSTATEOBJECTGROUP;
	static const std::string LINKSTATE;
	static const std::string DIF_NAME_WHATEVERCAST_RULE;
	static const std::string WATCHDOG;

	static const std::string DAF_RIB_OBJECT_CLASS;
	static const std::string DAF_RIB_OBJECT_NAME;
	static const std::string DIF_RIB_OBJECT_CLASS;
	static const std::string DIF_RIB_OBJECT_NAME;
	static const std::string DAF_MANAGEMENT_RIB_OBJECT_CLASS;
	static const std::string DAF_MANAGEMENT_RIB_OBJECT_NAME;
	static const std::string DIF_MANAGEMENT_RIB_OBJECT_CLASS;
	static const std::string DIF_MANAGEMENT_RIB_OBJECT_NAME;
	static const std::string RESOURCE_ALLOCATION_RIB_OBJECT_CLASS;
	static const std::string RESOURCE_ALLOCATION_RIB_OBJECT_NAME;
	static const std::string NMINUSONEFLOWMANAGER_RIB_OBJECT_CLASS;
	static const std::string NMINUSONEFLOWMANAGER_RIB_OBJECT_NAME;
	static const std::string NAMING_RIB_OBJECT_CLASS;
	static const std::string NAMING_RIB_OBJECT_NAME;
	static const std::string FLOW_ALLOCATOR_RIB_OBJECT_CLASS;
	static const std::string FLOW_ALLOCATOR_RIB_OBJECT_NAME;
	static const std::string LINKSTATE_RIB_OBJECT_CLASS;
	static const std::string LINKSTATE_RIB_OBJECT_NAME;
	static const std::string IPC_RIB_OBJECT_CLASS;
	static const std::string IPC_RIB_OBJECT_NAME;
	static const std::string DATA_TRANSFER_RIB_OBJECT_CLASS;
	static const std::string DATA_TRANSFER_RIB_OBJECT_NAME;
	/* Full names */
	static const std::string ADDRESS_RIB_OBJECT_CLASS;
	static const std::string ADDRESS_RIB_OBJECT_NAME;
	static const std::string DATA_TRANSFER_CONSTANTS_RIB_OBJECT_CLASS;
	static const std::string DATA_TRANSFER_CONSTANTS_RIB_OBJECT_NAME;
	static const std::string DFT_ENTRY_SET_RIB_OBJECT_NAME;
	static const std::string DFT_ENTRY_SET_RIB_OBJECT_CLASS;
	static const std::string DFT_ENTRY_RIB_OBJECT_CLASS;
	static const std::string ENROLLMENT_INFO_OBJECT_NAME;
	static const std::string ENROLLMENT_INFO_OBJECT_CLASS;
	static const std::string FLOW_SET_RIB_OBJECT_NAME;
	static const std::string FLOW_SET_RIB_OBJECT_CLASS ;
	static const std::string FLOW_RIB_OBJECT_CLASS;
	static const std::string FLOW_STATE_OBJECT_RIB_OBJECT_CLASS;
	static const std::string FLOW_STATE_OBJECT_GROUP_RIB_OBJECT_CLASS;
	static const std::string FLOW_STATE_OBJECT_GROUP_RIB_OBJECT_NAME;
	static const std::string OPERATIONAL_STATUS_RIB_OBJECT_NAME;
	static const std::string OPERATIONAL_STATUS_RIB_OBJECT_CLASS;
	static const std::string PDU_FORWARDING_TABLE_RIB_OBJECT_CLASS;
	static const std::string PDU_FORWARDING_TABLE_RIB_OBJECT_NAME;
	static const std::string QOS_CUBE_SET_RIB_OBJECT_NAME;
	static const std::string QOS_CUBE_SET_RIB_OBJECT_CLASS ;
	static const std::string QOS_CUBE_RIB_OBJECT_CLASS;
	static const std::string WATCHDOG_RIB_OBJECT_NAME;
	static const std::string WATCHDOG_RIB_OBJECT_CLASS;
	static const std::string WHATEVERCAST_NAME_SET_RIB_OBJECT_NAME;
	static const std::string WHATEVERCAST_NAME_SET_RIB_OBJECT_CLASS;
	static const std::string WHATEVERCAST_NAME_RIB_OBJECT_CLASS;
};

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
class ADataObjectEncoder: public rina::Encoder<rina::ADataObject> 
{
public:
	void encode(const rina::ADataObject &obj, rina::ser_obj_t& serobj);
	void decode(const rina::ser_obj_t &serobj, rina::ADataObject &des_obj);
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

}

#endif
#endif /// ENCODER_H_////

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

#include <string>
#include <map>

#include <librina/ipc-process.h>
#include "common/encoders/ApplicationProcessNamingInfoMessage.pb.h"
#include "common/encoders/CommonMessages.pb.h"
#include "common/encoders/ConnectionPoliciesMessage.pb.h"
#include "common/encoders/DirectoryForwardingTableEntryMessage.pb.h"
#include "common/encoders/FlowMessage.pb.h"
#include "common/encoders/NeighborMessage.pb.h"
#include "common/encoders/PolicyDescriptorMessage.pb.h"
#include "common/encoders/QoSCubeMessage.pb.h"
#include "common/encoders/QoSSpecification.pb.h"
#include "common/encoders/WhatevercastNameMessage.pb.h"

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

/// Implements an encoder that delegates the encoding/decoding
/// tasks to different subencoders. A different encoder is registered
/// by each type of object. The encoder also implements static helper functions
/// to encode/decode sub-objects that are shared between two or more classes.
class Encoder: public rina::IMasterEncoder {
public:
	~Encoder();
	/// Set the class that serializes/unserializes an object class
	/// @param objectClass The object class
	/// @param serializer

	void addEncoder(const std::string& object_class, rina::EncoderInterface *encoder);
	/// Converts an object of the type specified by "className" to a byte array.
	/// @param object
	/// @return

	void encode(const void* object, rina::CDAPMessage * cdapMessage);

	/// Converts a byte array to an object of the type specified by "className"
	/// @param serializedObject
	/// @param The type of object to be decoded
	/// @throws exception if the byte array is not an encoded in a way that the encoder can recognize, or the
	/// byte array value doesn't correspond to an object of the type "className"
	/// @return
	void* decode(const rina::CDAPMessage * cdapMessage);

	static rina::SerializedObject * get_serialized_object(const rina::ObjectValueInterface * object_value);
	static rina::messages::applicationProcessNamingInfo_t* get_applicationProcessNamingInfo_t(
			const rina::ApplicationProcessNamingInformation &name);
	static rina::ApplicationProcessNamingInformation* get_ApplicationProcessNamingInformation(
			const rina::messages::applicationProcessNamingInfo_t &gpf_app);
	static rina::messages::qosSpecification_t* get_qosSpecification_t(const rina::FlowSpecification &flow_spec);
	static rina::FlowSpecification* get_FlowSpecification(const rina::messages::qosSpecification_t &gpf_qos);
	static void get_property_t(rina::messages::property_t * gpb_conf, const rina::PolicyParameter &conf);
	static rina::PolicyParameter* get_PolicyParameter(const rina::messages::property_t &gpf_conf);
	static rina::messages::policyDescriptor_t* get_policyDescriptor_t(const rina::PolicyConfig &conf);
	static rina::PolicyConfig* get_PolicyConfig(const rina::messages::policyDescriptor_t &gpf_conf);
	static rina::messages::dtcpRtxControlConfig_t* get_dtcpRtxControlConfig_t(const rina::DTCPRtxControlConfig &conf);
	static rina::DTCPRtxControlConfig* get_DTCPRtxControlConfig(const rina::messages::dtcpRtxControlConfig_t &gpf_conf);
	static rina::messages::dtcpWindowBasedFlowControlConfig_t* get_dtcpWindowBasedFlowControlConfig_t(
			const rina::DTCPWindowBasedFlowControlConfig &conf);
	static rina::DTCPWindowBasedFlowControlConfig* get_DTCPWindowBasedFlowControlConfig(
			const rina::messages::dtcpWindowBasedFlowControlConfig_t &gpf_conf);
	static rina::messages::dtcpRateBasedFlowControlConfig_t* get_dtcpRateBasedFlowControlConfig_t(
			const rina::DTCPRateBasedFlowControlConfig &conf);
	static rina::DTCPRateBasedFlowControlConfig* get_DTCPRateBasedFlowControlConfig(
			const rina::messages::dtcpRateBasedFlowControlConfig_t &gpf_conf);
	static rina::messages::dtcpFlowControlConfig_t* get_dtcpFlowControlConfig_t(const rina::DTCPFlowControlConfig &conf);
	static rina::DTCPFlowControlConfig* get_DTCPFlowControlConfig(const rina::messages::dtcpFlowControlConfig_t &gpf_conf);
	static rina::messages::dtcpConfig_t* get_dtcpConfig_t(const rina::DTCPConfig &conf);
	static rina::DTCPConfig* get_DTCPConfig(const rina::messages::dtcpConfig_t &gpf_conf);
	static rina::messages::dtpConfig_t* get_dtpConfig_t(const rina::DTPConfig &conf);
	static rina::DTPConfig* get_DTPConfig(const rina::messages::dtpConfig_t &gpf_conf);
	static rina::Connection* get_Connection(const rina::messages::connectionId_t &gpf_conn);

private:
	/// Get the encoder associated to object class, throwing
	/// and exception if no encoder is found
	/// @param object_class
	/// @return A pointer to the encoder associated to object class
	rina::EncoderInterface * get_encoder(const std::string& object_class);

	std::map<std::string, rina::EncoderInterface*> encoders_;
};

/// Encoder of the DataTransferConstants object
class DataTransferConstantsEncoder: public rina::EncoderInterface {
public:
	const rina::SerializedObject* encode(const void* object);
	void* decode(const rina::ObjectValueInterface * object_value) const;
};

/// Encoder of DirectoryForwardingTableEntry object
class DirectoryForwardingTableEntryEncoder: public rina::EncoderInterface {
public:
	const rina::SerializedObject* encode(const void* object);
	void* decode(const rina::ObjectValueInterface * object_value) const;
	static void convertModelToGPB(rina::messages::directoryForwardingTableEntry_t * gpb_dfte,
			rina::DirectoryForwardingTableEntry * dfte);
	static rina::DirectoryForwardingTableEntry * convertGPBToModel(
			const rina::messages::directoryForwardingTableEntry_t& gpb_dfte);
};

/// Encoder of a list of DirectoryForwardingTableEntries
class DirectoryForwardingTableEntryListEncoder: public rina::EncoderInterface {
public:
	const rina::SerializedObject* encode(const void* object);
	void* decode(const rina::ObjectValueInterface * object_value) const;
};

/// Encoder of QoSCube object
class QoSCubeEncoder: public rina::EncoderInterface {
public:
	const rina::SerializedObject* encode(const void* object);
	void* decode(const rina::ObjectValueInterface * object_value) const;
	static void convertModelToGPB(rina::messages::qosCube_t * gpb_cube,
			rina::QoSCube * cube);
	static rina::QoSCube * convertGPBToModel(
			const rina::messages::qosCube_t & gpb_cube);
};

/// Encoder of a list of QoSCubes
class QoSCubeListEncoder: public rina::EncoderInterface {
public:
	const rina::SerializedObject* encode(const void* object);
	void* decode(const rina::ObjectValueInterface * object_value) const;
};

/// Encoder of WhatevercastName object
class WhatevercastNameEncoder: public rina::EncoderInterface {
public:
	const rina::SerializedObject* encode(const void* object);
	void* decode(const rina::ObjectValueInterface * object_value) const;
	static void convertModelToGPB(rina::messages::whatevercastName_t * gpb_name,
			rina::WhatevercastName * name);
	static rina::WhatevercastName * convertGPBToModel(
			const rina::messages::whatevercastName_t & gpb_name);
};

/// Encoder of a list of WhatevercastNames
class WhatevercastNameListEncoder: public rina::EncoderInterface {
public:
	const rina::SerializedObject* encode(const void* object);
	void* decode(const rina::ObjectValueInterface * object_value) const;
};

/// Encoder of Neighbor object
class NeighborEncoder: public rina::EncoderInterface {
public:
	const rina::SerializedObject* encode(const void* object);
	void* decode(const rina::ObjectValueInterface * object_value) const;
	static void convertModelToGPB(rina::messages::neighbor_t * gpb_nei,
			rina::Neighbor * nei);
	static rina::Neighbor * convertGPBToModel(
			const rina::messages::neighbor_t & gpb_nei);
};

/// Encoder of a list of Neighbors
class NeighborListEncoder: public rina::EncoderInterface {
public:
	const rina::SerializedObject* encode(const void* object);
	void* decode(const rina::ObjectValueInterface * object_value) const;
};

/// Encoder of Watchdog
class WatchdogEncoder: public rina::EncoderInterface {
public:
	const rina::SerializedObject* encode(const void* object);
	void* decode(const rina::ObjectValueInterface * object_value) const;
};

/// Encoder of the AData object
class ADataObjectEncoder: public rina::EncoderInterface {
public:
	const rina::SerializedObject* encode(const void* object);
	void* decode(const rina::ObjectValueInterface * object_value) const;
};

}

#endif
#endif /// ENCODER_H_////

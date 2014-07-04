/*
 * Common interfaces and classes for encoding/decoding RIB objects
 *
 *    Bernat Gaston <bernat.gaston@i2cat.net>
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
#include "librina/common.h"

namespace rinad {

/// Encodes and Decodes an object to/from bytes)
class EncoderInterface {
public:
	virtual ~EncoderInterface(){};
	 /// Converts an object to a byte array, if this object is recognized by the encoder
	 /// @param object
	 /// @throws exception if the object is not recognized by the encoder
	 /// @return
	virtual const rina::SerializedObject* encode(const void* object) const = 0;
	 /// Converts a byte array to an object of the type specified by "className"
	 /// @param byte[] serializedObject
	 /// @param objectClass The type of object to be decoded
	 /// @throws exception if the byte array is not an encoded in a way that the encoder can recognize, or the
	 /// byte array value doesn't correspond to an object of the type "className"
	 /// @return
	virtual void* decode(const rina::SerializedObject &serialized_object) const = 0;
};

/// Factory that provides and Encoder instance
class EncoderFactoryInterface {
public:
	virtual ~EncoderFactoryInterface(){};
	virtual EncoderInterface* createEncoderInstance() = 0;
};


/// Implements an encoder that delegates the encoding/decoding
/// tasks to different subencoders. A different encoder is registered
/// by each type of object
class Encoder {
public:
	~Encoder();
	/// Set the class that serializes/unserializes an object class
	/// @param objectClass The object class
	/// @param serializer
	void addEncoder(const std::string& object_class, EncoderInterface *encoder);
	/// Converts an object of the type specified by "className" to a byte array.
	/// @param object
	/// @return
	const rina::SerializedObject* encode(const void* object,
			const std::string& object_class);
	/// Converts a byte array to an object of the type specified by "className"
	/// @param serializedObject
	/// @param The type of object to be decoded
	/// @throws exception if the byte array is not an encoded in a way that the encoder can recognize, or the
	/// byte array value doesn't correspond to an object of the type "className"
	/// @return
	void* decode(const rina::SerializedObject &serialized_object,
			const std::string& object_class);
private:
	std::map<std::string, EncoderInterface*> encoders_;
};

/// Encoder of the ApplicationRegistrationEncoder
class ApplicationRegistrationEncoder: public  EncoderInterface{
	const rina::SerializedObject* encode(const void* object) const;
	void* decode(const rina::SerializedObject &serialized_object) const;
};

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
	static const std::string DIF_NAME_WHATEVERCAST_RULE;

	/* Full names */
	static const std::string DFT_ENTRY_SET_RIB_OBJECT_NAME;
	static const std::string DFT_ENTRY_SET_RIB_OBJECT_CLASS;
	static const std::string DFT_ENTRY_RIB_OBJECT_CLASS;
	static const std::string DIF_REGISTRATION_SET_RIB_OBJECT_CLASS;
	static const std::string DIF_REGISTRATION_RIB_OBJECT_CLASS;
	static const std::string DIF_REGISTRATION_SET_RIB_OBJECT_NAME;
	static const std::string ENROLLMENT_INFO_OBJECT_NAME;
	static const std::string FLOW_SET_RIB_OBJECT_NAME;
	static const std::string FLOW_SET_RIB_OBJECT_CLASS ;
	static const std::string FLOW_RIB_OBJECT_CLASS;
	static const std::string N_MINUS_ONE_FLOW_SET_RIB_OBJECT_CLASS;
	static const std::string N_MINUS_ONE_FLOW_RIB_OBJECT_CLASS;
	static const std::string N_MINUS_ONE_FLOW_SET_RIB_OBJECT_NAME;
	static const std::string OPERATIONAL_STATUS_RIB_OBJECT_NAME;
	static const std::string OPERATIONAL_STATUS_RIB_OBJECT_CLASS;
	static const std::string PDU_FORWARDING_TABLE_RIB_OBJECT_CLASS;
	static const std::string PDU_FORWARDING_TABLE_RIB_OBJECT_NAME;
	static const std::string QOS_CUBE_SET_RIB_OBJECT_NAME;
	static const std::string QOS_CUBE_SET_RIB_OBJECT_CLASS ;
	static const std::string QOS_CUBE_RIB_OBJECT_CLASS;
	static const std::string WHATEVERCAST_NAME_SET_RIB_OBJECT_NAME;
	static const std::string WHATEVERCAST_NAME_SET_RIB_OBJECT_CLASS;
	static const std::string WHATEVERCAST_NAME_RIB_OBJECT_CLASS;
};


}


#endif
#endif /// ENCODER_H_////

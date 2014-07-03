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
	enum ObjectClass {

	};
	~Encoder();
	/// Set the class that serializes/unserializes an object class
	/// @param objectClass The object class
	/// @param serializer
	void addEncoder(ObjectClass object_class, EncoderInterface *encoder);
	/// Converts an object of the type specified by "className" to a byte array.
	/// @param object
	/// @return
	const rina::SerializedObject* encode(const void* object, ObjectClass object_class);
	/// Converts a byte array to an object of the type specified by "className"
	/// @param serializedObject
	/// @param The type of object to be decoded
	/// @throws exception if the byte array is not an encoded in a way that the encoder can recognize, or the
	/// byte array value doesn't correspond to an object of the type "className"
	/// @return
	void* decode(const rina::SerializedObject &serialized_object, ObjectClass object_class);
private:
	std::map<ObjectClass, EncoderInterface*> encoders_;
};

/// Encoder of the ApplicationRegistrationEncoder
class ApplicationRegistrationEncoder: public  EncoderInterface{
	const rina::SerializedObject* encode(const void* object) const;
	void* decode(const rina::SerializedObject &serialized_object) const;
};


}


#endif
#endif /// ENCODER_H_////

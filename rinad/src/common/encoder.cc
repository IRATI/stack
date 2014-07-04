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
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include "encoder.h"
#include "librina/application.h"
#include "encoders/ApplicationProcessNamingInfoMessage.pb.h"
#include "encoders/ApplicationRegistrationMessage.pb.h"

namespace rinad {


/// CLASS Encoder
Encoder::~Encoder() {
	for (std::map<ObjectClass,EncoderInterface*>::iterator it = encoders_.begin(); it != encoders_.end(); ++it) {
		delete it->second;
		it->second = 0;
	}
	encoders_.clear();
}
void Encoder::addEncoder(ObjectClass object_class, EncoderInterface *encoder) {
	encoders_.insert(std::pair<ObjectClass,EncoderInterface*> (object_class, encoder));
}
const rina::SerializedObject* Encoder::encode(const void* object, ObjectClass object_class) {
	EncoderInterface* encoder = encoders_[object_class];
	return encoder->encode(object);
}
void* Encoder::decode(const rina::SerializedObject &serialized_object, ObjectClass object_class) {
	EncoderInterface* encoder = encoders_[object_class];
	return encoder->decode(serialized_object);
}

Encoder::ObjectClass Encoder::getEnum(std::string object_class)
{
	switch(object_class) {
	case "ApplicationRegistration":
		return ApplicationRegistration;
		break;
	default:
		throw new Exception("Class not found");
	}
}

// CLASS ApplicationRegistrationEncoder
const rina::SerializedObject* ApplicationRegistrationEncoder::encode(const void* object) const {
	rina::ApplicationRegistration *app_reg = (rina::ApplicationRegistration*) object;
	rina::messages::ApplicationRegistration gpf_app_reg;
	rina::ApplicationProcessNamingInformation app_proc_nam_info = app_reg->getApplicationName();
	rina::messages::applicationProcessNamingInfo_t gpf_app_proc_nam_info;

	// Application Naming Information
	gpf_app_proc_nam_info.set_applicationprocessname(app_proc_nam_info.getProcessName().c_str());
	gpf_app_proc_nam_info.set_applicationprocessinstance(app_proc_nam_info.getProcessInstance());
	gpf_app_proc_nam_info.set_applicationentityname(app_proc_nam_info.getEntityName());
	gpf_app_proc_nam_info.set_applicationentityinstance(app_proc_nam_info.getEntityInstance());
	gpf_app_reg.set_allocated_naminginfo(&gpf_app_proc_nam_info);

	// Socket Number
	gpf_app_reg.set_socketnumber(0);

	// DIF Names
	for (std::list<rina::ApplicationProcessNamingInformation>::const_iterator it = app_reg->getDIFNames().begin(); it != app_reg->getDIFNames().end(); ++it)
	{
		gpf_app_reg.add_difnames(it->getProcessName());
	}
	int size = gpf_app_reg.ByteSize();
	char *serialized_message = new char[size];
	gpf_app_reg.SerializeToArray(serialized_message, size);
	rina::SerializedObject *serialized_object =  new rina::SerializedObject(serialized_message,size);

	return serialized_object;
}
void* ApplicationRegistrationEncoder::decode(const rina::SerializedObject &serialized_object) const {
	rina::ApplicationProcessNamingInformation app_proc_nam_info;
	rina::ApplicationRegistration *app_reg = new rina::ApplicationRegistration(app_proc_nam_info);
	rina::messages::ApplicationRegistration gpf_app_reg;

	gpf_app_reg.ParseFromArray(serialized_object.message_, serialized_object.size_);
	rina::messages::applicationProcessNamingInfo_t gpf_app_proc_nam_info = gpf_app_reg.naminginfo();

	// Application Naming Information
	app_proc_nam_info.setProcessName(gpf_app_proc_nam_info.applicationprocessname());
	app_proc_nam_info.setProcessInstance(gpf_app_proc_nam_info.applicationprocessinstance());
	app_proc_nam_info.setEntityName(gpf_app_proc_nam_info.applicationentityname());
	app_proc_nam_info.setEntityInstance(gpf_app_proc_nam_info.applicationentityinstance());

	// DIF Names
	for (int i = 0; i < gpf_app_reg.difnames_size(); i++)
	{
		rina::ApplicationProcessNamingInformation ap;
		ap.setProcessName(gpf_app_reg.difnames(i));
		app_reg->addDIFName(ap);
	}

	return (void*) app_reg;
}

}

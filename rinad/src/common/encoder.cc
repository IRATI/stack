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
#include <librina/application.h>
#include "encoders/ApplicationProcessNamingInfoMessage.pb.h"
#include "encoders/ApplicationRegistrationMessage.pb.h"

namespace rinad {


/// CLASS Encoder
Encoder::~Encoder() {
	for (std::map<std::string,EncoderInterface*>::iterator it = encoders_.begin(); it != encoders_.end(); ++it) {
		delete it->second;
		it->second = 0;
	}
	encoders_.clear();
}
void Encoder::addEncoder(const std::string& object_class, EncoderInterface *encoder) {
	encoders_.insert(std::pair<std::string,EncoderInterface*> (object_class, encoder));
}
const rina::SerializedObject* Encoder::encode(const void* object, const std::string& object_class) {
	EncoderInterface* encoder = encoders_[object_class];
	return encoder->encode(object);
}
void* Encoder::decode(const rina::SerializedObject &serialized_object, const std::string& object_class) {
	EncoderInterface* encoder = encoders_[object_class];
	return encoder->decode(serialized_object);
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

//Class Encoder Constants
const std::string EncoderConstants::ADDRESS = "address";
const std::string EncoderConstants::APNAME = "applicationprocessname";
const std::string EncoderConstants::CONSTANTS = "constants";
const std::string EncoderConstants::DATA_TRANSFER = "datatransfer";
const std::string EncoderConstants::DAF = "daf";
const std::string EncoderConstants::DIF = "dif";
const std::string EncoderConstants::DIF_REGISTRATIONS = "difregistrations";
const std::string EncoderConstants::DIRECTORY_FORWARDING_TABLE_ENTRIES = "directoryforwardingtableentries";
const std::string EncoderConstants::ENROLLMENT = "enrollment";
const std::string EncoderConstants::FLOWS = "flows";
const std::string EncoderConstants::FLOW_ALLOCATOR = "flowallocator";
const std::string EncoderConstants::IPC = "ipc";
const std::string EncoderConstants::MANAGEMENT = "management";
const std::string EncoderConstants::NEIGHBORS = "neighbors";
const std::string EncoderConstants::NAMING = "naming";
const std::string EncoderConstants::NMINUSONEFLOWMANAGER = "nminusoneflowmanager";
const std::string EncoderConstants::NMINUSEONEFLOWS = "nminusoneflows";
const std::string EncoderConstants::OPERATIONAL_STATUS = "operationalStatus";
const std::string EncoderConstants::PDU_FORWARDING_TABLE = "pduforwardingtable";
const std::string EncoderConstants::QOS_CUBES = "qoscubes";
const std::string EncoderConstants::RESOURCE_ALLOCATION = "resourceallocation";
const std::string EncoderConstants::ROOT = "root";
const std::string EncoderConstants::SEPARATOR = "/";
const std::string EncoderConstants::SYNONYMS = "synonyms";
const std::string EncoderConstants::WHATEVERCAST_NAMES = "whatevercastnames";
const std::string EncoderConstants::ROUTING = "routing";
const std::string EncoderConstants::FLOWSTATEOBJECTGROUP = "flowstateobjectgroup";
const std::string EncoderConstants::LINKSTATE = "linkstate";
const std::string EncoderConstants::OPERATIONAL_STATUS_RIB_OBJECT_NAME = SEPARATOR + DAF +
		SEPARATOR + MANAGEMENT + SEPARATOR + OPERATIONAL_STATUS;
const std::string EncoderConstants::OPERATIONAL_STATUS_RIB_OBJECT_CLASS = "operationstatus";
const std::string EncoderConstants::PDU_FORWARDING_TABLE_RIB_OBJECT_CLASS = "pdu forwarding table";
const std::string EncoderConstants::PDU_FORWARDING_TABLE_RIB_OBJECT_NAME = SEPARATOR + DIF +
		SEPARATOR + RESOURCE_ALLOCATION + SEPARATOR + PDU_FORWARDING_TABLE;
const std::string EncoderConstants::DIF_REGISTRATION_SET_RIB_OBJECT_CLASS = "DIF registration set";
const std::string EncoderConstants::DIF_REGISTRATION_RIB_OBJECT_CLASS = "DIF registration";
const std::string EncoderConstants::DIF_REGISTRATION_SET_RIB_OBJECT_NAME = SEPARATOR + DIF +
		SEPARATOR + RESOURCE_ALLOCATION + SEPARATOR + NMINUSONEFLOWMANAGER + SEPARATOR + DIF_REGISTRATIONS;
const std::string EncoderConstants::N_MINUS_ONE_FLOW_SET_RIB_OBJECT_CLASS = "nminusone flow set";
const std::string EncoderConstants::N_MINUS_ONE_FLOW_RIB_OBJECT_CLASS = "nminusone flow";
const std::string EncoderConstants::N_MINUS_ONE_FLOW_SET_RIB_OBJECT_NAME = SEPARATOR + DIF +
		SEPARATOR + RESOURCE_ALLOCATION + SEPARATOR + NMINUSONEFLOWMANAGER + SEPARATOR + NMINUSEONEFLOWS;
const std::string EncoderConstants::WHATEVERCAST_NAME_SET_RIB_OBJECT_NAME = SEPARATOR + DAF +
		SEPARATOR + MANAGEMENT + SEPARATOR + NAMING + SEPARATOR + WHATEVERCAST_NAMES;
const std::string EncoderConstants::WHATEVERCAST_NAME_SET_RIB_OBJECT_CLASS = "whatname set";
const std::string EncoderConstants::WHATEVERCAST_NAME_RIB_OBJECT_CLASS = "whatname";
const std::string EncoderConstants::DIF_NAME_WHATEVERCAST_RULE = "any";
const std::string EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_NAME = SEPARATOR +
		DIF + SEPARATOR + MANAGEMENT + SEPARATOR + FLOW_ALLOCATOR + SEPARATOR +
		DIRECTORY_FORWARDING_TABLE_ENTRIES;
const std::string EncoderConstants::DFT_ENTRY_SET_RIB_OBJECT_CLASS = "directoryforwardingtableentry set";
const std::string EncoderConstants::DFT_ENTRY_RIB_OBJECT_CLASS = "directoryforwardingtableentry";
const std::string EncoderConstants::FLOW_SET_RIB_OBJECT_NAME = SEPARATOR + DIF + SEPARATOR +
	    RESOURCE_ALLOCATION + SEPARATOR + FLOW_ALLOCATOR + SEPARATOR + FLOWS;
const std::string EncoderConstants::FLOW_SET_RIB_OBJECT_CLASS = "flow set";
const std::string EncoderConstants::FLOW_RIB_OBJECT_CLASS = "flow";
const std::string EncoderConstants::QOS_CUBE_SET_RIB_OBJECT_NAME = SEPARATOR + DIF + SEPARATOR + MANAGEMENT +
		SEPARATOR + FLOW_ALLOCATOR + SEPARATOR + QOS_CUBES;
const std::string EncoderConstants::QOS_CUBE_SET_RIB_OBJECT_CLASS = "qoscube set";
const std::string EncoderConstants::QOS_CUBE_RIB_OBJECT_CLASS = "qoscube";
const std::string EncoderConstants::ENROLLMENT_INFO_OBJECT_NAME = SEPARATOR + DAF +
			SEPARATOR + MANAGEMENT + SEPARATOR + ENROLLMENT;
const std::string EncoderConstants::FLOW_STATE_OBJECT_RIB_OBJECT_CLASS = "flowstateobject";
const std::string EncoderConstants::FLOW_STATE_OBJECT_GROUP_RIB_OBJECT_CLASS = "flowstateobject set";
const std::string EncoderConstants::FLOW_STATE_OBJECT_GROUP_RIB_OBJECT_NAME = SEPARATOR +
		DIF + SEPARATOR + MANAGEMENT + SEPARATOR + PDU_FORWARDING_TABLE + SEPARATOR
		+ LINKSTATE + SEPARATOR + FLOWSTATEOBJECTGROUP;

}

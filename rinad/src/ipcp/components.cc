//
// Common interfaces and constants of the IPC Process components
//
//    Bernat Gaston <bernat.gaston@i2cat.net>
//    Eduard Grasa <eduard.grasa@i2cat.net>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#include "components.h"

namespace rinad {

//	CLASS RIBObjectNames
const std::string RIBObjectNames::ADDRESS = "address";
const std::string RIBObjectNames::APNAME = "applicationprocessname";
const std::string RIBObjectNames::CONSTANTS = "constants";
const std::string RIBObjectNames::DATA_TRANSFER = "datatransfer";
const std::string RIBObjectNames::DAF = "daf";
const std::string RIBObjectNames::DIF = "dif";
const std::string RIBObjectNames::DIF_REGISTRATIONS = "difregistrations";
const std::string RIBObjectNames::DIRECTORY_FORWARDING_TABLE_ENTRIES = "directoryforwardingtableentries";
const std::string RIBObjectNames::ENROLLMENT = "enrollment";
const std::string RIBObjectNames::FLOWS = "flows";
const std::string RIBObjectNames::FLOW_ALLOCATOR = "flowallocator";
const std::string RIBObjectNames::IPC = "ipc";
const std::string RIBObjectNames::MANAGEMENT = "management";
const std::string RIBObjectNames::NEIGHBORS = "neighbors";
const std::string RIBObjectNames::NAMING = "naming";
const std::string RIBObjectNames::NMINUSONEFLOWMANAGER = "nminusoneflowmanager";
const std::string RIBObjectNames::NMINUSEONEFLOWS = "nminusoneflows";
const std::string RIBObjectNames::OPERATIONAL_STATUS = "operationalStatus";
const std::string RIBObjectNames::PDU_FORWARDING_TABLE = "pduforwardingtable";
const std::string RIBObjectNames::QOS_CUBES = "qoscubes";
const std::string RIBObjectNames::RESOURCE_ALLOCATION = "resourceallocation";
const std::string RIBObjectNames::ROOT = "root";
const std::string RIBObjectNames::SEPARATOR = "/";
const std::string RIBObjectNames::SYNONYMS = "synonyms";
const std::string RIBObjectNames::WHATEVERCAST_NAMES = "whatevercastnames";
const std::string RIBObjectNames::ROUTING = "routing";
const std::string RIBObjectNames::FLOWSTATEOBJECTGROUP = "flowstateobjectgroup";
const std::string RIBObjectNames::OPERATIONAL_STATUS_RIB_OBJECT_NAME = RIBObjectNames::SEPARATOR + RIBObjectNames::DAF +
		RIBObjectNames::SEPARATOR + RIBObjectNames::MANAGEMENT + RIBObjectNames::SEPARATOR + RIBObjectNames::OPERATIONAL_STATUS;

const std::string RIBObjectNames::OPERATIONAL_STATUS_RIB_OBJECT_CLASS = "operationstatus";

const std::string RIBObjectNames::PDU_FORWARDING_TABLE_RIB_OBJECT_CLASS = "pdu forwarding table";
const std::string RIBObjectNames::PDU_FORWARDING_TABLE_RIB_OBJECT_NAME = RIBObjectNames::SEPARATOR + RIBObjectNames::DIF +
		RIBObjectNames::SEPARATOR + RIBObjectNames::RESOURCE_ALLOCATION + RIBObjectNames::SEPARATOR + RIBObjectNames::PDU_FORWARDING_TABLE;

//	CLASS EnrollmentRequest
EnrollmentRequest::EnrollmentRequest(
		const rina::Neighbor& neighbor, const rina::EnrollToDIFRequestEvent& event) {
	neighbor_ = neighbor;
	event_ = event;
}

const rina::Neighbor& EnrollmentRequest::get_neighbor() const {
	return neighbor_;
}

void EnrollmentRequest::set_neighbor(const rina::Neighbor &neighbor) {
	neighbor_ = neighbor;
}

const rina::EnrollToDIFRequestEvent& EnrollmentRequest::get_event() const{
	return event_;
}

void EnrollmentRequest::set_event(const rina::EnrollToDIFRequestEvent& event) {
	event_ = event;
}

// CLASS NotificationPolicy
NotificationPolicy::NotificationPolicy(const std::list<unsigned int>& cdap_session_ids) {
	cdap_session_ids_ = cdap_session_ids;
}

const std::list<unsigned int>& NotificationPolicy::get_cdap_session_ids() const {
	return cdap_session_ids_;
}

}

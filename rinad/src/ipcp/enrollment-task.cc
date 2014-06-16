//
// Enrollment Task
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

#include "enrollment-task.h"

namespace rinad {

//	CLASS EnrollmentInformationRequest
const std::string EnrollmentInformationRequest::ENROLLMENT_INFO_OBJECT_NAME = RIBObjectNames::SEPARATOR + RIBObjectNames::DAF +
			RIBObjectNames::SEPARATOR + RIBObjectNames::MANAGEMENT + RIBObjectNames::SEPARATOR + RIBObjectNames::ENROLLMENT;

EnrollmentInformationRequest::EnrollmentInformationRequest() {
	address_ = 0;
}

unsigned int EnrollmentInformationRequest::get_address() const {
	return address_;
}

void EnrollmentInformationRequest::set_address(unsigned int address) {
	address_ = address;
}

const std::list<rina::ApplicationProcessNamingInformation>&
EnrollmentInformationRequest::get_supporting_difs() const {
	return supporting_difs_;
}

void EnrollmentInformationRequest::set_supporting_difs(
		const std::list<rina::ApplicationProcessNamingInformation> &supporting_difs) {
	supporting_difs_ = supporting_difs;
}

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


}

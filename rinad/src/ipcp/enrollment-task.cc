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


}

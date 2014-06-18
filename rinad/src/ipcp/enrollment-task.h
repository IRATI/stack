/*
 * Enrollment Task
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

#ifndef IPCP_ENROLLMENT_TASK_HH
#define IPCP_ENROLLMENT_TASK_HH

#ifdef __cplusplus

#include "ipcp/components.h"

namespace rinad {

/// The object that contains all the information
/// that is required to initiate an enrollment
/// request (send as the objectvalue of a CDAP M_START
/// message, as specified by the Enrollment spec)
class EnrollmentInformationRequest {
public:
	static const std::string ENROLLMENT_INFO_OBJECT_NAME;

	EnrollmentInformationRequest();
	unsigned int get_address() const;
	void set_address(unsigned int address);
	const std::list<rina::ApplicationProcessNamingInformation>& get_supporting_difs() const;
	void set_supporting_difs(const std::list<rina::ApplicationProcessNamingInformation> &supporting_difs);

private:
	/// The address of the IPC Process that requests
	///to join a DIF
	unsigned int address_;
	std::list<rina::ApplicationProcessNamingInformation> supporting_difs_;
};

}

#endif

#endif

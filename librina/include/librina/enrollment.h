/*
 * Enrollment
 *
 *    Bernat Gaston         <bernat.gaston@i2cat.net>
 *    Eduard Grasa          <eduard.grasa@i2cat.net>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Vincenzo Maffione <v.maffione@nextworks.it>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

#ifndef LIBRINA_ENROLLMENT_H
#define LIBRINA_ENROLLMENT_H

#ifdef __cplusplus

#include <limits>

#include "irm.h"
#include "timer.h"
#include "security-manager.h"
#include "rib_v2.h"

namespace rina {

/**
 * The App Manager requests the application to enroll to a DAF,
 * through neighbour neighbourName, which can be reached by allocating
 * a flow through the supportingDIFName
 */
class EnrollToDAFRequestEvent: public IPCEvent {
public:
        /** The DAF to enroll to */
        ApplicationProcessNamingInformation dafName;

        /** The N-1 DIF name to allocate a flow to the member */
        ApplicationProcessNamingInformation supportingDIFName;

        /** The neighbor to contact */
        ApplicationProcessNamingInformation neighborName;

        int current_enroll_attempts;

        bool prepare_for_handover;

        rina::ApplicationProcessNamingInformation disc_neigh_name;

        EnrollToDAFRequestEvent() : current_enroll_attempts(0),
        		prepare_for_handover(false){ };
        EnrollToDAFRequestEvent(
                const ApplicationProcessNamingInformation& daf,
                const ApplicationProcessNamingInformation& supportingDIF,
                const ApplicationProcessNamingInformation& neighbor,
                unsigned int sequenceNumber,
		unsigned int ctrl_port, unsigned short ipcp_id)
        	: IPCEvent(ENROLL_TO_DIF_REQUEST_EVENT, sequenceNumber, ctrl_port, ipcp_id),
                	dafName(daf), supportingDIFName(supportingDIF),
                	neighborName(neighbor), current_enroll_attempts(0),
			prepare_for_handover(false) { };
        EnrollToDAFRequestEvent(
                const ApplicationProcessNamingInformation& daf,
                const ApplicationProcessNamingInformation& supportingDIF,
                const ApplicationProcessNamingInformation& neighbor,
		bool prepare,
		const ApplicationProcessNamingInformation& disc_neigh,
                unsigned int sequenceNumber,
		unsigned int ctrl_port, unsigned short ipcp_id)
        		: IPCEvent(ENROLL_TO_DIF_REQUEST_EVENT, sequenceNumber, ctrl_port, ipcp_id),
                	dafName(daf), supportingDIFName(supportingDIF),
                	neighborName(neighbor), current_enroll_attempts(0),
			prepare_for_handover(prepare), disc_neigh_name(disc_neigh) { };
};

class DisconnectNeighborRequestEvent: public IPCEvent {
public:
        /** The neighbor to contact */
        ApplicationProcessNamingInformation neighborName;

        DisconnectNeighborRequestEvent() { };
        DisconnectNeighborRequestEvent(
                const ApplicationProcessNamingInformation& neighbor,
                unsigned int sequenceNumber,
		unsigned int ctrl_port, unsigned short ipcp_id)
        : IPCEvent(DISCONNECT_NEIGHBOR_REQUEST_EVENT, sequenceNumber, ctrl_port, ipcp_id),
                	neighborName(neighbor) { };
};

/// Contains the information of an enrollment request
class EnrollmentRequest
{
public:
	EnrollmentRequest() : ipcm_initiated_(false),
	                      enrollment_attempts(0),
			      abort_timer_task(0) {};
	EnrollmentRequest(const rina::Neighbor& neighbor) : neighbor_(neighbor),
		ipcm_initiated_(false),
		enrollment_attempts(0),
		 abort_timer_task(0) { };
	EnrollmentRequest(const rina::Neighbor& neighbor,
                          const EnrollToDAFRequestEvent & event) : neighbor_(neighbor),
                 event_(event), ipcm_initiated_(true),
		 enrollment_attempts(0),
		 abort_timer_task(0) { };

	/// The neighbor to enroll to
	Neighbor neighbor_;

	/// The request event that triggered the Enrollment
	EnrollToDAFRequestEvent event_;

	/// True if the enrollment request came via the IPC Manager
	bool ipcm_initiated_;

	/// Current number of enrollment attempts
	unsigned int enrollment_attempts;

	TimerTask * abort_timer_task;
};

/// Interface that must be implementing by classes that provide
/// the behavior of an enrollment task
class IEnrollmentTask : public rina::ApplicationEntity,
			public rina::cacep::AppConHandlerInterface {
public:
	IEnrollmentTask() : rina::ApplicationEntity(ApplicationEntity::ENROLLMENT_TASK_AE_NAME) { };
	virtual ~IEnrollmentTask() { };
	virtual std::list<Neighbor> get_neighbors() = 0;
	virtual void add_neighbor(const Neighbor& neighbor) = 0;
	virtual void add_or_update_neighbor(const Neighbor& neighbor) = 0;
	virtual void remove_neighbor(const std::string& neighbor_key) = 0;
	virtual std::list<std::string> get_enrolled_app_names() = 0;

	/// Process a request to initiate enrollment with a new Neighbor, triggered by the IPC Manager
	/// @param event
	virtual void processEnrollmentRequestEvent(const EnrollToDAFRequestEvent& event) = 0;

	/// Process a request to disconnect from a Neighbor, triggered by the IPC Manager
	/// @param event
	virtual void processDisconnectNeighborRequestEvent(const DisconnectNeighborRequestEvent& event) = 0;

	/// Starts the enrollment program
	/// @param cdapMessage
	/// @param cdapSessionDescriptor
	virtual void initiateEnrollment(const EnrollmentRequest& request) = 0;

	/// Called by the enrollment state machine when the enrollment request has been completed,
	/// either successfully or unsuccessfully
	/// @param neighbor the App process we were trying to enroll to
	/// @param enrollee true if this App process is the one that initiated the
	/// enrollment sequence (i.e. it is the application process that wants to
	/// join the DIF)
	virtual void enrollmentCompleted(const rina::Neighbor& neighbor, bool enrollee,
				 	 bool prepare_handover,
					 const rina::ApplicationProcessNamingInformation& disc_neigh_name) = 0;

	/// Called by the enrollment state machine when the enrollment sequence fails
	/// @param remotePeer
	/// @param portId
	/// @param enrollee
	/// @param sendMessage
	/// @param reason
	virtual void enrollmentFailed(const ApplicationProcessNamingInformation& remotePeerNamingInfo,
                                      int portId,
				      int internal_portId,
                                      const std::string& reason,
                                      bool sendReleaseMessage) = 0;

	/// Finds out if the App process is already enrolled to the App process identified by
	/// the provided apNamingInfo
	/// @param apNamingInfo
	/// @return
	virtual bool isEnrolledTo(const std::string& applicationProcessName) = 0;

	/// Callback used to signal the enrollment task that authentication completed
	/// successfully (if success = true) or failed (if success = false)
	virtual void authentication_completed(int port_id, bool success) = 0;
};

}

#endif

#endif

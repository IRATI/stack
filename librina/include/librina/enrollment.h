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

#include "rib.h"
#include "timer.h"

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

        EnrollToDAFRequestEvent() { };
        EnrollToDAFRequestEvent(
                const ApplicationProcessNamingInformation& daf,
                const ApplicationProcessNamingInformation& supportingDIF,
                const ApplicationProcessNamingInformation& neighbor,
                unsigned int sequenceNumber) : IPCEvent(ENROLL_TO_DIF_REQUEST_EVENT, sequenceNumber),
                	dafName(daf), supportingDIFName(supportingDIF),
                	neighborName(neighbor) { };
};

/// Contains the information of an enrollment request
class EnrollmentRequest
{
public:
	EnrollmentRequest(Neighbor * neighbor) : neighbor_(neighbor),
		ipcm_initiated_(false) { };
	EnrollmentRequest(Neighbor * neighbor,
                          const EnrollToDAFRequestEvent & event) : neighbor_(neighbor),
                 event_(event), ipcm_initiated_(true) { };

	/// The neighbor to enroll to
	Neighbor * neighbor_;

	/// The request event that triggered the Enrollment
	EnrollToDAFRequestEvent event_;

	/// True if the enrollment request came via the IPC Manager
	bool ipcm_initiated_;
};

class NeighborRIBObject: public SimpleSetMemberRIBObject {
public:
	NeighborRIBObject(IRIBDaemon * rib_daemon,
			  const std::string& object_class,
			  const std::string& object_name,
			  const rina::Neighbor* neighbor);
	std::string get_displayable_value();
};

class NeighborSetRIBObject: public BaseRIBObject {
public:
	static const std::string NEIGHBOR_SET_RIB_OBJECT_CLASS;
	static const std::string NEIGHBOR_RIB_OBJECT_CLASS;
	static const std::string NEIGHBOR_SET_RIB_OBJECT_NAME;

	NeighborSetRIBObject(ApplicationProcess * app, IRIBDaemon * rib_daemon);
	~NeighborSetRIBObject();
	const void* get_value() const;
	void remoteCreateObject(void * object_value, const std::string& object_name,
			int invoke_id, rina::CDAPSessionDescriptor * session_descriptor);
	void createObject(const std::string& objectClass,
			const std::string& objectName,
			const void* objectValue);

private:
	void populateNeighborsToCreateList(rina::Neighbor * neighbor,
			std::list<rina::Neighbor *> * list);
	void createNeighbor(rina::Neighbor * neighbor);

	Lockable * lock_;
	ApplicationProcess * app_;
};

/// Interface that must be implementing by classes that provide
/// the behavior of an enrollment task
class IEnrollmentTask : public rina::ApplicationEntity,
			public rina::CACEPHandler {
public:
	IEnrollmentTask() : rina::ApplicationEntity(ApplicationEntity::ENROLLMENT_TASK_AE_NAME) { };
	virtual ~IEnrollmentTask(){};
	virtual const std::list<Neighbor *> get_neighbors() const = 0;
	virtual const std::list<std::string> get_enrolled_ipc_process_names() const = 0;

	/// Process a request to initiate enrollment with a new Neighbor, triggered by the IPC Manager
	/// @param event
	virtual void processEnrollmentRequestEvent(EnrollToDAFRequestEvent * event) = 0;

	/// Starts the enrollment program
	/// @param cdapMessage
	/// @param cdapSessionDescriptor
	virtual void initiateEnrollment(EnrollmentRequest * request) = 0;

	/// Called by the enrollment state machine when the enrollment request has been completed,
	/// either successfully or unsuccessfully
	/// @param neighbor the App process we were trying to enroll to
	/// @param enrollee true if this App process is the one that initiated the
	/// enrollment sequence (i.e. it is the application process that wants to
	/// join the DIF)
	virtual void enrollmentCompleted(Neighbor * neighbor,
                                         bool enrollee) = 0;

	/// Called by the enrollment state machine when the enrollment sequence fails
	/// @param remotePeer
	/// @param portId
	/// @param enrollee
	/// @param sendMessage
	/// @param reason
	virtual void enrollmentFailed(const rina::ApplicationProcessNamingInformation& remotePeerNamingInfo,
                                      int portId,
                                      const std::string& reason,
                                      bool enrolle,
                                      bool sendReleaseMessage) = 0;

	/// Finds out if the App process is already enrolled to the App process identified by
	/// the provided apNamingInfo
	/// @param apNamingInfo
	/// @return
	virtual bool isEnrolledTo(const std::string& applicationProcessName) const = 0;
};

// The common elements of an enrollment state machine
class IEnrollmentStateMachine : public BaseCDAPResponseMessageHandler {
public:
	static const std::string STATE_NULL;
	static const std::string STATE_ENROLLED;

	IEnrollmentStateMachine(ApplicationProcess * app, bool isIPCP,
			const rina::ApplicationProcessNamingInformation& remote_naming_info,
			int timeout, rina::ApplicationProcessNamingInformation * supporting_dif_name);
	~IEnrollmentStateMachine();

	/// Called by the EnrollmentTask when it got an M_RELEASE message
	/// @param invoke_id the invoke_id of the release message
	/// @param cdapSessionDescriptor
	void release(int invoke_id,
			rina::CDAPSessionDescriptor * session_descriptor);

	/// Called by the EnrollmentTask when it got an M_RELEASE_R message
	/// @param result
	/// @param result_reason
	/// @param session_descriptor
	void releaseResponse(int result, const std::string& result_reason,
			rina::CDAPSessionDescriptor * session_descriptor);

	/// Called by the EnrollmentTask when the flow supporting the CDAP session with the remote peer
	/// has been deallocated
	/// @param cdapSessionDescriptor
	void flowDeallocated(rina::CDAPSessionDescriptor * cdapSessionDescriptor);

	rina::Neighbor * remote_peer_;
	bool enroller_;
	std::string state_;

protected:
	bool isValidPortId(const rina::CDAPSessionDescriptor * cdapSessionDescriptor);

	/// Called by the enrollment state machine when the enrollment sequence fails
	void abortEnrollment(const rina::ApplicationProcessNamingInformation& remotePeerNamingInfo,
			     int portId, const std::string& reason, bool enrollee,
			     bool sendReleaseMessage);

	/// Create or update the neighbor information in the RIB
	/// @param enrolled true if the neighbor is enrolled, false otherwise
	void createOrUpdateNeighborInformation(bool enrolled);

	/// Send the neighbors (if any)
	void sendNeighbors();

	/// Gets the object value from the RIB and send it as a CDAP Mesage
	/// @param objectClass the class of the object to be send
	/// @param objectName the name of the object to be send
	void sendCreateInformation(const std::string& objectClass, const std::string& objectName);

	ApplicationProcess * app_;
	IRIBDaemon * rib_daemon_;
	IEnrollmentTask * enrollment_task_;
	int timeout_;
	Timer * timer_;
	Lockable * lock_;
	int port_id_;
	TimerTask * last_scheduled_task_;
	bool isIPCP_;
};

}

#endif

#endif

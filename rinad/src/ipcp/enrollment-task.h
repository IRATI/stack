/*
 * Enrollment Task
 *
 *    Bernat Gaston <bernat.gaston@i2cat.net>
 *    Eduard Grasa <eduard.grasa@i2cat.net>
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

#ifndef IPCP_ENROLLMENT_TASK_HH
#define IPCP_ENROLLMENT_TASK_HH

#include <map>

#include "common/concurrency.h"
#include "ipcp/components.h"
#include <librina/internal-events.h>
#include "ipcp/ipc-process.h"

namespace rinad {

class NeighborRIBObj: public rina::rib::RIBObj {
public:
	NeighborRIBObj(rina::Neighbor* neigh);
	const std::string get_displayable_value() const;
	const std::string& get_class() const {
		return class_name;
	};

	void read(const rina::cdap_rib::con_handle_t &con,
		  const std::string& fqn,
		  const std::string& class_,
		  const rina::cdap_rib::filt_info_t &filt,
		  const int invoke_id,
		  rina::cdap_rib::obj_info_t &obj_reply,
		  rina::cdap_rib::res_info_t& res);

	static void create_cb(const rina::rib::rib_handle_t rib,
		  const rina::cdap_rib::con_handle_t &con,
		  const std::string& fqn, const std::string& class_,
		  const rina::cdap_rib::filt_info_t &filt,
		  const int invoke_id, const rina::ser_obj_t &obj_req,
		  rina::cdap_rib::obj_info_t &obj_reply,
		  rina::cdap_rib::res_info_t& res);
	const static std::string class_name;
	const static std::string object_name_prefix;

private:
	rina::Neighbor * neighbor;
	static bool createNeighbor(rina::Neighbor &object);
};

class NeighborsRIBObj: public IPCPRIBObj {
public:
	NeighborsRIBObj(IPCProcess * ipcp);
	const std::string& get_class() const {
		return class_name;
	};

	//Create
	void create(const rina::cdap_rib::con_handle_t &con,
		    const std::string& fqn,
		    const std::string& class_,
		    const rina::cdap_rib::filt_info_t &filt,
		    const int invoke_id,
		    const rina::ser_obj_t &obj_req,
		    rina::ser_obj_t &obj_reply,
		    rina::cdap_rib::res_info_t& res);

	const static std::string class_name;
	const static std::string object_name;
};

class WatchdogRIBObject;

/// Sends a CDAP message to check that the CDAP connection
/// is still working ok
class WatchdogTimerTask: public rina::TimerTask {
public:
	WatchdogTimerTask(WatchdogRIBObject * watchdog, rina::Timer * timer, int delay);
	void run();

private:
	WatchdogRIBObject * watchdog_;
	rina::Timer * timer_;
	int delay_;
};

class WatchdogRIBObject: public IPCPRIBObj, public rina::rib::RIBOpsRespHandler {
public:
	WatchdogRIBObject(IPCProcess * ipc_process,
			  int wdog_period_ms,
			  int declared_dead_int_ms);
	~WatchdogRIBObject();

	const std::string& get_class() const {
		return class_name;
	};

	void read(const rina::cdap_rib::con_handle_t &con,
		  const std::string& fqn,
		  const std::string& class_,
		  const rina::cdap_rib::filt_info_t &filt,
		  const int invoke_id,
		  rina::cdap_rib::obj_info_t &obj_reply,
		  rina::cdap_rib::res_info_t& res);

	/// Send watchdog messages to the IPC processes that are our neighbors and we're enrolled to
	void sendMessages();

	/// Take advantadge of the watchdog message responses to measure the RTT,
	/// and store it in the neighbor object (average of the last 4 RTTs)
	void remoteReadResult(const rina::cdap_rib::con_handle_t &con,
			      const rina::cdap_rib::obj_info_t &obj,
			      const rina::cdap_rib::res_info_t &res);

	const static std::string class_name;
	const static std::string object_name;

private:
	rina::Timer * timer_;
	int wathchdog_period_;
	int declared_dead_interval_;
	std::map<std::string, int> neighbor_statistics_;
	rina::Lockable * lock_;
};

/// Handles the operations related to the "daf.management.naming.currentsynonym" objects
class AddressRIBObject: public IPCPRIBObj {
public:
	AddressRIBObject(IPCProcess * ipc_process);
	~AddressRIBObject();
	const std::string get_displayable_value() const;
	const std::string& get_class() const {
		return class_name;
	};

	void read(const rina::cdap_rib::con_handle_t &con,
		  const std::string& fqn,
		  const std::string& class_,
		  const rina::cdap_rib::filt_info_t &filt,
		  const int invoke_id,
		  rina::cdap_rib::obj_info_t &obj_reply,
		  rina::cdap_rib::res_info_t& res);

	const static std::string class_name;
	const static std::string object_name;
};

// The common elements of an enrollment state machine
class IEnrollmentStateMachine : public rina::rib::RIBOpsRespHandler {
	friend class EnrollmentFailedTimerTask;
public:
	static const std::string STATE_NULL;
	static const std::string STATE_ENROLLED;

	IEnrollmentStateMachine(IPCProcess * ipcp,
			const rina::ApplicationProcessNamingInformation& remote_naming_info,
			int timeout, rina::ApplicationProcessNamingInformation * supporting_dif_name);
	virtual ~IEnrollmentStateMachine();

	/// Called by the EnrollmentTask when it got an M_RELEASE message
	/// @param invoke_id the invoke_id of the release message
	/// @param con_handle
	void release(int invoke_id,
		     const rina::cdap_rib::con_handle_t &con_handle);

	/// Called by the EnrollmentTask when it got an M_RELEASE_R message
	/// @param result
	/// @param result_reason
	/// @param session_descriptor
	void releaseResult(const rina::cdap_rib::res_info_t &res,
			   const rina::cdap_rib::con_handle_t &con_handle);

	virtual void process_authentication_message(const rina::cdap::CDAPMessage& message,
			    	    	    	    const rina::cdap_rib::con_handle_t &con) = 0;

	virtual void authentication_completed(bool success) = 0;

	/// Called by the EnrollmentTask when the flow supporting the CDAP session with the remote peer
	/// has been deallocated
	/// @param cdapSessionDescriptor
	void flowDeallocated(int portId);

	rina::Neighbor remote_peer_;
	bool enroller_;
	std::string state_;

protected:
	bool isValidPortId(int portId);

	/// Called by the enrollment state machine when the enrollment sequence fails
	void abortEnrollment(const rina::ApplicationProcessNamingInformation& remotePeerNamingInfo,
			     int portId,
			     const std::string& reason,
			     bool sendReleaseMessage);

	/// Create or update the neighbor information in the RIB
	/// @param enrolled true if the neighbor is enrolled, false otherwise
	void createOrUpdateNeighborInformation(bool enrolled);

	/// Send the neighbors (if any)
	void sendNeighbors();

	IPCProcess * ipcp_;
	IPCPRIBDaemon * rib_daemon_;
	rina::IEnrollmentTask * enrollment_task_;
	rina::IAuthPolicySet * auth_ps_;
	int timeout_;
	rina::Timer timer_;
	rina::Lockable lock_;
	rina::cdap_rib::con_handle_t con;
	rina::TimerTask * last_scheduled_task_;
};

class EnrollmentFailedTimerTask: public rina::TimerTask {
public:
	EnrollmentFailedTimerTask(IEnrollmentStateMachine * state_machine,
			const std::string& reason);
	~EnrollmentFailedTimerTask() throw() {};
	void run();

	IEnrollmentStateMachine * state_machine_;
	std::string reason_;
};

class EnrollmentTask: public IPCPEnrollmentTask, public rina::InternalEventListener {
public:
	static const std::string ENROLL_TIMEOUT_IN_MS;
	static const std::string WATCHDOG_PERIOD_IN_MS;
	static const std::string DECLARED_DEAD_INTERVAL_IN_MS;
	static const std::string NEIGHBORS_ENROLLER_PERIOD_IN_MS;
	static const std::string MAX_ENROLLMENT_RETRIES;

	EnrollmentTask();
	~EnrollmentTask();
	void set_application_process(rina::ApplicationProcess * ap);
	void set_dif_configuration(const rina::DIFConfiguration& dif_configuration);
	void eventHappened(rina::InternalEvent * event);
	const std::list<rina::Neighbor> get_neighbors() const;
	std::list<rina::Neighbor*> get_neighbor_pointers();
	void add_neighbor(const rina::Neighbor& neighbor);
	void add_or_update_neighbor(const rina::Neighbor& neighbor);
	void remove_neighbor(const std::string& neighbor_key);
	bool isEnrolledTo(const std::string& applicationProcessName);
	const std::list<std::string> get_enrolled_app_names() const;
	void processEnrollmentRequestEvent(rina::EnrollToDAFRequestEvent * event);
	void initiateEnrollment(const rina::EnrollmentRequest& request);
	void connect(const rina::cdap::CDAPMessage& message,
	             const rina::cdap_rib::con_handle_t &con);
	void connectResult(const rina::cdap_rib::res_info_t &res,
			   const rina::cdap_rib::con_handle_t &con,
			   const rina::cdap_rib::auth_policy_t& auth);
	void release(int invoke_id,
		     const rina::cdap_rib::con_handle_t &con);
	void releaseResult(const rina::cdap_rib::res_info_t &res,
			   const rina::cdap_rib::con_handle_t &con);
	void process_authentication_message(const rina::cdap::CDAPMessage& message,
					    const rina::cdap_rib::con_handle_t &con);
	void authentication_completed(int port_id, bool success);
	void enrollmentFailed(const rina::ApplicationProcessNamingInformation& remotePeerNamingInfo,
			int portId, const std::string& reason, bool sendReleaseMessage);
	void enrollmentCompleted(const rina::Neighbor& neighbor, bool enrollee);
	IEnrollmentStateMachine * getEnrollmentStateMachine(int portId, bool remove);
	void deallocateFlow(int portId);
	void add_enrollment_state_machine(int portId, IEnrollmentStateMachine * stateMachine);

	/// The maximum time to wait between steps of the enrollment sequence (in ms)
	int timeout_;

	/// Maximum number of enrollment attempts
	unsigned int max_num_enroll_attempts_;

	/// Watchdog period in ms
	int watchdog_per_ms_;

	/// The neighbor declared dead interval
	int declared_dead_int_ms_;

	/// The neighbor enroller period in ms
	int neigh_enroll_per_ms_;

private:
	void _add_neighbor(const rina::Neighbor& neighbor);

	void subscribeToEvents();

	///  If the N-1 flow with the neighbor is still allocated, request its deallocation
	/// @param deadEvent
	void neighborDeclaredDead(rina::NeighborDeclaredDeadEvent * deadEvent);

	/// Called by the RIB Daemon when the flow supporting the CDAP session with the remote peer
	/// has been deallocated
	/// @param event
	void nMinusOneFlowDeallocated(rina::NMinusOneFlowDeallocatedEvent  * event);

	/// Called when a new N-1 flow has been allocated
	// @param portId
	void nMinusOneFlowAllocated(rina::NMinusOneFlowAllocatedEvent * flowEvent);

	/// Called when a new N-1 flow allocation has failed
	/// @param portId
	/// @param flowService
	/// @param resultReason
	void nMinusOneFlowAllocationFailed(rina::NMinusOneFlowAllocationFailedEvent * event);

	IPCPRIBDaemon * rib_daemon_;
	rina::InternalEventManager * event_manager_;
	rina::IPCResourceManager * irm_;
	INamespaceManager * namespace_manager_;
	rina::Thread * neighbors_enroller_;

	rina::Lockable lock_;

	/// Stores the enrollment state machines, one per remote IPC process that this IPC
	/// process is enrolled to.
	rina::ThreadSafeMapOfPointers<int, IEnrollmentStateMachine> state_machines_;

	rina::ThreadSafeMapOfPointers<unsigned int, rina::EnrollmentRequest> port_ids_pending_to_be_allocated_;

	rina::ThreadSafeMapOfPointers<std::string, rina::Neighbor> neighbors;
	IPCPEnrollmentTaskPS * ipcp_ps;
};

/// Handles the operations related to the "daf.management.operationalStatus" object
class OperationalStatusRIBObject: public IPCPRIBObj {
public:
	OperationalStatusRIBObject(IPCProcess * ipc_process);

	void start(const rina::cdap_rib::con_handle_t &con,
		   const std::string& fqn,
		   const std::string& class_,
		   const rina::cdap_rib::filt_info_t &filt,
		   const int invoke_id,
		   const rina::ser_obj_t &obj_req,
		   rina::ser_obj_t &obj_reply,
		   rina::cdap_rib::res_info_t& res);

	void read(const rina::cdap_rib::con_handle_t &con,
		  const std::string& fqn,
		  const std::string& class_,
		  const rina::cdap_rib::filt_info_t &filt,
		  const int invoke_id,
		  rina::cdap_rib::obj_info_t &obj_reply,
		  rina::cdap_rib::res_info_t& res);
	void startObject();
	void stopObject();

	const std::string get_displayable_value() const;

	const std::string& get_class() const {
		return class_name;
	};

	const static std::string class_name;
	const static std::string object_name;

private:
	void sendErrorMessage(const rina::cdap_rib::con_handle_t &con);
	EnrollmentTask * enrollment_task_;
};

} //namespace rinad

#endif //IPCP_ENROLLMENT_TASK_HH

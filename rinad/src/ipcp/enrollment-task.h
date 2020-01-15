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
	NeighborRIBObj(const std::string& neigh_key);
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
	std::string neighbor_key;
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

	//Allow a neighbor to update its address
	void write(const rina::cdap_rib::con_handle_t &con,
		   const std::string& fqn,
		   const std::string& class_,
		   const rina::cdap_rib::filt_info_t &filt,
		   const int invoke_id,
		   const rina::ser_obj_t &obj_req,
		   rina::ser_obj_t &obj_reply,
		   rina::cdap_rib::res_info_t& res);

	const static std::string class_name;
	const static std::string object_name;

private:
	rina::Lockable lock;
};

class WatchdogRIBObject;

/// Sends a CDAP message to check that the CDAP connection
/// is still working ok
class WatchdogTimerTask: public rina::TimerTask {
public:
	WatchdogTimerTask(WatchdogRIBObject * watchdog, rina::Timer * timer, int delay);
	void run();
	std::string name() const {
		return "watchdog";
	}

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
	static const std::string STATE_TERMINATED;

	IEnrollmentStateMachine(IPCProcess * ipcp,
				const rina::ApplicationProcessNamingInformation& remote_naming_info,
				int timeout,
				const rina::ApplicationProcessNamingInformation& supporting_dif_name,
				rina::Timer * timer);
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
	/// @param portid
	void flowDeallocated(int portId);

	/// Called by the Enrollment Task when the supporting internal flow allocation result is available
	virtual void internal_flow_allocate_result(int portId,
						   int fd,
					   	   const std::string& result_reason) = 0;

	virtual void operational_status_start(int invoke_id,
					      const rina::ser_obj_t &obj_req) = 0;

	void reset_state(void);
	std::string get_state();

	rina::Neighbor remote_peer_;
	bool enroller_;
	rina::cdap_rib::con_handle_t con;

	/// The enrollment request, store it to be able to retry enrollment afterwards
	rina::EnrollmentRequest enr_request;
	bool being_destroyed;

protected:
	bool isValidPortId(int portId);

	/// Called by the enrollment state machine when the enrollment sequence fails
	void abortEnrollment(const std::string& reason,
			     bool sendReleaseMessage);

	/// Create or update the neighbor information in the RIB
	/// @param enrolled true if the neighbor is enrolled, false otherwise
	void createOrUpdateNeighborInformation(bool enrolled);

	/// Send the neighbors (if any)
	void sendNeighbors();

	IPCProcess * ipcp_;
	IPCPRIBDaemon * rib_daemon_;
	IPCPEnrollmentTask * enrollment_task_;
	rina::IAuthPolicySet * auth_ps_;
	int timeout_;
	rina::Timer * timer;
	rina::Lockable lock_;
	rina::TimerTask * last_scheduled_task_;
	std::string state_;
};

class AbortEnrollmentTimerTask: public rina::TimerTask {
public:
	AbortEnrollmentTimerTask(rina::IEnrollmentTask * enr_task,
				 const rina::ApplicationProcessNamingInformation& remotePeerNamingInfo,
				 int portId,
				 int internal_portId,
				 const std::string& reason,
				 bool sendReleaseMessage);
	~AbortEnrollmentTimerTask() throw() {};
	void run();
	std::string name() const {
		return "abort-enrollment";
	}

private:
	rina::IEnrollmentTask * etask;
	rina::ApplicationProcessNamingInformation peer_name;
	int port_id;
	int internal_portId;
	std::string reason;
	bool send_message;
};

class DestroyESMTimerTask : public rina::TimerTask {
public:
	DestroyESMTimerTask(IEnrollmentStateMachine * sm);
	~DestroyESMTimerTask() throw() {};
	void run();
	std::string name() const {
		return "destroy-esm";
	}

private:
	IEnrollmentStateMachine * state_machine;
};

class DeallocateFlowTimerTask : public rina::TimerTask {
public:
	DeallocateFlowTimerTask(IPCProcess * ipcp,
				int port_id,
				bool internal);
	~DeallocateFlowTimerTask() throw() {};
	void run();
	std::string name() const {
		return "deallocate-flow";
	}

private:
	IPCProcess * ipcp;
	int port_id;
	bool internal;
};

class RetryEnrollmentTimerTask: public rina::TimerTask {
public:
	RetryEnrollmentTimerTask(rina::IEnrollmentTask * enr_task,
				 const rina::EnrollmentRequest& request);
	~RetryEnrollmentTimerTask() throw() {};
	void run();
	std::string name() const {
		return "retry-enrollment";
	}

private:
	rina::IEnrollmentTask * etask;
	rina::EnrollmentRequest erequest;
};

class EnrollmentTask: public IPCPEnrollmentTask, public rina::InternalEventListener {
public:
	static const std::string ENROLL_TIMEOUT_IN_MS;
	static const std::string WATCHDOG_PERIOD_IN_MS;
	static const std::string DECLARED_DEAD_INTERVAL_IN_MS;
	static const std::string MAX_ENROLLMENT_RETRIES;
	static const std::string USE_RELIABLE_N_FLOW;
	static const std::string N1_FLOWS;
	static const std::string N1_DIFS_PEER_DISCOVERY;
	static const std::string PEER_DISCOVERY_PERIOD_IN_MS;
	static const std::string MAX_PEER_DISCOVERY_ATTEMPTS;

	EnrollmentTask();
	~EnrollmentTask();
	void set_application_process(rina::ApplicationProcess * ap);
	void set_dif_configuration(const rina::DIFInformation& dif_information);
	void eventHappened(rina::InternalEvent * event);
	std::list<rina::Neighbor> get_neighbors();
	void add_neighbor(const rina::Neighbor& neighbor);
	void add_or_update_neighbor(const rina::Neighbor& neighbor);
	rina::Neighbor get_neighbor(const std::string& neighbor_key);
	void remove_neighbor(const std::string& neighbor_key);
	bool isEnrolledTo(const std::string& applicationProcessName);
	std::list<std::string> get_enrolled_app_names();
	void processEnrollmentRequestEvent(const rina::EnrollToDAFRequestEvent& event);
	void processDisconnectNeighborRequestEvent(const rina::DisconnectNeighborRequestEvent& event);
	void initiateEnrollment(const rina::EnrollmentRequest& request);
	void connect(const rina::cdap::CDAPMessage& message,
	             rina::cdap_rib::con_handle_t &con);
	void connectResult(const rina::cdap::CDAPMessage& message,
			   rina::cdap_rib::con_handle_t &con);
	void release(int invoke_id,
		     const rina::cdap_rib::con_handle_t &con);
	void releaseResult(const rina::cdap_rib::res_info_t &res,
			   const rina::cdap_rib::con_handle_t &con);
	void process_authentication_message(const rina::cdap::CDAPMessage& message,
					    const rina::cdap_rib::con_handle_t &con);
	void authentication_completed(int port_id, bool success);
	void enrollmentFailed(const rina::ApplicationProcessNamingInformation& remotePeerNamingInfo,
			      int portId,
			      int internal_portId,
			      const std::string& reason,
			      bool sendReleaseMessage);
	void enrollmentCompleted(const rina::Neighbor& neighbor, bool enrollee,
				 bool prepare_handover,
				 const rina::ApplicationProcessNamingInformation& disc_neigh_name);
	IEnrollmentStateMachine * getEnrollmentStateMachine(int portId, bool remove);
	void deallocateFlow(int portId);
	void add_enrollment_state_machine(int portId, IEnrollmentStateMachine * stateMachine);
	void update_neighbor_address(const rina::Neighbor& neighbor);
	void incr_neigh_enroll_attempts(const rina::Neighbor& neighbor);
	void watchdog_read(const std::string& remote_app_name);
	void watchdog_read_result(const std::string& remote_app_name,
				  int stored_time);
	void operational_status_start(int port_id,
				      int invoke_id,
			       	      const rina::ser_obj_t &obj_req);
	int get_con_handle_to_ipcp(unsigned int address,
				   rina::cdap_rib::con_handle_t& con);
	unsigned int get_con_handle_to_ipcp(const std::string& ipcp_name,
				            rina::cdap_rib::con_handle_t& con);
	int get_neighbor_info(rina::Neighbor& neigh);
	void clean_state(unsigned int port_id);

private:
	void parse_n1flows(const std::string& name,
			   const std::string& value);

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

	void internal_flow_allocated(rina::IPCPInternalFlowAllocatedEvent * event);
	void internal_flow_deallocated(rina::IPCPInternalFlowDeallocatedEvent * event);
	void internal_flow_allocation_failed(rina::IPCPInternalFlowAllocationFailedEvent * event);
	void addressChange(rina::AddressChangeEvent * event);

	void deallocate_flows_and_destroy_esm(IEnrollmentStateMachine * esm,
					      unsigned int port_id,
					      bool call_ps = true);

	int get_con_handle_to_ipcp_with_address(unsigned int address,
				   	   	rina::cdap_rib::con_handle_t& con);

	IPCPRIBDaemon * rib_daemon_;
	rina::InternalEventManager * event_manager_;
	rina::IPCResourceManager * irm_;
	INamespaceManager * namespace_manager_;

	rina::Lockable lock_;
	rina::Timer timer;

	/// Stores the enrollment state machines, one per remote IPC process that this IPC
	/// process is enrolled to.
	std::map<int, IEnrollmentStateMachine*> state_machines_;
	rina::ReadWriteLockable sm_lock;

	rina::ThreadSafeMapOfPointers<unsigned int, rina::EnrollmentRequest> port_ids_pending_to_be_allocated_;

	std::map<std::string, rina::Neighbor *> neighbors;
	rina::ReadWriteLockable neigh_lock;

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

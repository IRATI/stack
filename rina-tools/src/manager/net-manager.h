//
// Header file for the Network Manager
//
// Eduard Grasa <eduard.grasa@i2cat.net>
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//   1. Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//   2. Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.
//

#ifndef NET_MANAGER_HPP
#define NET_MANAGER_HPP

#include <string>
#include <librina/cdap_v2.h>
#include <librina/concurrency.h>
#include <librina/console.h>
#include <librina/irm.h>
#include <librina/rib_v2.h>
#include <librina/security-manager.h>

#include "dif-template-manager.h"

//Constants
#define PROMISE_TIMEOUT_S 8
#define PROMISE_RETRY_NSEC 10000000 //10ms
#define _PROMISE_1_SEC_NSEC 1000000000

class NetworkManager;

//
// Return codes
//
typedef enum netman_res{
	//Success
	NETMAN_SUCCESS = 0,

	//Return value will be deferred
	NETMAN_PENDING = 1,

	//Generic failure
	NETMAN_FAILURE = -1,

	//TODO: add more codes here...
}netman_res_t;


//fwd decl
class TransactionState;

//
// Promise base class
//
class Promise {

public:

	virtual ~Promise(){};

	//
	// Wait (blocking)
	//
	netman_res_t wait(void);

	//
	// Signal the condition
	//
	inline void signal(void){
		wait_cond.signal();
	}

	//
	// Timed wait (blocking)
	//
	// Return SUCCESS or NETMAN_PENDING if the operation did not finally
	// succeed
	//
	netman_res_t timed_wait(const unsigned int seconds);

	//
	// Return code
	//
	netman_res_t ret;

protected:
	//Protect setting of trans
	friend class TransactionState;

	//Transaction back reference
	TransactionState* trans;

	//Condition variable
	rina::ConditionVariable wait_cond;
};

//
// Create IPCP promise
//
class CreateIPCPPromise : public Promise {
public:
	int ipcp_id;
};

//
// This base class encapsulates the generics of any two-step
//
class TransactionState {
public:
	TransactionState(NetworkManager* callee_, Promise* promise_,
			const std::string& tid_):
				promise(promise_),
				tid(tid_),
				callee(callee_),
				finalised(false){
		if(promise){
			promise->ret = NETMAN_PENDING;
			promise->trans = this;
		}
	}
	virtual ~TransactionState(){};

	//
	// Used to recover the promise from the transaction
	//
	// @arg Template T: type of the promise
	//
	template<typename T>
	T* get_promise(){
		try{
			T* t = dynamic_cast<T*>(promise);
			return t;
		}catch(...){
			return NULL;
		}
	}

	//
	// This method and signals any existing set complete flag
	//
	void completed(netman_res_t _ret){
		rina::ScopedLock slock(mutex);

		if(finalised)
			return;

		if(!promise)
			return;

		promise->ret = _ret;
		promise->signal();
	}

	//Promise
	Promise* promise;

	//Transaction id
	const std::string tid;

	//Callee that generated the transaction
	NetworkManager* callee;

protected:
	//Protect abort call
	friend class Promise;

	//
	// @brief Abort the transaction (hard timeout)
	//
	// This particular method aborts the transaction due to a wait in the
	// promise that has timedout
	//
	// @ret On success, the transaction has been aborted. On failure, a
	// completed() call has successfully finished the transaction
	//
	bool abort(void){
		rina::ScopedLock slock(mutex);

		if(finalised)
			return false;

		return finalised = true;
	}

	//Completed flag
	bool finalised;

	// Mutex
	rina::Lockable mutex;
};

/**
* Standard System related transaction state
*/
class MASystemTransState: public TransactionState {
public:
	MASystemTransState(NetworkManager* callee, Promise* promise, int sys_id,
			   struct IPCPDescriptor * desc, const std::string& tid)
					: TransactionState(callee, promise, tid),
						system_id(sys_id), ipcp_desc(desc){}
	virtual ~MASystemTransState(){};

	//System identifier
	int system_id;

	// IPCP descriptor
	struct IPCPDescriptor * ipcp_desc;
};

//Class NMConsole
class NMConsole : public rina::UNIXConsole {
public:
	NMConsole(const std::string& socket_path, NetworkManager * nm);
	virtual ~NMConsole() throw() {};

private:
	NetworkManager * netman;
};

//Class SDUReader
class SDUReader : public rina::SimpleThread
{
public:
	SDUReader(int port_id, int fd_, NetworkManager * nm);
	~SDUReader() throw() {};
	int run();

private:
	int portid;
	int fd;
	NetworkManager * netman;
};

typedef enum nm_res{
	//Success
	NM_SUCCESS = 0,

	//Return value will be deferred
	NM_PENDING = 1,

	//Generic failure
	NM_FAILURE = -1,
} da_res_t;

struct ManagedSystem
{
	ManagedSystem();
	~ManagedSystem();

	rina::cdap_rib::con_handle_t con;
	rina::IAuthPolicySet * auth_ps_;
	int system_id;
	nm_res status;
	std::map<std::string, rina::rib::RIBObj*> objs_to_create;

	/* Map of IPC Processes, indexed by IPCP id */
	std::map<int, IPCPDescriptor*> ipcps;
};

// Class NMEnrollmentTask
class NMEnrollmentTask: public rina::cacep::AppConHandlerInterface,
			 public rina::ApplicationEntity
{
public:
	NMEnrollmentTask();
	~NMEnrollmentTask();

	void set_application_process(rina::ApplicationProcess * ap);
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

	void initiateEnrollment(const rina::ApplicationProcessNamingInformation& peer_name,
				const std::string& dif_name, int port_id);

	rina::Lockable lock;
	std::map<std::string, ManagedSystem *> enrolled_systems;

private:
	NetworkManager * nm;
};

// Class NM RIB Daemon
class NMRIBDaemon : public rina::rib::RIBDaemonAE
{
public:
	NMRIBDaemon(rina::cacep::AppConHandlerInterface *app_con_callback);
	~NMRIBDaemon();

	rina::rib::RIBDaemonProxy * getProxy();
        const rina::rib::rib_handle_t & get_rib_handle();
        int64_t addObjRIB(const std::string& fqn, rina::rib::RIBObj** obj);
        void removeObjRIB(const std::string& fqn, bool force = false);
        std::list<rina::rib::RIBObjectData> get_rib_objects_data(void);

private:
	//Handle to the RIB
	rina::rib::rib_handle_t rib;
	rina::rib::RIBDaemonProxy* ribd;
};

class DisconnectFromSystemTimerTask: public rina::TimerTask {
public:
	DisconnectFromSystemTimerTask(NetworkManager * nm, int fd) :
		netman(nm), fildesc(fd) {};
	~DisconnectFromSystemTimerTask() throw() {};
	void run();
	std::string name() const {
		return "disc-from-system";
	}

	NetworkManager * netman;
	int fildesc;
};

class ConsecutiveUnsignedIntegerGenerator {
public:
	ConsecutiveUnsignedIntegerGenerator() : counter_(0) {}
	unsigned int next() {
		rina::ScopedLock g(lock_);

		if (counter_ == UINT_MAX) {
			counter_ = 0;
		}
		counter_++;

		return counter_;
	}

private:
	unsigned int counter_;
	rina::Lockable lock_;
};

// Uses one thread per connected Management Agent (it is
// ok for demonstration purposes, consider changing to
// non-blocking I/O and a state machine approach to improve
// scalability if needed in the future)
class NetworkManager: public rina::ApplicationProcess, public rina::rib::RIBOpsRespHandler
{
public:
	NetworkManager(const std::string& app_name,
		       const std::string& app_instance,
		       const std::string& console_path,
		       const std::string& dif_templates_path);
        ~NetworkManager();

        void event_loop(std::list<std::string>& dif_names);
        unsigned int get_address() const;
        void disconnect_from_system(int fd);
        void disconnect_from_system_async(int fd);
        void enrollment_completed(struct ManagedSystem * system);

	void remoteReadResult(const rina::cdap_rib::con_handle_t &con,
			      const rina::cdap_rib::obj_info_t &obj,
			      const rina::cdap_rib::res_info_t &res,
			      const rina::cdap_rib::flags_t & flags);

	void remoteCreateResult(const rina::cdap_rib::con_handle_t &con,
				const rina::cdap_rib::obj_info_t &obj,
				const rina::cdap_rib::res_info_t &res);

	void remoteDeleteResult(const rina::cdap_rib::con_handle_t &con,
				const rina::cdap_rib::obj_info_t &obj,
				const rina::cdap_rib::res_info_t &res);

	// Operations to manage transactions
	unsigned int get_next_tid();
	int add_transaction_state(TransactionState* t);
	int remove_transaction_state(const std::string& tid);
	TransactionState * get_transaction_state(const std::string& tid);

	// Operations to process console commands
	std::string query_manager_rib(void);
	void list_systems(std::ostream& os);
	netman_res_t create_ipcp(CreateIPCPPromise * promise, int system_id,
				 const std::string& ipcp_desc);
	netman_res_t destroy_ipcp(Promise * promise, int system_id, int ipcp_id);
	netman_res_t create_dif(std::map<std::string, int>& result,
				const std::string& dif_desc);
	netman_res_t destroy_dif(std::map<int, int>& result,
				 const std::string& dif_name);

private:
        void n1_flow_accepted(const char * incoming_apn, int fd);
        int assign_system_id(void);
        netman_res_t create_ipcp(CreateIPCPPromise * promise, ManagedSystem * mas,
        			 rinad::configs::ipcp_config_t& ipcp_config);

        IPCPDescriptor * ipcp_desc_from_config(const std::string system_name,
        				       const rinad::configs::ipcp_config_t& ipcp_config);

        int cfd;
        std::string complete_name;

        NMEnrollmentTask * et;
        NMRIBDaemon * rd;
        NMConsole * console;
        DIFTemplateManager * dtm;

        rina::Timer timer;

	/// Readers of N-1 flows
	rina::Lockable lock;
	std::map<int, SDUReader *> sdu_readers;

	// Variables for handling transactions
	rina::ReadWriteLockable trans_rwlock;
	std::map<std::string, TransactionState*> pend_transactions;
	ConsecutiveUnsignedIntegerGenerator tid_gen;
};

struct RIBObjectClasses {
	enum class_name_code {
		CL_DAF, CL_PROCESSING_SYSTEM, CL_SOFTWARE, CL_HARDWARE,
		CL_KERNEL_AP, CL_OS_AP, CL_IPCPS, CL_MGMT_AGENTS, CL_UNKNOWN,
		CL_COMPUTING_SYSTEM, CL_IPCP
	};

	static const std::string DAF;
	static const std::string COMPUTING_SYSTEM;
	static const std::string PROCESSING_SYSTEM;
	static const std::string SOFTWARE;
	static const std::string HARDWARE;
	static const std::string KERNEL_AP;
	static const std::string OS_AP;
	static const std::string IPCPS;
	static const std::string MGMT_AGENTS;
	static const std::string IPCP;

	static class_name_code hash_it(const std::string& class_name);
};

#endif//NET_MANAGER_HPP

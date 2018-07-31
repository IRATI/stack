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

class NetworkManager;

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
	SDUReader(rina::ThreadAttributes * threadAttributes, int port_id, int fd_,
		  NetworkManager * nm);
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

	NetworkManager * netman;
	int fildesc;
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
		       const std::string& console_path);
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

	// Operations to process console commands
	std::string query_manager_rib(void);
	std::string list_systems(void);

private:
        void n1_flow_accepted(const char * incoming_apn, int fd);
        int assign_system_id(void);

        int cfd;
        std::string complete_name;

        NMEnrollmentTask * et;
        NMRIBDaemon * rd;
        NMConsole * console;

        rina::Timer timer;

	/// Readers of N-1 flows
	rina::Lockable lock;
	std::map<int, SDUReader *> sdu_readers;
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

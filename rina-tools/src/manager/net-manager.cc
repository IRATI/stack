//
// Network Manager (single standalone network manager to demo / debut / test
// interaction with Management Agents)
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

#define RINA_PREFIX "net-manager"
#include <librina/logs.h>
#include <librina/timer.h>
#include <rinad/common/configuration.h>
#include <rinad/common/encoder.h>
#include "net-manager.h"

#include <rina/api.h>

//TODO Add cache with managed system names - system id

//Class NMConsole
class CreateDIFsConsoleCmd: public rina::ConsoleCmdInfo {
public:
	CreateDIFsConsoleCmd(NMConsole * console, NetworkManager * nm) :
		rina::ConsoleCmdInfo("USAGE: create-difs <path to DIF descriptor folder> ddesc1 ddesc2 ...", console) {
		netman = nm;
	};

	int execute(std::vector<std::string>& args) {
		std::map<std::string, int> ipcp_ids;
		std::map<std::string, int>::iterator it;
		int start_time;
		int end_time;
		std::stringstream ss;

		if (args.size() < 3) {
			console->outstream << console->commands_map[args[0]]->usage << std::endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		for (unsigned int i = 2; i < args.size(); i++) {
			ss << args[1] << args[i];

			start_time = rina::Time::get_time_in_ms();
			if(netman->create_dif(ipcp_ids, ss.str()) == NETMAN_FAILURE){
				console->outstream << "Error while creating IPC process" << std::endl;
				return rina::UNIXConsole::CMDRETCONT;
			}
			end_time = rina::Time::get_time_in_ms();

			console->outstream << "DIF described in " << args[i]
			                   << " created in " << end_time - start_time << " ms. ";
			console->outstream << "IPCPs created at the following systems: " << std::endl;
			for (it = ipcp_ids.begin(); it != ipcp_ids.end(); ++it) {
				console->outstream << "System " << it->first
						<< ", IPCP id " << it->second << std::endl;
			}

			ipcp_ids.clear();
			ss.str(std::string());
		}

		return rina::UNIXConsole::CMDRETCONT;
	}

private:
	NetworkManager * netman;
};

//Class NMConsole
class CreateDIFConsoleCmd: public rina::ConsoleCmdInfo {
public:
	CreateDIFConsoleCmd(NMConsole * console, NetworkManager * nm) :
		rina::ConsoleCmdInfo("USAGE: create-dif <path to DIF descriptor file>", console) {
		netman = nm;
	};

	int execute(std::vector<std::string>& args) {
		std::map<std::string, int> ipcp_ids;
		std::map<std::string, int>::iterator it;
		int start_time;
		int end_time;

		if (args.size() < 2) {
			console->outstream << console->commands_map[args[0]]->usage << std::endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		start_time = rina::Time::get_time_in_ms();
		if(netman->create_dif(ipcp_ids, args[1]) == NETMAN_FAILURE){
			console->outstream << "Error while creating IPC process" << std::endl;
			return rina::UNIXConsole::CMDRETCONT;
		}
		end_time = rina::Time::get_time_in_ms();

		console->outstream << "DIF created in " << end_time - start_time << " ms. ";
		console->outstream << "IPCPs created at the following systems: " << std::endl;
		for (it = ipcp_ids.begin(); it != ipcp_ids.end(); ++it) {
			console->outstream << "System " << it->first
					   << ", IPCP id " << it->second << std::endl;
		}

		return rina::UNIXConsole::CMDRETCONT;
	}

private:
	NetworkManager * netman;
};

class CreateIPCPConsoleCmd: public rina::ConsoleCmdInfo {
public:
	CreateIPCPConsoleCmd(NMConsole * console, NetworkManager * nm) :
		rina::ConsoleCmdInfo("USAGE: create-ipcp <system-id> <path to IPCP descriptor file>", console) {
		netman = nm;
	};

	int execute(std::vector<std::string>& args) {
		CreateIPCPPromise promise;
		int system_id;

		if (args.size() < 3) {
			console->outstream << console->commands_map[args[0]]->usage << std::endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		if(rina::string2int(args[1], system_id)){
			console->outstream << "Invalid system id" << std::endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		if(netman->create_ipcp(&promise, system_id, args[2]) == NETMAN_FAILURE ||
				promise.wait() != NETMAN_SUCCESS){
			console->outstream << "Error while creating IPC process" << std::endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		console->outstream << "IPC process created successfully [ipcp_id = "
		      << promise.ipcp_id << ", system_id="<<args[1]<<"]" << std::endl;
		return rina::UNIXConsole::CMDRETCONT;
	}

private:
	NetworkManager * netman;
};

class DestroyIPCPConsoleCmd: public rina::ConsoleCmdInfo {
public:
	DestroyIPCPConsoleCmd(NMConsole * console, NetworkManager * nm) :
		rina::ConsoleCmdInfo("USAGE: destroy-ipcp <system-id> <ipcp-id>", console) {
		netman = nm;
	};

	int execute(std::vector<std::string>& args) {
		Promise promise;
		int system_id, ipcp_id;

		if (args.size() < 3) {
			console->outstream << console->commands_map[args[0]]->usage << std::endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		if(rina::string2int(args[1], system_id)){
			console->outstream << "Invalid system id" << std::endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		if(rina::string2int(args[2], ipcp_id)){
			console->outstream << "Invalid IPCP id" << std::endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		if(netman->destroy_ipcp(&promise, system_id, ipcp_id) == NETMAN_FAILURE ||
				promise.wait() != NETMAN_SUCCESS){
			console->outstream << "Error while destroying IPC process" << std::endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		console->outstream << "IPC process destroyed successfully [id = "
		      << ipcp_id <<"]" << std::endl;
		return rina::UNIXConsole::CMDRETCONT;
	}

private:
	NetworkManager * netman;
};

class DestroyDIFConsoleCmd: public rina::ConsoleCmdInfo {
public:
	DestroyDIFConsoleCmd(NMConsole * console, NetworkManager * nm) :
		rina::ConsoleCmdInfo("USAGE: destroy-dif <dif-name>", console) {
		netman = nm;
	};

	int execute(std::vector<std::string>& args) {
		std::map<int, int> ipcp_ids;
		std::map<int, int>::iterator it;
		int start_time;
		int end_time;

		if (args.size() < 2) {
			console->outstream << console->commands_map[args[0]]->usage << std::endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		start_time = rina::Time::get_time_in_ms();
		if(netman->destroy_dif(ipcp_ids, args[1]) == NETMAN_FAILURE){
			console->outstream << "Error while destroying DIF" << std::endl;
			return rina::UNIXConsole::CMDRETCONT;
		}
		end_time = rina::Time::get_time_in_ms();

		console->outstream << "DIF " << args[1] << " destroyed in " << end_time - start_time;
		console->outstream << " ms. IPCPs destroyed at the following systems: " << std::endl;
		for (it = ipcp_ids.begin(); it != ipcp_ids.end(); ++it) {
			console->outstream << "System " << it->first
					   << ", IPCP id " << it->second << std::endl;
		}

		return rina::UNIXConsole::CMDRETCONT;
	}

private:
	NetworkManager * netman;
};

class DestroyDIFsConsoleCmd: public rina::ConsoleCmdInfo {
public:
	DestroyDIFsConsoleCmd(NMConsole * console, NetworkManager * nm) :
		rina::ConsoleCmdInfo("USAGE: destroy-difs <dif-name> <dif_name> ...", console) {
		netman = nm;
	};

	int execute(std::vector<std::string>& args) {
		std::map<int, int> ipcp_ids;
		std::map<int, int>::iterator it;
		int start_time;
		int end_time;

		if (args.size() < 2) {
			console->outstream << console->commands_map[args[0]]->usage << std::endl;
			return rina::UNIXConsole::CMDRETCONT;
		}

		for (unsigned int i = 1; i < args.size(); i++) {
			start_time = rina::Time::get_time_in_ms();
			if(netman->destroy_dif(ipcp_ids, args[i]) == NETMAN_FAILURE){
				console->outstream << "Error while destroying DIF" << std::endl;
				return rina::UNIXConsole::CMDRETCONT;
			}
			end_time = rina::Time::get_time_in_ms();

			console->outstream << "DIF " << args[i] << " destroyed in " << end_time - start_time;
			console->outstream << " ms. IPCPs destroyed at the following systems: " << std::endl;
			for (it = ipcp_ids.begin(); it != ipcp_ids.end(); ++it) {
				console->outstream << "System " << it->first
						<< ", IPCP id " << it->second << std::endl;
			}

			ipcp_ids.clear();
		}

		return rina::UNIXConsole::CMDRETCONT;
	}

private:
	NetworkManager * netman;
};

class ListSystemsConsoleCmd: public rina::ConsoleCmdInfo {
public:
	ListSystemsConsoleCmd(NMConsole * console, NetworkManager * nm) :
		rina::ConsoleCmdInfo("USAGE: list-systems", console) {
		netman = nm;
	};

	int execute(std::vector<std::string>& args) {
		netman->list_systems(console->outstream);
		return rina::UNIXConsole::CMDRETCONT;
	}

private:
	NetworkManager * netman;
};

class QueryRIBConsoleCmd: public rina::ConsoleCmdInfo {
public:
	QueryRIBConsoleCmd(NMConsole * console, NetworkManager * nm) :
		rina::ConsoleCmdInfo("USAGE: query-rib", console) {
		netman = nm;
	};

	int execute(std::vector<std::string>& args) {
		console->outstream << netman->query_manager_rib() << std::endl;
		return rina::UNIXConsole::CMDRETCONT;
	}

private:
	NetworkManager * netman;
};


NMConsole::NMConsole(const std::string& socket_path, NetworkManager * nm) :
			rina::UNIXConsole(socket_path, "Mgr")
{
	netman = nm;
	commands_map["create-dif"] = new CreateDIFConsoleCmd(this, netman);
	commands_map["create-difs"] = new CreateDIFsConsoleCmd(this, netman);
	commands_map["create-ipcp"] = new CreateIPCPConsoleCmd(this, netman);
	commands_map["destroy-dif"] = new DestroyDIFConsoleCmd(this, netman);
	commands_map["destroy-difs"] = new DestroyDIFsConsoleCmd(this, netman);
	commands_map["destroy-ipcp"] = new DestroyIPCPConsoleCmd(this, netman);
	commands_map["list-systems"] = new ListSystemsConsoleCmd(this, netman);
	commands_map["query-rib"] = new QueryRIBConsoleCmd(this, netman);
}

//Class SDUReader
SDUReader::SDUReader(int port_id, int fd_, NetworkManager * nm)
	: SimpleThread(std::string("sdu-reader"), false)
{
	portid = port_id;
	fd = fd_;
	netman = nm;
}

int SDUReader::run()
{
	rina::ser_obj_t message;
	message.message_ = new unsigned char[10000];
	int bytes_read = 0;

	LOG_DBG("SDU reader of port-id %d starting", portid);

	while(true) {
		LOG_DBG("Going to read from file descriptor %d", fd);
		bytes_read = read(fd, message.message_, 10000);
		if (bytes_read <= 0) {
			LOG_ERR("Read error or EOF: %d", bytes_read);
			break;
		}

		LOG_DBG("Read %d bytes", bytes_read);
		message.size_ = bytes_read;

		//Instruct CDAP provider to process the CACEP message
		try{
			rina::cdap::getProvider()->process_message(message,
								   portid);
		} catch(rina::Exception &e){
			LOG_ERR("Problems processing message from port-id %d",
				portid);
		}
	}

	LOG_DBG("SDU Reader of port-id %d terminating", portid);
	netman->disconnect_from_system_async(fd);

	return 0;
}

//Struct ManagedSystem
ManagedSystem::ManagedSystem() {
	auth_ps_ = NULL;
	system_id = 0;
	status = NM_PENDING;
}

ManagedSystem::~ManagedSystem()
{
	std::map<std::string, rina::rib::RIBObj*>::iterator it;
	std::map<int, IPCPDescriptor*>::iterator cit;

	for (it = objs_to_create.begin(); it != objs_to_create.end() /* not hoisted */; /* no increment */) {
		delete it->second;
	        objs_to_create.erase(it++);
	}

	for (cit = ipcps.begin(); cit != ipcps.end() /* not hoisted */; /* no increment */) {
		delete cit->second;
	        ipcps.erase(cit++);
	}
}

//Class NMEnrollmentTask
NMEnrollmentTask::NMEnrollmentTask() :
		ApplicationEntity(ApplicationEntity::ENROLLMENT_TASK_AE_NAME)
{
	nm = NULL;
}

NMEnrollmentTask::~NMEnrollmentTask()
{
	std::map<std::string, ManagedSystem *>::iterator itr;
	ManagedSystem * mgdsys;

	itr = enrolled_systems.begin();
	while (itr != enrolled_systems.end()) {
		mgdsys = itr->second;
		enrolled_systems.erase(itr++);
		delete mgdsys;
	}
}

void NMEnrollmentTask::set_application_process(rina::ApplicationProcess * ap)
{
	if (!ap) {
		LOG_ERR("Bogus app passed");
		return;
	}

	nm = static_cast<NetworkManager *>(ap);
}

void NMEnrollmentTask::process_authentication_message(const rina::cdap::CDAPMessage& message,
						      const rina::cdap_rib::con_handle_t &con)
{
	//Ignore for now
}

void NMEnrollmentTask::release(int invoke_id, const rina::cdap_rib::con_handle_t &con)
{
	//Ignore for now
}

void NMEnrollmentTask::releaseResult(const rina::cdap_rib::res_info_t &res,
				     const rina::cdap_rib::con_handle_t &con)
{
	//Ignore for now
}

void NMEnrollmentTask::initiateEnrollment(const rina::ApplicationProcessNamingInformation& peer_name,
					   const std::string& dif_name, int port_id)
{
	//Do nothing, we let the Management Agent initiate enrollment
}

void NMEnrollmentTask::connect(const rina::cdap::CDAPMessage& message,
			       rina::cdap_rib::con_handle_t &con)
{
	std::map<std::string, ManagedSystem *>::iterator it;
	ManagedSystem * mgd_system = 0;
	rina::cdap_rib::res_info_t res;

	//1 Find out if the sender is really connecting to us
	if(con.src_.ap_name_ != nm->get_name()){
		LOG_ERR("an M_CONNECT message whose destination was not this DA instance, ignoring it");
		nm->disconnect_from_system(con.port_id);
		return;
	}

	//2 Check we are not enrolled yet
	lock.lock();
	it = enrolled_systems.find(con.dest_.ap_name_);
	if (it != enrolled_systems.end()) {
		lock.unlock();
		LOG_ERR("I am already enrolled to DIF Allocator instance %s",
			con.dest_.ap_name_.c_str());
		nm->disconnect_from_system(con.port_id);
		return;
	}

	//3 TODO authenticate
	LOG_INFO("Authenticating Management Agent %s-%s ...",
		  con.dest_.ap_name_.c_str(),
		  con.dest_.ap_inst_.c_str());

	LOG_INFO("Authentication successful!");

	//4 Send M_CONNECT_R
	try{
		res.code_ = rina::cdap_rib::CDAP_SUCCESS;
		rina::cdap::getProvider()->send_open_connection_result(con,
								       res,
								       con.auth_,
								       message.invoke_id_);
	}catch(rina::Exception &e){
		lock.unlock();
		LOG_ERR("Problems sending CDAP message: %s", e.what());
		nm->disconnect_from_system(con.port_id);
		return;
	}

	mgd_system = new ManagedSystem();
	mgd_system->con = con;
	mgd_system->system_id = 0;
	mgd_system->status = NM_SUCCESS;
	enrolled_systems[con.dest_.ap_name_] = mgd_system;

	lock.unlock();

	nm->enrollment_completed(mgd_system);
}

void NMEnrollmentTask::connectResult(const rina::cdap::CDAPMessage& message,
		   	   	     rina::cdap_rib::con_handle_t &con)
{
	//Not implemented, connectResult must be handled by the MA right now
}

//Class NM RIB Daemon
NMRIBDaemon::NMRIBDaemon(rina::cacep::AppConHandlerInterface *app_con_callback)
{
	rina::cdap_rib::cdap_params params;
	rina::cdap_rib::vers_info_t vers;

	//Initialize the RIB library and cdap
	params.ipcp = false;
	rina::rib::init(app_con_callback, params);
	ribd = rina::rib::RIBDaemonProxyFactory();

	//Create schema
	vers.version_ = 0x1ULL;
	ribd->createSchema(vers);

	//Create RIB
	rib = ribd->createRIB(vers);
	ribd->associateRIBtoAE(rib, "");

	LOG_INFO("RIB Daemon Initialized");
}

NMRIBDaemon::~NMRIBDaemon()
{
	rina::rib::fini();
}

rina::rib::RIBDaemonProxy * NMRIBDaemon::getProxy()
{
	if (ribd)
		return ribd;

	return NULL;
}

const rina::rib::rib_handle_t & NMRIBDaemon::get_rib_handle()
{
	return rib;
}

int64_t NMRIBDaemon::addObjRIB(const std::string& fqn,
			       rina::rib::RIBObj** obj)
{
	return ribd->addObjRIB(rib, fqn, obj);
}

void NMRIBDaemon::removeObjRIB(const std::string& fqn, bool force)
{
	ribd->removeObjRIB(rib, fqn, force);
}

std::list<rina::rib::RIBObjectData> NMRIBDaemon::get_rib_objects_data(void)
{
	return ribd->get_rib_objects_data(rib);
}

void DisconnectFromSystemTimerTask::run()
{
	netman->disconnect_from_system(fildesc);
}

//Class NetworkManager
NetworkManager::NetworkManager(const std::string& app_name,
			       const std::string& app_instance,
			       const std::string& console_path,
			       const std::string& dif_templates_path) :
			       	       rina::ApplicationProcess(app_name, app_instance)
{
	std::stringstream ss;
	rina::rib::RIBObj * tmp;

	cfd = 0;
	ss << app_name << "|" << app_instance << "||";
	complete_name = ss.str();

	console = new NMConsole(console_path, this);

	et = new NMEnrollmentTask();
	et->set_application_process(this);

	rd = new NMRIBDaemon(et);
	rd->set_application_process(this);

	// Initialize DIF Templates Manager (with its monitor thread)
	dtm = new DIFTemplateManager(dif_templates_path);

	//Add required RIB objects to RIB
	try {
		tmp = new rina::rib::RIBObj("ManagedSystems");
		rd->addObjRIB("/systems", &tmp);
	} catch (rina::Exception &e1) {
		LOG_ERR("RIB basic objects were not created because %s",
			e1.what());
	}
}

NetworkManager::~NetworkManager()
{
	void * status;
	std::map<int, SDUReader *>::iterator itr;
	std::map<std::string, TransactionState*>::iterator t_itr;
	SDUReader * reader;

	if (console)
		delete console;

	if (et)
		delete et;

	if (rd)
		delete rd;

	if (dtm)
		delete dtm;

	for (t_itr = pend_transactions.begin(); t_itr != pend_transactions.end(); ++t_itr) {
		delete t_itr->second;
	}

	itr = sdu_readers.begin();
	while (itr != sdu_readers.end()) {
		close(itr->first);
		reader = itr->second;
		reader->join(&status);
		sdu_readers.erase(itr++);
		delete reader;
	}

	close(cfd);
}

unsigned int NetworkManager::get_address() const
{
	return 0;
}

void NetworkManager::event_loop(std::list<std::string>& dif_names)
{
	std::list<std::string>::iterator it;
	struct rina_flow_spec fspec;
	char *incomingapn = NULL;
	int nfd = 0;

	// 1 Open RINA descriptor
	cfd = rina_open();
	if (cfd < 0) {
		LOG_DBG("rina_open() failed");
		return;
	}

	// 2 Register to one or more DIFs
	for (it = dif_names.begin(); it != dif_names.end(); ++it) {
		if (rina_register(cfd, it->c_str(), complete_name.c_str(), 0) < 0) {
			LOG_WARN("Failed to register to DIF %s", it->c_str());
		} else {
			LOG_INFO("Registered to DIF %s", it->c_str());
		}
	}

	if (dif_names.size() == 0) {
		if (rina_register(cfd, NULL, complete_name.c_str(), 0) < 0) {
			LOG_WARN("Failed to register at DIF %s", it->c_str());
		} else {
			LOG_INFO("Registered at DIF");
		}
	}

	// Main event loop
	for (;;) {
		// Accept new flow
		fspec.version = RINA_FLOW_SPEC_VERSION;
		nfd = rina_flow_accept(cfd, &incomingapn, &fspec, 0);
		if (nfd < 0)
			break;

		LOG_DBG("Accepted flow from remote application %s, fd = %d",
			incomingapn, nfd);

		//Give the flow handle to a new worker, which will proceed with enrollment
		n1_flow_accepted(incomingapn, nfd);
		free(incomingapn);
	}
}

void NetworkManager::n1_flow_accepted(const char * incoming_apn, int fd)
{
	SDUReader * reader = 0;

	rina::ScopedLock g(lock);

	// Use fd as port-id
	reader = new SDUReader(fd, fd, this);
	reader->start();

	sdu_readers[fd] = reader;

	rina::cdap::add_fd_to_port_id_mapping(fd, fd);
}

int NetworkManager::assign_system_id(void)
{
	rina::ScopedLock(et->lock);
	int candidate = 1;
	std::map<std::string, ManagedSystem *>::iterator it;
	bool assigned;

	for (candidate = 1; candidate < 2000; candidate ++) {
		assigned = false;
		for (it = et->enrolled_systems.begin();
				it != et->enrolled_systems.end(); ++it) {
			if (it->second->system_id == candidate) {
				assigned = true;
				break;
			}
		}

		if (!assigned)
			break;
	}

	return candidate;
}

void NetworkManager::disconnect_from_system_async(int fd)
{
	DisconnectFromSystemTimerTask * ttask;

	ttask = new DisconnectFromSystemTimerTask(this, fd);
	timer.scheduleTask(ttask, 0);
}

void NetworkManager::disconnect_from_system(int fd)
{
	void * status;
	std::map<int, SDUReader *>::iterator itr;
	std::map<std::string, ManagedSystem *>::iterator it;
	std::stringstream ss;
	ManagedSystem * ms;
	SDUReader * reader;

	// Remove Managed System
	ms = NULL;
	et->lock.lock();
	for (it = et->enrolled_systems.begin();
			it != et->enrolled_systems.end(); ++it) {
		if (it->second->con.port_id == (unsigned int) fd) {
			ms = it->second;
			et->enrolled_systems.erase(it);
			break;
		}
	}
	et->lock.unlock();

	// Remove objects from RIB
	if (ms) {
		try {
			ss << "/systems/msid=" << ms->system_id;
			rd->removeObjRIB(ss.str(), true);
		} catch (rina::Exception &e1) {
			LOG_ERR("RIB basic objects were not created because %s",
					e1.what());
		}

		delete ms;
	}

	// Remove SDU Reader
	rina::ScopedLock g(lock);

	itr = sdu_readers.find(fd);
	if (itr == sdu_readers.end())
		return;

	close(itr->first);
	reader = itr->second;
	reader->join(&status);
	sdu_readers.erase(itr);

	delete reader;
}

void NetworkManager::enrollment_completed(struct ManagedSystem * system)
{
	rina::cdap_rib::res_info_t res;
	rina::cdap_rib::obj_info_t obj_info;
        rina::cdap_rib::flags_t flags;
        rina::cdap_rib::filt_info_t filt;
        rina::rib::RIBObj * csobj;
        std::stringstream ss;

	LOG_DBG("Enrollment to Management Agent %s %s completed",
			system->con.dest_.ap_name_.c_str(),
			system->con.dest_.ap_inst_.c_str());

	system->system_id = assign_system_id();

	//Add RIB objects for the managed system
	try {
		ss << "/systems/msid=" << system->system_id;
		csobj = new rina::rib::RIBObj("ComputingSystem");
		rd->addObjRIB(ss.str(), &csobj);
	} catch (rina::Exception &e1) {
		LOG_ERR("RIB basic objects were not created because %s",
			e1.what());
	}

	//2 query MA and get relevant system information
	obj_info.class_ = RIB_ROOT_CN;
	obj_info.name_ = "/";
	filt.scope_ = 100;
	try{
		rd->getProxy()->remote_read(system->con, obj_info, flags, filt, this);
	}catch(rina::Exception &e){
		lock.unlock();
		LOG_ERR("Problems sending CDAP message: %s", e.what());
		disconnect_from_system(system->con.port_id);
		return;
	}

}

void NetworkManager::remoteReadResult(const rina::cdap_rib::con_handle_t &con,
		      	      	      const rina::cdap_rib::obj_info_t &obj,
				      const rina::cdap_rib::res_info_t &res,
				      const rina::cdap_rib::flags_t & flags)
{
	rina::rib::RIBObj * rib_obj;
	std::map<std::string, ManagedSystem *>::iterator it;
	ManagedSystem * system;
	std::map<std::string, rina::rib::RIBObj*>::iterator rit;
	std::stringstream ss;

	LOG_INFO("Got read result. Class: %s Name: %s",
		  obj.class_.c_str(), obj.name_.c_str());

	rina::ScopedLock (et->lock);

	system = NULL;
	for (it = et->enrolled_systems.begin();
			it != et->enrolled_systems.end(); ++it) {
		if (it->second->con.port_id == con.port_id) {
			system = it->second;
			break;
		}
	}

	if (!system) {
		LOG_WARN("Got read response for an unknown port-id: %u",
			 con.port_id);
		return;
	}

	switch(RIBObjectClasses::hash_it(obj.class_)) {
	case RIBObjectClasses::CL_DAF :
	case RIBObjectClasses::CL_COMPUTING_SYSTEM :
	case RIBObjectClasses::CL_PROCESSING_SYSTEM :
	case RIBObjectClasses::CL_SOFTWARE :
	case RIBObjectClasses::CL_HARDWARE :
	case RIBObjectClasses::CL_KERNEL_AP :
	case RIBObjectClasses::CL_OS_AP :
	case RIBObjectClasses::CL_IPCPS :
	case RIBObjectClasses::CL_MGMT_AGENTS :
	case RIBObjectClasses::CL_IPCP :
		rib_obj = new rina::rib::RIBObj(obj.class_);
		ss << "/systems/msid=" << system->system_id << obj.name_;
		system->objs_to_create[ss.str()] = rib_obj;
		break;
	case RIBObjectClasses::CL_UNKNOWN :
		//Ignore for now
		break;
	default:
		LOG_ERR("Ignoring object class");
	}

	if (flags.flags_ == rina::cdap_rib::flags::F_RD_INCOMPLETE)
		return;

	for (rit = system->objs_to_create.begin();
			rit != system->objs_to_create.end(); ++rit) {
		rd->addObjRIB(rit->first, &rit->second);
	}

	system->objs_to_create.clear();
}

std::string NetworkManager::query_manager_rib()
{
	std::stringstream ss;
	std::list<rina::rib::RIBObjectData> objects;
	std::list<rina::rib::RIBObjectData>::iterator it;

	objects = rd->get_rib_objects_data();
	for (it = objects.begin(); it != objects.end(); ++it) {
		ss << "Name: " << it->name_ <<
			"; Class: "<< it->class_;
		ss << "; Instance: "<< it->instance_ << std::endl;
		ss << "Value: " << it->displayable_value_ << std::endl;
		ss << "" << std::endl;
	}

	return ss.str();
}

void NetworkManager::list_systems(std::ostream& os)
{
	std::map<std::string, ManagedSystem *>::iterator it;

	os << "Current Managed Systems (system id | MA name | port-id)" << std::endl;

	rina::ScopedLock(et->lock);

	for (it = et->enrolled_systems.begin();
			it != et->enrolled_systems.end(); ++it) {
		os << "       " << it->second->system_id << "   |   ";
		os << it->second->con.dest_.ap_name_ << "-"
		   << it->second->con.dest_.ap_inst_ << "   |   ";
		os << "       " << it->second->con.port_id << std::endl;
	}
}

netman_res_t NetworkManager::create_ipcp(CreateIPCPPromise * promise, int system_id,
			 	 	 const std::string& ipcp_desc)
{
	std::map<std::string, ManagedSystem *>::iterator it;
	ManagedSystem * mas;
	rinad::configs::ipcp_config_t ipcp_config;

	//1 Retrieve system from system-id, if it doesn't exist, return error
	mas = NULL;
	et->lock.lock();
	for (it = et->enrolled_systems.begin();
			it != et->enrolled_systems.end(); ++it) {
		if (it->second->system_id == system_id) {
			mas = it->second;
			break;
		}
	}
	et->lock.unlock();

	if (!mas) {
		LOG_ERR("Could not find Managed System with id %d", system_id);
		return NETMAN_FAILURE;
	}

	//2 Read and parse IPCP descriptor
	if (dtm->get_ipcp_config_from_desc(ipcp_config, mas->con.dest_.ap_name_, ipcp_desc)) {
		LOG_ERR("Problems getting IPCP configuration from IPCP descriptor");
		return NETMAN_FAILURE;
	}

	return create_ipcp(promise, mas, ipcp_config);
}

IPCPDescriptor * NetworkManager::ipcp_desc_from_config(const std::string system_name,
		       	       	       	       	       const rinad::configs::ipcp_config_t& ipcp_config)
{
	IPCPDescriptor * result;

	result = new IPCPDescriptor();
	result->system_name = system_name;
	result->dif_name = ipcp_config.dif_to_assign.dif_name_.processName;
	result->dif_template_name = ipcp_config.dif_template;

	return result;
}

netman_res_t NetworkManager::create_ipcp(CreateIPCPPromise * promise,
					 ManagedSystem * mas,
					 rinad::configs::ipcp_config_t& ipcp_config)
{
	MASystemTransState * trans;
	rina::cdap_rib::obj_info_t obj_info;
        rina::cdap_rib::flags_t flags;
        rina::cdap_rib::filt_info_t filt;
        rinad::encoders::IPCPConfigEncoder encoder;
        std::stringstream ss;
        IPCPDescriptor * ipcp_desc;

	obj_info.name_ = "/csid=1/psid=1/kernelap/osap/ipcps/ipcpid";
	obj_info.class_ = "IPCProcess";
	encoder.encode(ipcp_config, obj_info.value_);

	ipcp_desc = ipcp_desc_from_config(mas->con.src_.ap_name_, ipcp_config);

	ss << mas->con.port_id << "-" << obj_info.name_;
	//3 Store transaction
	trans = new MASystemTransState(this, promise, mas->system_id,
				       ipcp_desc, ss.str());
	if (!trans) {
		LOG_ERR("Unable to allocate memory for the transaction object. Out of memory!");
		return NETMAN_FAILURE;
	}

	if (add_transaction_state(trans) < 0)
	{
		LOG_ERR("Unable to add transaction; repeated transaction?");
		remove_transaction_state(trans->tid);
		return NETMAN_FAILURE;
	}

	//4 Send CDAP message
	try{
		rd->getProxy()->remote_create(mas->con, obj_info, flags, filt, this);
	}catch(rina::Exception &e){
		LOG_ERR("Problems sending CDAP message: %s", e.what());
		remove_transaction_state(trans->tid);
		return NETMAN_FAILURE;
	}

	return NETMAN_PENDING;
}

void NetworkManager::remoteCreateResult(const rina::cdap_rib::con_handle_t &con,
					const rina::cdap_rib::obj_info_t &obj,
					const rina::cdap_rib::res_info_t &res)
{
	std::stringstream ss;
	MASystemTransState * trans;
	CreateIPCPPromise * promise;
	int ipcp_id;
	rina::rib::RIBObj * rib_obj;
	ManagedSystem * mas;
	std::map<std::string, ManagedSystem *>::iterator it;

	ss << con.port_id << "-" << obj.name_.substr(0, obj.name_.find("ipcpid") + 6);

	LOG_INFO("Received object reply for object name %s", obj.name_.c_str());

	// Retrieve transaction
	trans = dynamic_cast<MASystemTransState*>(get_transaction_state(ss.str()));
	if(!trans){
		LOG_ERR("Could not retrieve transaction with id %s", ss.str().c_str());
		return;
	}

	promise = dynamic_cast<CreateIPCPPromise*>(trans->promise);
	if (!promise) {
		LOG_ERR("Could not retrieve promise from transaction");
	}

	ipcp_id = 0;
	rina::string2int(obj.name_.substr(obj.name_.find("ipcpid") + 7), ipcp_id);

	promise->ipcp_id = ipcp_id;

	// Mark transaction as completed
	if (res.code_ == rina::cdap_rib::CDAP_SUCCESS) {
		mas = NULL;
		et->lock.lock();
		for (it = et->enrolled_systems.begin();
				it != et->enrolled_systems.end(); ++it) {
			if (it->second->system_id == trans->system_id) {
				mas = it->second;
				break;
			}
		}
		et->lock.unlock();

		if (!mas) {
			LOG_WARN("Could not find Managed System with id %d", trans->system_id);
			if (trans->ipcp_desc)
				delete trans->ipcp_desc;
		} else if (trans->ipcp_desc){
			mas->ipcps[ipcp_id] = trans->ipcp_desc;
		}

		ss.str(std::string());
		rib_obj = new rina::rib::RIBObj(obj.class_);
		ss << "/systems/msid=" << trans->system_id
		   << "/csid=1/psid=1/kernelap/osap/ipcps/ipcpid="
		   << promise->ipcp_id;
		rd->addObjRIB(ss.str(), &rib_obj);

		trans->completed(NETMAN_SUCCESS);
	} else {
		trans->completed(NETMAN_FAILURE);
	}
	remove_transaction_state(trans->tid);
}

netman_res_t NetworkManager::create_dif(std::map<std::string, int>& result,
					const std::string& dif_desc)
{
	DIFDescriptor ddesc;
	std::list<IPCPDescriptor>::iterator it;
	std::map<std::string, ManagedSystem *>::iterator mit;
	ManagedSystem * mas;

	if (dtm->parse_dif_descriptor(dif_desc, ddesc)) {
		LOG_ERR("Problems parsing DIF descriptor");
		return NETMAN_FAILURE;
	}

	for ( it = ddesc.ipcps.begin(); it != ddesc.ipcps.end(); ++it) {
		rinad::configs::ipcp_config_t ipcp_config;
		CreateIPCPPromise promise;

		//1 Retrieve system from system-name, if it doesn't exist, continue
		mas = NULL;
		et->lock.lock();
		for (mit = et->enrolled_systems.begin();
				mit != et->enrolled_systems.end(); ++mit) {
			if (mit->second->con.dest_.ap_name_ == it->system_name) {
				mas = mit->second;
				break;
			}
		}
		et->lock.unlock();

		if (!mas) {
			LOG_ERR("Could not find Managed System with name %s",
				it->system_name.c_str());
			result[it->system_name] = -1;
			continue;
		}

		//2 Get IPCP configuration from IPCP descriptor
		if (dtm->__get_ipcp_config_from_desc(ipcp_config,
						     it->system_name, *it)) {
			LOG_ERR("Error getting IPCP configuration");
			result[it->system_name] = -1;
			continue;
		}

		//3 Create IPCP process and wait for result
		if(create_ipcp(&promise, mas, ipcp_config) == NETMAN_FAILURE ||
				promise.wait() != NETMAN_SUCCESS){
			LOG_ERR("Error while creating IPC process");
			result[it->system_name] = -1;
		} else {
			result[it->system_name] = promise.ipcp_id;
		}
	}

	return NETMAN_SUCCESS;
}

netman_res_t NetworkManager::destroy_dif(std::map<int, int>& result,
			 	 	 const std::string& dif_name)
{
	std::map<std::string, ManagedSystem *>::iterator it;
	std::map<int, IPCPDescriptor *>::iterator cit;
	std::map<int, int>::iterator ipcpit;
	ManagedSystem * mas;
	Promise promise;
	int ipcp_id;

	// 1 Get systems and IPCPs in DIFs
	mas = NULL;
	et->lock.lock();
	for (it = et->enrolled_systems.begin();
			it != et->enrolled_systems.end(); ++it) {
		mas = it->second;
		ipcp_id = 0;

		for (cit = mas->ipcps.begin(); cit != mas->ipcps.end(); ++cit) {
			if (cit->second->dif_name == dif_name) {
				ipcp_id = cit->first;
				break;
			}
		}

		if (ipcp_id != 0)
			result[mas->system_id] = ipcp_id;
	}
	et->lock.unlock();

	if (result.size() == 0) {
		LOG_ERR("Could not find any IPC Process belonging to DIF %s",
			dif_name.c_str());
		return NETMAN_FAILURE;
	}

	// 2 Request destruction of IPCPs
	for (ipcpit = result.begin(); ipcpit != result.end(); ipcpit++) {
		if(destroy_ipcp(&promise, ipcpit->first, ipcpit->second) == NETMAN_FAILURE ||
				promise.wait() != NETMAN_SUCCESS){
			LOG_WARN("Error destroying IPCP %d at system %d",
					ipcpit->second, ipcpit->first);
			result[ipcpit->first] = -1;
		}
	}

	return NETMAN_SUCCESS;
}

netman_res_t NetworkManager::destroy_ipcp(Promise * promise, int system_id, int ipcp_id)
{
	MASystemTransState * trans;
	std::map<std::string, ManagedSystem *>::iterator it;
	ManagedSystem * mas;
	rina::cdap_rib::res_info_t res;
	rina::cdap_rib::obj_info_t obj_info;
        rina::cdap_rib::flags_t flags;
        rina::cdap_rib::filt_info_t filt;
        std::stringstream ss;

	//1 Retrieve system from system-id, if it doesn't exist, return error
        mas = NULL;
	et->lock.lock();
	for (it = et->enrolled_systems.begin();
			it != et->enrolled_systems.end(); ++it) {
		if (it->second->system_id == system_id) {
			mas = it->second;
		}
	}
	et->lock.unlock();

	if (!mas) {
		LOG_ERR("Could not find Managed System with id %d", system_id);
		return NETMAN_FAILURE;
	}

	ss << "/csid=1/psid=1/kernelap/osap/ipcps/ipcpid=" << ipcp_id;
	obj_info.name_ = ss.str();
	obj_info.class_ = "IPCProcess";
	ss.str(std::string());

	ss << mas->con.port_id << "-" << obj_info.name_;
	//3 Store transaction
	trans = new MASystemTransState(this, promise, system_id, NULL, ss.str());
	if (!trans) {
		LOG_ERR("Unable to allocate memory for the transaction object. Out of memory!");
		return NETMAN_FAILURE;
	}

	if (add_transaction_state(trans) < 0)
	{
		LOG_ERR("Unable to add transaction; out of memory?");
		remove_transaction_state(trans->tid);
		return NETMAN_FAILURE;
	}

	//4 Send CDAP message
	try{
		rd->getProxy()->remote_delete(mas->con, obj_info, flags, filt, this);
	}catch(rina::Exception &e){
		LOG_ERR("Problems sending CDAP message: %s", e.what());
		remove_transaction_state(trans->tid);
		return NETMAN_FAILURE;
	}

	return NETMAN_PENDING;
}

void NetworkManager::remoteDeleteResult(const rina::cdap_rib::con_handle_t &con,
					const rina::cdap_rib::obj_info_t &obj,
					const rina::cdap_rib::res_info_t &res)
{
	std::stringstream ss;
	MASystemTransState * trans;
	ManagedSystem * mas;
	std::map<std::string, ManagedSystem *>::iterator it;
	std::map<int, IPCPDescriptor *>::iterator cit;
	int ipcp_id;

	ss << con.port_id << "-" << obj.name_;

	LOG_INFO("Received object reply for object name %s", obj.name_.c_str());

	// Retrieve transaction
	trans = dynamic_cast<MASystemTransState*>(get_transaction_state(ss.str()));
	if(!trans){
		LOG_ERR("Could not retrieve transaction with id %s", ss.str().c_str());
		return;
	}

	ipcp_id = 0;
	rina::string2int(obj.name_.substr(obj.name_.find("ipcpid") + 7), ipcp_id);

	// Mark transaction as completed
	if (res.code_ == rina::cdap_rib::CDAP_SUCCESS) {
		//1 Retrieve system from system-name, if it doesn't exist, continue
		mas = NULL;
		et->lock.lock();
		for (it = et->enrolled_systems.begin();
				it != et->enrolled_systems.end(); ++it) {
			if (it->second->system_id == trans->system_id) {
				mas = it->second;
				break;
			}
		}
		et->lock.unlock();

		if (!mas) {
			LOG_WARN("Could not find Managed System with id %d", trans->system_id);
		} else {
			cit = mas->ipcps.find(ipcp_id);
			if (cit != mas->ipcps.end()) {
				delete cit->second;
				mas->ipcps.erase(cit);
			} else {
				LOG_WARN("Could not find IPCP descriptor with id %d in system %d",
					  ipcp_id, mas->system_id);
			}
		}

		ss.str(std::string());
		ss << "/systems/msid=" << trans->system_id
		   << obj.name_;
		rd->removeObjRIB(ss.str(), true);
		trans->completed(NETMAN_SUCCESS);
	} else {
		trans->completed(NETMAN_FAILURE);
	}
	remove_transaction_state(trans->tid);
}

//Class RIBObjectClasses
const std::string RIBObjectClasses::DAF = "DAF";
const std::string RIBObjectClasses::COMPUTING_SYSTEM = "ComputingSystem";
const std::string RIBObjectClasses::PROCESSING_SYSTEM = "ProcessingSystem";
const std::string RIBObjectClasses::SOFTWARE = "Software";
const std::string RIBObjectClasses::HARDWARE = "Hardware";
const std::string RIBObjectClasses::KERNEL_AP = "KernelApplicationProcess";
const std::string RIBObjectClasses::OS_AP = "OSApplicationProcess";
const std::string RIBObjectClasses::IPCPS = "IPCProcesses";
const std::string RIBObjectClasses::MGMT_AGENTS = "ManagementAgents";
const std::string RIBObjectClasses::IPCP = "IPCProcess";

RIBObjectClasses::class_name_code RIBObjectClasses::hash_it(const std::string& class_name)
{
	if (class_name == DAF) return CL_DAF;
	if (class_name == COMPUTING_SYSTEM) return CL_COMPUTING_SYSTEM;
	if (class_name == PROCESSING_SYSTEM) return CL_PROCESSING_SYSTEM;
	if (class_name == SOFTWARE) return CL_SOFTWARE;
	if (class_name == HARDWARE) return CL_HARDWARE;
	if (class_name == KERNEL_AP) return CL_KERNEL_AP;
	if (class_name == OS_AP) return CL_OS_AP;
	if (class_name == IPCPS) return CL_IPCPS;
	if (class_name == MGMT_AGENTS) return CL_MGMT_AGENTS;
	if (class_name == IPCP) return CL_IPCP;

	return CL_UNKNOWN;
}

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
#include <configuration.h>
#include <encoder.h>
#include "net-manager.h"

#include <rina/api.h>

//TODO Add cache with managed system names - system id
//TODO Add DIF template manager

//Class NMConsole
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

		console->outstream << "IPC process created successfully [id = "
		      << promise.ipcp_id << ", name="<<args[1]<<"]" << std::endl;
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

class ListSystemsConsoleCmd: public rina::ConsoleCmdInfo {
public:
	ListSystemsConsoleCmd(NMConsole * console, NetworkManager * nm) :
		rina::ConsoleCmdInfo("USAGE: list-systems", console) {
		netman = nm;
	};

	int execute(std::vector<std::string>& args) {
		console->outstream << netman->list_systems() << std::endl;
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
			rina::UNIXConsole(socket_path)
{
	netman = nm;
	commands_map["create-ipcp"] = new CreateIPCPConsoleCmd(this, netman);
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

	for (it = objs_to_create.begin(); it != objs_to_create.end() /* not hoisted */; /* no increment */) {
		delete it->second;
	        objs_to_create.erase(it++);
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

std::string NetworkManager::list_systems(void)
{
	std::map<std::string, ManagedSystem *>::iterator it;
	std::stringstream ss;

	ss << "        Managed Systems       " << std::endl;
	ss << "    system id    |   MA name   |  port-id  " << std::endl;

	rina::ScopedLock(et->lock);

	for (it = et->enrolled_systems.begin();
			it != et->enrolled_systems.end(); ++it) {
		ss << "       " << it->second->system_id << "   |   ";
		ss << it->second->con.dest_.ap_name_ << "-"
		   << it->second->con.dest_.ap_inst_ << "   |   ";
		ss << "       " << it->second->con.port_id << std::endl;
	}

	return ss.str();
}

netman_res_t NetworkManager::create_ipcp(CreateIPCPPromise * promise, int system_id,
			 	 	 const std::string& ipcp_desc)
{
	MASystemTransState * trans;
	std::map<std::string, ManagedSystem *>::iterator it;
	ManagedSystem * mas;
	rinad::configs::ipcp_config_t ipcp_config;
	rina::cdap_rib::res_info_t res;
	rina::cdap_rib::obj_info_t obj_info;
        rina::cdap_rib::flags_t flags;
        rina::cdap_rib::filt_info_t filt;
        rinad::encoders::IPCPConfigEncoder encoder;
        std::stringstream ss;

	//1 Retrieve system from system-id, if it doesn't exist, return error
	rina::ScopedLock g(et->lock);
	mas = NULL;
	for (it = et->enrolled_systems.begin();
			it != et->enrolled_systems.end(); ++it) {
		if (it->second->system_id == system_id) {
			mas = it->second;
		}
	}

	if (!mas) {
		LOG_ERR("Could not find Managed System with id %d", system_id);
		return NETMAN_FAILURE;
	}

	//2 Read and parse IPCP descriptor
	if (dtm->get_ipcp_config_from_desc(ipcp_config, mas->con.dest_.ap_name_, ipcp_desc)) {
		LOG_ERR("Problems getting IPCP configuration from IPCP descriptor");
		return NETMAN_FAILURE;
	}

	obj_info.name_ = "/csid=1/psid=1/kernelap/osap/ipcps/ipcpid";
	obj_info.class_ = "IPCProcess";
	encoder.encode(ipcp_config, obj_info.value_);

	ss << mas->con.port_id << "-" << obj_info.name_;
	//3 Store transaction
	trans = new MASystemTransState(this, promise, system_id, ss.str());
	if (!trans) {
		LOG_ERR("Unable to allocate memory for the transaction object. Out of memory!");
		return NETMAN_FAILURE;
	}

	if (add_transaction_state(trans) < 0)
	{
		LOG_ERR("Unable to add transaction; out of memory?");
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
		trans->completed(NETMAN_SUCCESS);
	} else {
		trans->completed(NETMAN_FAILURE);
	}
	remove_transaction_state(trans->tid);
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
	rina::ScopedLock g(et->lock);
	mas = NULL;
	for (it = et->enrolled_systems.begin();
			it != et->enrolled_systems.end(); ++it) {
		if (it->second->system_id == system_id) {
			mas = it->second;
		}
	}

	if (!mas) {
		LOG_ERR("Could not find Managed System with id %d", system_id);
		return NETMAN_FAILURE;
	}

	ss << "/csid=1/psid=1/kernelap/osap/ipcps/ipcpid=" << ipcp_id;
	obj_info.name_ = ss.str();;
	obj_info.class_ = "IPCProcess";
	ss.str(std::string());

	ss << mas->con.port_id << "-" << obj_info.name_;
	//3 Store transaction
	trans = new MASystemTransState(this, promise, system_id, ss.str());
	if (!trans) {
		LOG_ERR("Unable to allocate memory for the transaction object. Out of memory!");
		return NETMAN_FAILURE;
	}

	if (add_transaction_state(trans) < 0)
	{
		LOG_ERR("Unable to add transaction; out of memory?");
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

	ss << con.port_id << "-" << obj.name_;

	LOG_INFO("Received object reply for object name %s", obj.name_.c_str());

	// Retrieve transaction
	trans = dynamic_cast<MASystemTransState*>(get_transaction_state(ss.str()));
	if(!trans){
		LOG_ERR("Could not retrieve transaction with id %s", ss.str().c_str());
		return;
	}

	// Mark transaction as completed
	if (res.code_ == rina::cdap_rib::CDAP_SUCCESS) {
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

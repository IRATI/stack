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
#include "net-manager.h"

#include <rina/api.h>

//Class SDUReader
SDUReader::SDUReader(rina::ThreadAttributes * threadAttributes, int port_id, int fd_)
				: SimpleThread(threadAttributes)
{
	portid = port_id;
	fd = fd_;
}

int SDUReader::run()
{
	rina::ser_obj_t message;
	message.message_ = new unsigned char[5000];
	int bytes_read = 0;

	LOG_DBG("SDU reader of port-id %d starting", portid);

	while(true) {
		LOG_DBG("Going to read from file descriptor %d", fd);
		bytes_read = read(fd, message.message_, 5000);
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

	return 0;
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
	mgd_system->invoke_id = message.invoke_id_;
	mgd_system->status = NM_SUCCESS;
	enrolled_systems[con.dest_.ap_name_] = mgd_system;

	lock.unlock();

	nm->enrollment_completed(con);
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

void NMRIBDaemon::removeObjRIB(const std::string& fqn)
{
	ribd->removeObjRIB(rib, fqn);
}

//Class NetworkManager
NetworkManager::NetworkManager(const std::string& app_name,
			       const std::string& app_instance) :
			       	       rina::ApplicationProcess(app_name, app_instance)
{
	std::stringstream ss;

	cfd = 0;
	ss << app_name << "|" << app_instance << "||";
	complete_name = ss.str();

	et = new NMEnrollmentTask();
	et->set_application_process(this);

	rd = new NMRIBDaemon(et);
	rd->set_application_process(this);

	//TODO add required RIB objects to RIB
}

NetworkManager::~NetworkManager()
{
	void * status;
	std::map<int, SDUReader *>::iterator itr;
	SDUReader * reader;

	if (et)
		delete et;

	if (rd)
		delete rd;

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
	rina::ThreadAttributes thread_attrs;
	std::stringstream ss;
	SDUReader * reader = 0;

	rina::ScopedLock g(lock);

	thread_attrs.setJoinable();
	ss << "SDU Reader of fd " << fd;
	thread_attrs.setName(ss.str());
	// Use fd as port-id
	reader = new SDUReader(&thread_attrs, fd, fd);
	reader->start();

	sdu_readers[fd] = reader;

	rina::cdap::add_fd_to_port_id_mapping(fd, fd);
}

void NetworkManager::disconnect_from_system(int fd)
{
	void * status;
	std::map<int, SDUReader *>::iterator itr;
	SDUReader * reader;

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

void NetworkManager::enrollment_completed(const rina::cdap_rib::con_handle_t &con)
{
	LOG_DBG("Enrollment to Management Agent %s %s completed",
			con.dest_.ap_name_.c_str(),
			con.dest_.ap_inst_.c_str());

	//TODO query MA and get relevant system information

}

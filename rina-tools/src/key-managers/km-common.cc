//
// Key Management Agent and Key Management Agent common classes
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

#include <cstring>
#include <iostream>
#include <sstream>
#include <time.h>

#include <openssl/conf.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

#define RINA_PREFIX "key-management-common"
#include <librina/logs.h>
#include <librina/cdap_v2.h>

#include "km-common.h"
#include "kma.h"
#include "ckm.h"
#include "km.pb.h"

/* Macro useful to perform downcasts in declarations. */
#define DOWNCAST_DECL(_var,_class,_name)        \
        _class *_name = dynamic_cast<_class*>(_var);

using namespace std;
using namespace rina;

// Class AppEventLoop
AppEventLoop::AppEventLoop()
{
	keep_running = true;
}

void AppEventLoop::stop()
{
	rina::ScopedLock g(lock);
	keep_running = false;
	rina::librina_finalize();
}

void AppEventLoop::event_loop()
{
	rina::IPCEvent *e;

	keep_running = true;

	LOG_INFO("Starting main I/O loop...");

	while(keep_running) {
		e = rina::ipcEventProducer->eventWait();
		if(!e && !keep_running)
			break;

		if (!e)
			continue;

		if(!keep_running){
			delete e;
			break;
		}

		LOG_DBG("Got event of type %s and sequence number %u",
			rina::IPCEvent::eventTypeToString(e->eventType).c_str(),
			e->sequenceNumber);

		switch(e->eventType){
		case rina::REGISTER_APPLICATION_RESPONSE_EVENT:
		{
			DOWNCAST_DECL(e, rina::RegisterApplicationResponseEvent, event);
			register_application_response_handler(*event);
		}
		break;
		case UNREGISTER_APPLICATION_RESPONSE_EVENT:
		{
			DOWNCAST_DECL(e, rina::UnregisterApplicationResponseEvent, event);
			unregister_application_response_handler(*event);
		}
		break;
		case rina::ALLOCATE_FLOW_REQUEST_RESULT_EVENT:
		{
			DOWNCAST_DECL(e, rina::AllocateFlowRequestResultEvent, event);
			allocate_flow_request_result_handler(*event);
		}
		break;
		case rina::FLOW_ALLOCATION_REQUESTED_EVENT:
		{
			DOWNCAST_DECL(e, rina::FlowRequestEvent, event);
			flow_allocation_requested_handler(*event);
		}
		break;
		case rina::FLOW_DEALLOCATED_EVENT:
		{
			DOWNCAST_DECL(e, rina::FlowDeallocatedEvent, event);
			flow_deallocated_event_handler(*event);
		}
		break;

		//Unsupported events
		case rina::ALLOCATE_FLOW_RESPONSE_EVENT:
		case rina::APPLICATION_REGISTRATION_REQUEST_EVENT:
		case rina::APPLICATION_UNREGISTRATION_REQUEST_EVENT:
		case rina::ENROLL_TO_DIF_REQUEST_EVENT:
		case rina::IPC_PROCESS_QUERY_RIB:
		case rina::IPC_PROCESS_CREATE_CONNECTION_RESPONSE:
		case rina::IPC_PROCESS_CREATE_CONNECTION_RESULT:
		case rina::IPC_PROCESS_UPDATE_CONNECTION_RESPONSE:
		case rina::IPC_PROCESS_DESTROY_CONNECTION_RESULT:
		case rina::IPC_PROCESS_DUMP_FT_RESPONSE:
		case rina::IPC_PROCESS_SET_POLICY_SET_PARAM:
		case rina::IPC_PROCESS_SET_POLICY_SET_PARAM_RESPONSE:
		case rina::IPC_PROCESS_SELECT_POLICY_SET:
		case rina::IPC_PROCESS_SELECT_POLICY_SET_RESPONSE:
		case rina::IPC_PROCESS_PLUGIN_LOAD:
		case rina::IPC_PROCESS_UPDATE_CRYPTO_STATE_RESPONSE:
		case rina::IPC_PROCESS_FWD_CDAP_MSG:
		case rina::FLOW_DEALLOCATION_REQUESTED_EVENT:
		case rina::ASSIGN_TO_DIF_REQUEST_EVENT:
		case rina::ASSIGN_TO_DIF_RESPONSE_EVENT:
		case rina::IPC_PROCESS_DIF_REGISTRATION_NOTIFICATION:
		case rina::APPLICATION_UNREGISTERED_EVENT:
		case rina::APPLICATION_REGISTRATION_CANCELED_EVENT:
		case rina::UPDATE_DIF_CONFIG_REQUEST_EVENT:
		case rina::UPDATE_DIF_CONFIG_RESPONSE_EVENT:
		case rina::ENROLL_TO_DIF_RESPONSE_EVENT:
		case rina::GET_DIF_PROPERTIES:
		case rina::GET_DIF_PROPERTIES_RESPONSE_EVENT:
		case rina::IPCM_REGISTER_APP_RESPONSE_EVENT:
		case rina::IPCM_UNREGISTER_APP_RESPONSE_EVENT:
		case rina::IPCM_ALLOCATE_FLOW_REQUEST_RESULT:
		case rina::QUERY_RIB_RESPONSE_EVENT:
		case rina::IPC_PROCESS_DAEMON_INITIALIZED_EVENT:
		case rina::TIMER_EXPIRED_EVENT:
		case rina::IPC_PROCESS_PLUGIN_LOAD_RESPONSE:
		default:
			break;
		}
		delete e;
	}
}

// Class DummySecurityManagerPs
int DummySecurityManagerPs::isAllowedToJoinDAF(const rina::cdap_rib::con_handle_t & con,
		       	       	       	       const rina::Neighbor& newMember,
					       rina::cdap_rib::auth_policy_t & auth)
{
	return 0;
}

int DummySecurityManagerPs::storeAccessControlCreds(const rina::cdap_rib::auth_policy_t & auth,
			    	    	    	    const rina::cdap_rib::con_handle_t & con)
{
	return 0;
}

int DummySecurityManagerPs::getAccessControlCreds(rina::cdap_rib::auth_policy_t & auth,
		          	  	  	  const rina::cdap_rib::con_handle_t & con)
{
	return 0;
}

void DummySecurityManagerPs::checkRIBOperation(const rina::cdap_rib::auth_policy_t & auth,
		       	       	       	       const rina::cdap_rib::con_handle_t & con,
					       const rina::cdap::cdap_m_t::Opcode opcode,
					       const std::string obj_name,
					       rina::cdap_rib::res_info_t& res)
{
	res.code_ = cdap_rib::CDAP_SUCCESS;
}

int DummySecurityManagerPs::set_policy_set_param(const std::string& name,
			 	 	         const std::string& value)
{
	return 0;
}

// Class Key Manager RIB Daemon
KMRIBDaemon::KMRIBDaemon(rina::cacep::AppConHandlerInterface *app_con_callback)
{
	rina::cdap_rib::cdap_params params;
	rina::cdap_rib::vers_info_t vers;
	rina::rib::RIBObj * robj = 0;

	//Initialize the RIB library and cdap
	params.ipcp = false;
	rina::rib::init(app_con_callback, params);
	ribd = rina::rib::RIBDaemonProxyFactory();

	//Create schema
	vers.version_ = 0x1ULL;
	ribd->createSchema(vers);

	//Add create callbacks
	ribd->addCreateCallbackSchema(vers,
				      KeyContainerRIBObject::class_name,
				      KeyContainersRIBObject::object_name,
				      KeyContainerRIBObject::create_cb);

	//Create RIB
	rib = ribd->createRIB(vers);
	ribd->associateRIBtoAE(rib, "");

	LOG_INFO("RIB Daemon Initialized");

	try {
		robj = new rib::RIBObj("ipcManagement");
		addObjRIB("/ipcManagement", &robj);
	} catch (rina::Exception &e) {
		LOG_ERR("Problems adding object to RIB: %s", e.what());
	}
}

rina::rib::RIBDaemonProxy * KMRIBDaemon::getProxy()
{
	if (ribd)
		return ribd;

	return NULL;
}

const rina::rib::rib_handle_t & KMRIBDaemon::get_rib_handle()
{
	return rib;
}

int64_t KMRIBDaemon::addObjRIB(const std::string& fqn,
			       rina::rib::RIBObj** obj)
{
	return ribd->addObjRIB(rib, fqn, obj);
}

void KMRIBDaemon::removeObjRIB(const std::string& fqn)
{
	ribd->removeObjRIB(rib, fqn);
}

// Class Key Manager SDU Protection Handler
KMSDUProtectionHandler::KMSDUProtectionHandler()
{
	secman = 0;

	/* Initialise OpenSSL library */
	ERR_load_crypto_strings();
	OpenSSL_add_all_algorithms();
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	OPENSSL_config(NULL);
#endif
}

void KMSDUProtectionHandler::set_security_manager(rina::ISecurityManager * sec_man)
{
	secman = sec_man;
}

/// Right now only supports AES256 encryption in CBC mode. Assumes authentication
/// policy is SSH2
void KMSDUProtectionHandler::protect_sdu(rina::ser_obj_t& sdu, int port_id)
{
	SSH2SecurityContext * sc = 0;
	unsigned char * ciphertext = 0;
	int len = 0;
	int ciphertext_len = 0;
	EVP_CIPHER_CTX *ctx = 0;

	sc = static_cast<SSH2SecurityContext *> (secman->get_security_context(port_id));
	if (!sc || !sc->crypto_tx_enabled)
		return;

	/* Create and initialise the context */
	if(!(ctx = EVP_CIPHER_CTX_new())) {
		LOG_ERR("Problems initialising context");
		return;
	}

	/* Initialise the encryption operation. IMPORTANT - ensure you use a key
	 * and IV size appropriate for your cipher
	 * In this example we are using 256 bit AES (i.e. a 256 bit key). The
	 * IV size for *most* modes is the same as the block size. For AES this
	 * is 128 bits */
	if(1 != EVP_EncryptInit_ex(ctx,
				   EVP_aes_256_cbc(),
				   NULL,
				   sc->encrypt_key_client.data,
				   sc->mac_key_client.data)) {
		LOG_ERR("Problems initialising AES 256 CBC context");
		return;
	}

	ciphertext = new unsigned char[1500];
	/* Provide the message to be encrypted, and obtain the encrypted output.
	 * EVP_EncryptUpdate can be called multiple times if necessary
	 */
	if(1 != EVP_EncryptUpdate(ctx,
				  ciphertext,
				  &len,
				  sdu.message_,
				  sdu.size_)) {
		LOG_ERR("Problems encrypting SDU");
		delete[] ciphertext;
		return;
	}
	ciphertext_len = len;

	/* Finalise the encryption. Further ciphertext bytes may be written at
	 * this stage.
	 */
	if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) {
		LOG_ERR("Problems finalising encryption");
		delete[] ciphertext;
		return;
	}
	ciphertext_len += len;

	/* Clean up */
	EVP_CIPHER_CTX_free(ctx);

	delete[] sdu.message_;
	sdu.message_ = ciphertext;
	sdu.size_ = ciphertext_len;
}

/// Right now only supports AES256 encryption in CBC mode. Assumes authentication
/// policy is SSH2
void KMSDUProtectionHandler::unprotect_sdu(rina::ser_obj_t& sdu, int port_id)
{
	SSH2SecurityContext * sc = 0;
	EVP_CIPHER_CTX *ctx = 0;
	unsigned char * plaintext = 0;
	int len = 0;
	int plaintext_len = 0;

	sc = static_cast<SSH2SecurityContext *> (secman->get_security_context(port_id));
	if (!sc || !sc->crypto_rx_enabled)
		return;

	/* Create and initialise the context */
	if(!(ctx = EVP_CIPHER_CTX_new())) {
		LOG_ERR("Problems initialising context");
		return;
	}

	/* Initialise the decryption operation. IMPORTANT - ensure you use a key
	 * and IV size appropriate for your cipher
	 * In this example we are using 256 bit AES (i.e. a 256 bit key). The
	 * IV size for *most* modes is the same as the block size. For AES this
	 * is 128 bits */
	if(1 != EVP_DecryptInit_ex(ctx,
				   EVP_aes_256_cbc(),
				   NULL,
				   sc->encrypt_key_client.data,
				   sc->mac_key_client.data)) {
		LOG_ERR("Problems initialising AES 256 CBC context");
		return;
	}

	plaintext = new unsigned char[1500];
	/* Provide the message to be decrypted, and obtain the plaintext output.
	 * EVP_DecryptUpdate can be called multiple times if necessary
	 */
	if(1 != EVP_DecryptUpdate(ctx,
				  plaintext,
				  &len,
				  sdu.message_,
				  sdu.size_)) {
		LOG_ERR("Problems decrypting sdu");
		delete[] plaintext;
		return;
	}
	plaintext_len = len;

	/* Finalise the decryption. Further plaintext bytes may be written at
	 * this stage.
	 */
	if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) {
		LOG_ERR("Problems finalising decryption");
		delete[] plaintext;
		return;
	}
	plaintext_len += len;

	/* Clean up */
	EVP_CIPHER_CTX_free(ctx);

	delete[] sdu.message_;
	sdu.message_ = plaintext;
	sdu.size_ = plaintext_len;
}

// Class Key Manager Security Manager
KMSecurityManager::KMSecurityManager(const std::string& creds_location)
{
	auth_ps = 0;
	sdup = 0;

	PolicyParameter parameter;
	sec_profile.authPolicy.name_ = IAuthPolicySet::AUTH_SSH2;
	sec_profile.authPolicy.version_ = "1";
	parameter.name_ = "keyExchangeAlg";
	parameter.value_ = "EDH";
	sec_profile.authPolicy.parameters_.push_back(parameter);
	parameter.name_ = "keystore";
	parameter.value_ = creds_location;
	sec_profile.authPolicy.parameters_.push_back(parameter);
	parameter.name_ = "keystorePass";
	parameter.value_ = "test";
	sec_profile.authPolicy.parameters_.push_back(parameter);
	sec_profile.encryptPolicy.name_ = "default";
	sec_profile.encryptPolicy.version_ = "1";
	parameter.name_ = "encryptAlg";
	parameter.value_ = "AES256";
	sec_profile.encryptPolicy.parameters_.push_back(parameter);
	parameter.name_ = "macAlg";
	parameter.value_ = "SHA256";
	sec_profile.encryptPolicy.parameters_.push_back(parameter);
	parameter.name_ = "compressAlg";
	parameter.value_ = "deflate";
	sec_profile.encryptPolicy.parameters_.push_back(parameter);
}

KMSecurityManager::~KMSecurityManager()
{
	if (ps)
		delete ps;

	if (auth_ps)
		delete auth_ps;

	if (sdup)
		delete sdup;
}

void KMSecurityManager::set_application_process(rina::ApplicationProcess * ap)
{
	KMRIBDaemon * ribd = 0;

	if (!ap)
		return;

	app = ap;
	ribd = static_cast<KMRIBDaemon *>(app->get_rib_daemon());
	if (!ribd) {
		LOG_ERR("Could not obtain RIB Daemon");
		return;
	}

	ribd->set_security_manager(this);

	sdup = new KMSDUProtectionHandler();
	sdup->set_security_manager(this);
	rina::cdap::set_sdu_protection_handler(sdup);

	ps = new DummySecurityManagerPs();
	auth_ps = new AuthSSH2PolicySet(ribd->getProxy(), this);
	add_auth_policy_set(IAuthPolicySet::AUTH_SSH2, auth_ps);

	LOG_INFO("Security Manager initialized");
}

rina::IAuthPolicySet::AuthStatus KMSecurityManager::update_crypto_state(const rina::CryptoState& state,
						     	     	     	rina::IAuthPolicySet * caller)
{
	return IAuthPolicySet::SUCCESSFULL;
}

//Class KMIPCResourceManager
KMIPCResourceManager::~KMIPCResourceManager()
{
	map<int, SDUReader *>::iterator it;
	void * status;

	rina::ScopedLock g(lock);

	for (it = sdu_readers.begin(); it != sdu_readers.end(); ++it) {
		it->second->join(&status);
		delete it->second;
	}

	sdu_readers.clear();
}

void KMIPCResourceManager::set_application_process(rina::ApplicationProcess * ap)
{
	KMRIBDaemon * ribd;

	if (!ap) {
		LOG_ERR("Bogus instance of APP passed, return");
		return;
	}

	app = ap;

	ribd = dynamic_cast<KMRIBDaemon *>(app->get_rib_daemon());
	if (!ribd) {
		LOG_ERR("App has no RIB Daemon AE, return");
		return;
	}
	rib_daemon_ = ribd->getProxy();
	rib = ribd->get_rib_handle();

	event_manager_ = dynamic_cast<InternalEventManager*>(app->get_internal_event_manager());
	if (!event_manager_) {
		LOG_ERR("App has no Internal Event Manager AE, return");
		return;
	}
	event_manager_->subscribeToEvent(InternalEvent::APP_N_MINUS_1_FLOW_ALLOCATED,
						 this);
	event_manager_->subscribeToEvent(InternalEvent::APP_N_MINUS_1_FLOW_DEALLOCATED,
					 this);

	populateRIB();
}

void KMIPCResourceManager::eventHappened(InternalEvent * event)
{
	NMinusOneFlowDeallocatedEvent * fd_event = 0;
	NMinusOneFlowAllocatedEvent * fa_event = 0;

	if (event->type == InternalEvent::APP_N_MINUS_1_FLOW_DEALLOCATED) {
		fd_event = static_cast<NMinusOneFlowDeallocatedEvent *>(event);
		stop_flow_reader(fd_event->port_id_);
		rina::cdap::remove_fd_to_port_id_mapping(fd_event->port_id_);
	} else if (event->type == InternalEvent::APP_N_MINUS_1_FLOW_ALLOCATED) {
		fa_event = static_cast<NMinusOneFlowAllocatedEvent *>(event);
		rina::cdap::add_fd_to_port_id_mapping(fa_event->flow_information_.fd,
						      fa_event->flow_information_.portId);
		start_flow_reader(fa_event->flow_information_.portId,
				  fa_event->flow_information_.fd);
	}
}

void KMIPCResourceManager::register_application_response(const rina::RegisterApplicationResponseEvent& event)
{
        ipcManager->commitPendingRegistration(event.sequenceNumber, event.DIFName);

	if (event.result != 0) {
		LOG_ERR("Application registration to DIF %s failed",
			event.DIFName.processName.c_str());
		return;
	} else {
		LOG_INFO("Application registered in DIF %s",
			 event.DIFName.processName.c_str());
	}

	try {
		std::stringstream ss;
		ss << UnderlayingRegistrationRIBObj::object_name_prefix << event.DIFName.processName;

		UnderlayingRegistrationRIBObj * urobj =
				new UnderlayingRegistrationRIBObj(event.DIFName.processName);
		rib_daemon_->addObjRIB(rib, ss.str(), &urobj);
	} catch (Exception &e) {
		LOG_ERR("Problems creating RIB object: %s", e.what());
	}
}

void KMIPCResourceManager::unregister_application_response(const rina::UnregisterApplicationResponseEvent& event)
{
        ipcManager->appUnregistrationResult(event.sequenceNumber, event.result == 0);

        if (event.result != 0) {
        	LOG_ERR("Application unregistration from DIF %s failed",
        		event.DIFName.processName.c_str());
        	return;
	} else {
		LOG_INFO("Application unregistered from DIF %s",
			 event.DIFName.processName.c_str());
	}

	try {
		std::stringstream ss;
		ss << UnderlayingRegistrationRIBObj::object_name_prefix << event.DIFName.processName;
		rib_daemon_->removeObjRIB(rib, ss.str());
	} catch (Exception &e) {
		LOG_ERR("Problems creating RIB object: %s", e.what());
	}
}

void KMIPCResourceManager::applicationRegister(const std::list<std::string>& dif_names,
					       const std::string& app_name,
					       const std::string& app_instance)
{
        ApplicationRegistrationInformation ari;

        ari.ipcProcessId = 0;  // This is an application, not an IPC process
        ari.appName = ApplicationProcessNamingInformation(app_name,
                                                          app_instance);

        for (std::list<std::string>::const_iterator it = dif_names.begin();
        		it != dif_names.end(); ++ it)
        {
        	if (it->compare("") == 0) {
        		ari.applicationRegistrationType = APPLICATION_REGISTRATION_ANY_DIF;
        	} else {
        		ari.applicationRegistrationType = APPLICATION_REGISTRATION_SINGLE_DIF;
        		ari.difName = ApplicationProcessNamingInformation(*it, string());
        	}

        	// Request the registration
        	ipcManager->requestApplicationRegistration(ari);
        }
}

void KMIPCResourceManager::allocate_flow(const ApplicationProcessNamingInformation & local_app_name,
				         const ApplicationProcessNamingInformation & remote_app_name)
{
        FlowSpecification qosspec;

        //Request a reliable flow
        qosspec.maxAllowableGap = 0;
        qosspec.orderedDelivery = true;
        qosspec.msg_boundaries = true;

        ipcManager->requestFlowAllocation(local_app_name,
        				  remote_app_name,
					  qosspec);
}

void KMIPCResourceManager::deallocate_flow(int port_id)
{
	ipcManager->deallocate_flow(port_id);
}

void KMIPCResourceManager::start_flow_reader(int port_id, int fd)
{
	std::stringstream ss;
	SDUReader * reader = 0;

	rina::ScopedLock g(lock);

	reader = new SDUReader(port_id, fd);
	reader->start();

	sdu_readers[port_id] = reader;
}

void KMIPCResourceManager::stop_flow_reader(int port_id)
{
	map<int, SDUReader *>::iterator it;
	void * status;

	ScopedLock g(lock);
	it = sdu_readers.find(port_id);
	if (it != sdu_readers.end()) {
		sdu_readers.erase(it);
		it->second->join(&status);
		delete it->second;
	}
}

//Class AbstractKM
AbstractKM::AbstractKM(const std::string& ap_name,
		       const std::string& ap_instance) :
				       ApplicationProcess(ap_name, ap_instance)
{
	ribd = 0;
	secman = 0;
	irm = 0;
	eventm = 0;
	kcm = 0;
}

AbstractKM::~AbstractKM()
{
	if (ribd) {
		delete ribd;
		ribd = 0;
	}

	if (secman) {
		delete secman;
		secman = 0;
	}

	if (irm) {
		delete irm;
		irm = 0;
	}

	if (eventm) {
		delete eventm;
		eventm = 0;
	}

	if (kcm) {
		delete kcm;
		kcm = 0;
	}
};

unsigned int AbstractKM::get_address() const
{
	return 0;
}

void AbstractKM::unregister_application_response_handler(const rina::UnregisterApplicationResponseEvent& event)
{
	irm->unregister_application_response(event);
}

void AbstractKM::register_application_response_handler(const rina::RegisterApplicationResponseEvent& event)
{
	irm->register_application_response(event);
}

void AbstractKM::allocate_flow_request_result_handler(const rina::AllocateFlowRequestResultEvent& event)
{
	irm->allocateRequestResult(event);
}

void AbstractKM::flow_allocation_requested_handler(const rina::FlowRequestEvent& event)
{
	irm->flowAllocationRequested(event);
}

void AbstractKM::flow_deallocated_event_handler(const rina::FlowDeallocatedEvent& event)
{
	irm->flowDeallocatedRemotely(event);
}

//Class KM Factory
static AbstractKM * km_instance = 0;

AbstractKM * KMFactory::create_central_key_manager(const std::list<std::string>& dif_names,
		  	  	  	           const std::string& app_name,
						   const std::string& app_instance,
						   const std::string& creds_folder)
{
	if (km_instance)
		return 0;

	km_instance = new CentralKeyManager(dif_names,
					    app_name,
					    app_instance,
					    creds_folder);

	return km_instance;
}

AbstractKM * KMFactory::create_key_management_agent(const std::string& creds_folder,
		   	   	   	   	    const std::list<std::string>& dif_names,
						    const std::string& apn,
						    const std::string& api,
						    const std::string& ckm_apn,
						    const std::string& ckm_api,
						    bool  q)
{
	if (km_instance)
		return 0;

	km_instance = new KeyManagementAgent(creds_folder,
					     dif_names,
					     apn,
					     api,
					     ckm_apn,
					     ckm_api,
					     q);

	return km_instance;
}

AbstractKM * KMFactory::get_instance()
{
	if (km_instance)
		return km_instance;

	return 0;
}

// Class SDUReader
SDUReader::SDUReader(int port_id, int fd_)
			: SimpleThread(std::string("sdu-reader"), false)
{
	portid = port_id;
	fd = fd_;
}

int SDUReader::run()
{
	rina::ser_obj_t message;
	message.message_ = new unsigned char[5000];
	int bytes_read = 0;
	bool keep_going = true;

	LOG_DBG("SDU reader of port-id %d starting", portid);

	while(keep_going) {
		LOG_INFO("Going to read from file descriptor %d", fd);
		bytes_read = read(fd, message.message_, 5000);
		if (bytes_read <= 0) {
			LOG_INFO("Error reading fd %d or EOF", fd);
			break;
		}

		LOG_INFO("Read %d bytes", bytes_read);
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

//Class Key Container Manager
KeyContainerManager::KeyContainerManager() : ApplicationEntity("KeyContainerManager")
{
}

KeyContainerManager::~KeyContainerManager()
{
	std::map<std::string, struct key_container *>::iterator it;

	for (it = key_containers.begin(); it != key_containers.end(); ++it) {
		delete it->second;
		it->second = 0;
	}

	key_containers.clear();
}

void KeyContainerManager::set_application_process(rina::ApplicationProcess * ap)
{
	if (!ap)
		return;
}

int KeyContainerManager::generate_rsa_key_pair(struct key_container * kc)
{
	RSA * myrsa;
	BIO * pri, * pub;
	BIGNUM * bne = NULL;
	unsigned long e = RSA_3;
	int bits = 1024;

	bne = BN_new();
	if (BN_set_word(bne, e) != 1) {
		BN_free(bne);
		return -1;
	}

	myrsa = RSA_new();
	if (RSA_generate_key_ex(myrsa, bits, bne, NULL) != 1) {
		LOG_ERR("Problems generating RSA key");
		RSA_free(myrsa);
		BN_free(bne);
		return -1;
	}

	pri = BIO_new(BIO_s_mem());
	pub = BIO_new(BIO_s_mem());

	PEM_write_bio_RSAPrivateKey(pri, myrsa, NULL, NULL, 0, NULL, NULL);
	PEM_write_bio_RSAPublicKey(pub, myrsa);

	kc->private_key.size_ = BIO_pending(pri);
	kc->private_key.message_ = new unsigned char[kc->private_key.size_];
	kc->public_key.size_ = BIO_pending(pub);
	kc->public_key.message_ = new unsigned char[kc->public_key.size_];

	BIO_read(pri, kc->private_key.message_, kc->private_key.size_);
	BIO_read(pub, kc->public_key.message_, kc->public_key.size_);

	RSA_free(myrsa);
	BN_free(bne);

	return 0;
}

void KeyContainerManager::add_key_container(struct key_container * kc)
{
	if (!kc)
		return;

	ScopedLock g(lock);
	key_containers[kc->key_id] = kc;
}

struct key_container * KeyContainerManager::find_key_container(const std::string& key)
{
	std::map<std::string, struct key_container *>::iterator it;

	ScopedLock g(lock);

	it = key_containers.find(key);
	if (it != key_containers.end())
		return it->second;

	return 0;
}

struct key_container * KeyContainerManager::remove_key_container(const std::string& key)
{
	std::map<std::string, struct key_container *>::iterator it;

	ScopedLock g(lock);

	it = key_containers.find(key);
	if (it != key_containers.end()) {
		key_containers.erase(it);
		return it->second;
	}

	return 0;
}

void KeyContainerManager::encode_key_container_message(const struct key_container& kc,
			          	 	       rina::ser_obj_t& result)
{
	rina::messages::key_container_t gpb_kc;

	gpb_kc.set_id(kc.key_id);
	gpb_kc.set_state(kc.state);

	if (kc.private_key.size_ > 0) {
		gpb_kc.set_private_key(kc.private_key.message_,
				       kc.private_key.size_);
	}

	if (kc.public_key.size_ > 0) {
		gpb_kc.set_public_key(kc.public_key.message_,
				      kc.public_key.size_);
	}

	int size = gpb_kc.ByteSize();
	result.message_ = new unsigned char[size];
	result.size_ = size;
	gpb_kc.SerializeToArray(result.message_, size);
}

void KeyContainerManager::decode_key_container_message(const rina::ser_obj_t &message,
					 	       struct key_container& kc)
{
	rina::messages::key_container_t gpb_kc;

	gpb_kc.ParseFromArray(message.message_, message.size_);

	if (gpb_kc.has_private_key()) {
		  kc.private_key.message_ =
				  new unsigned char[gpb_kc.private_key().size()];
		  memcpy(kc.private_key.message_,
			 gpb_kc.private_key().data(),
			 gpb_kc.private_key().size());
		  kc.private_key.size_ = gpb_kc.private_key().size();
	}

	if (gpb_kc.has_public_key()) {
		  kc.public_key.message_ =
				  new unsigned char[gpb_kc.public_key().size()];
		  memcpy(kc.public_key.message_,
			 gpb_kc.public_key().data(),
			 gpb_kc.public_key().size());
		  kc.public_key.size_ = gpb_kc.public_key().size();
	}

	kc.key_id = gpb_kc.id();
	kc.state = gpb_kc.state();
}

//Class Key Containers RIB Object
const std::string KeyContainersRIBObject::class_name = "keyContainers";
const std::string KeyContainersRIBObject::object_name = "/keyContainers";
KeyContainersRIBObject::KeyContainersRIBObject() : rib::RIBObj(class_name)
{
}

//Class Key Container RIB Object
const std::string KeyContainerRIBObject::class_name = "keyContainer";
const std::string KeyContainerRIBObject::object_name_prefix = "/keyContainers/id=";
KeyContainerRIBObject::KeyContainerRIBObject(KeyContainerManager * kcm_,
		      	      	      	     const std::string& id) : rib::RIBObj(class_name)
{
	kcm = kcm_;
	kc_id = id;
}

const std::string KeyContainerRIBObject::get_displayable_value() const
{
	std::stringstream ss;

	struct key_container * kc = kcm->remove_key_container(kc_id);
	if (!kc)
		return "";

	ss << "Key container id: " << kc->key_id
	   << "; state: " << kc->state << std::endl;

	return ss.str();
}

bool KeyContainerRIBObject::delete_(const rina::cdap_rib::con_handle_t &con,
				    const std::string& fqn,
				    const std::string& class_,
				    const rina::cdap_rib::filt_info_t &filt,
				    const int invoke_id,
				    rina::cdap_rib::res_info_t& res)
{
	struct key_container * kc = kcm->remove_key_container(kc_id);

	if (kc) {
		delete kc;
		LOG_INFO("Deleted Key container with ID %s", kc_id.c_str());
	}

	res.code_ = rina::cdap_rib::CDAP_SUCCESS;
	return true;
}

void KeyContainerRIBObject::read(const rina::cdap_rib::con_handle_t &con,
				 const std::string& fqn,
				 const std::string& class_,
				 const rina::cdap_rib::filt_info_t &filt,
				 const int invoke_id,
				 rina::cdap_rib::obj_info_t &obj_reply,
				 rina::cdap_rib::res_info_t& res)
{
	struct key_container * kc = kcm->find_key_container(kc_id);

	if (!kc) {
		res.code_ = rina::cdap_rib::CDAP_ERROR;
		res.reason_ = "Could not find associated key container";
		return;
	}

	KeyContainerManager::encode_key_container_message(*kc, obj_reply.value_);
	res.code_ = rina::cdap_rib::CDAP_SUCCESS;

	LOG_INFO("Value of key container %s read", fqn.c_str());
}

void KeyContainerRIBObject::create_cb(const rina::rib::rib_handle_t rib,
			      	      const rina::cdap_rib::con_handle_t &con,
				      const std::string& fqn,
				      const std::string& class_,
				      const rina::cdap_rib::filt_info_t &filt,
				      const int invoke_id,
				      const rina::ser_obj_t &obj_req,
				      rina::cdap_rib::obj_info_t &obj_reply,
				      rina::cdap_rib::res_info_t& res)
{
	struct key_container * kc = 0;
	AbstractKM * km_instance = 0;
	rib::RIBObj * kc_ribo = 0;
	std::stringstream ss;

	if (!obj_req.message_) {
		LOG_ERR("Object value is null");
		res.code_ = rina::cdap_rib::CDAP_INVALID_OBJ;
		return;
	}

	kc = new key_container();
	KeyContainerManager::decode_key_container_message(obj_req, *kc);
	km_instance = KMFactory::get_instance();
	km_instance->kcm->add_key_container(kc);
	LOG_INFO("Created new Key Container with id %s",
		  kc->key_id.c_str());

	//Add new object to the RIB
	try {
		kc_ribo = new KeyContainerRIBObject(km_instance->kcm,
				kc->key_id);
		ss << KeyContainerRIBObject::object_name_prefix << kc->key_id;
		km_instance->ribd->addObjRIB(ss.str(), &kc_ribo);
	} catch (rina::Exception &e) {
		LOG_ERR("Problems adding object %s to RIB: %s",
				ss.str().c_str(), e.what());
	}

	res.code_ = rina::cdap_rib::CDAP_SUCCESS;
}

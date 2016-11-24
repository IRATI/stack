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
#include <openssl/evp.h>
#include <openssl/err.h>

#define RINA_PREFIX "key-management-common"
#include <librina/logs.h>

#include "km-common.h"

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
}

void AppEventLoop::event_loop()
{
	rina::IPCEvent *e;

	keep_running = true;

	LOG_INFO("Starting main I/O loop...");

	while(keep_running) {
		e = rina::ipcEventProducer->eventTimedWait(0, 1000000000);
		if(!e)
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
		case rina::DEALLOCATE_FLOW_RESPONSE_EVENT:
		{
			DOWNCAST_DECL(e, rina::DeallocateFlowResponseEvent, event);
			deallocate_flow_response_handler(*event);
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
		case rina::OS_PROCESS_FINALIZED:
		case rina::IPCM_REGISTER_APP_RESPONSE_EVENT:
		case rina::IPCM_UNREGISTER_APP_RESPONSE_EVENT:
		case rina::IPCM_DEALLOCATE_FLOW_RESPONSE_EVENT:
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
	params.is_IPCP_ = false;
	rina::rib::init(app_con_callback, params);
	ribd = rina::rib::RIBDaemonProxyFactory();

	//Create schema
	vers.version_ = 0x1ULL;
	ribd->createSchema(vers);

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
	OPENSSL_config(NULL);
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
	} else if (event->type == InternalEvent::APP_N_MINUS_1_FLOW_ALLOCATED) {
		fa_event = static_cast<NMinusOneFlowAllocatedEvent *>(event);
		start_flow_reader(fa_event->flow_information_.portId);
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

        ipcManager->requestFlowAllocation(local_app_name,
        				  remote_app_name,
					  qosspec);
}

void KMIPCResourceManager::deallocate_flow(int port_id)
{
	ipcManager->requestFlowDeallocation(port_id);
}

void KMIPCResourceManager::start_flow_reader(int port_id)
{
	ThreadAttributes thread_attrs;
	std::stringstream ss;
	SDUReader * reader = 0;

	rina::ScopedLock g(lock);

	thread_attrs.setJoinable();
	ss << "SDU Reader of port-id " << port_id;
	thread_attrs.setName(ss.str());
	reader = new SDUReader(&thread_attrs, port_id);
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

//Class KMEventLoop
void KMEventLoop::set_km_irm(KMIPCResourceManager * irm)
{
	kirm = irm;
}

void KMEventLoop::unregister_application_response_handler(const rina::UnregisterApplicationResponseEvent& event)
{
	kirm->unregister_application_response(event);
}

void KMEventLoop::register_application_response_handler(const rina::RegisterApplicationResponseEvent& event)
{
	kirm->register_application_response(event);
}

void KMEventLoop::allocate_flow_request_result_handler(const rina::AllocateFlowRequestResultEvent& event)
{
	kirm->allocateRequestResult(event);
}

void KMEventLoop::flow_allocation_requested_handler(const rina::FlowRequestEvent& event)
{
	kirm->flowAllocationRequested(event);
}

void KMEventLoop::deallocate_flow_response_handler(const rina::DeallocateFlowResponseEvent& event)
{
	kirm->deallocateFlowResponse(event);
}

void KMEventLoop::flow_deallocated_event_handler(const rina::FlowDeallocatedEvent& event)
{
	kirm->flowDeallocatedRemotely(event);
}

// Class SDUReader
SDUReader::SDUReader(rina::ThreadAttributes * threadAttributes, int port_id)
				: SimpleThread(threadAttributes)
{
	portid = port_id;
}

int SDUReader::run()
{
	rina::ser_obj_t message;
	message.message_ = new unsigned char[5000];
	int bytes_read = 0;
	bool keep_going = true;

	LOG_DBG("SDU reader of port-id %d starting", portid);

	while(keep_going) {
		try {
			bytes_read = ipcManager->readSDU(portid, message.message_, 5000);
			message.size_ = bytes_read;
		} catch (rina::FlowAllocationException &e) {
			LOG_ERR("Flow has been deallocated");
			break;
		} catch (rina::UnknownFlowException &e) {
			LOG_ERR("Flow does not exist");
			break;
		} catch (rina::Exception &e) {
			LOG_ERR("Problems reading SDU from flow, exiting");
			break;
		}

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

/*
 * Flow Manager
 *
 *    Bernat Gaston         <bernat.gaston@i2cat.net>
 *    Marc Sune             <marc.sune (at) bisdn.de>
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

#include "ribf.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include <openssl/conf.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

#include "agent.h"
#define RINA_PREFIX "ipcm.mad.ribf"
#include <librina/logs.h>

#include "ribs/ribd_v1.h"

using namespace rina;
using namespace rina::rib;


namespace rinad {
namespace mad {

//static (unique) ribd proxy ref
rina::rib::RIBDaemonProxy* RIBFactory::ribd = NULL;

/*
 * RIBFactory
 */

//Constructors destructors
RIBFactory::RIBFactory(RIBAEassoc ver_assoc){

	cdap_rib::cdap_params params;
	rib_handle_t rib_handle;
	RIBAEassoc::const_iterator it;
	std::list<std::string>::const_iterator itt;


	//First initialize the RIB library
	params.ipcp = false;
	rina::rib::init(&rib_con_handler, params);

	for (it = ver_assoc.begin();
			it != ver_assoc.end(); ++it) {
		uint64_t ver = it->first;
		const std::list<std::string>& aes = it->second;

		//If schema for this version does not exist
		switch(ver) {
			case 0x1ULL:
				{
				//Create the schema FIXME there should be an
				//exists
				try{
					rib_v1::createSchema();
				}catch(eSchemaExists& s){}

				//Create the RIB
				rib_handle = rib_v1::createRIB();

				//Register to all AEs
				for(itt = aes.begin(); itt != aes.end(); ++itt)
					rib_v1::associateRIBtoAE(rib_handle,
									*itt);

				//Store the version
				ribs[rib_handle] = ver;
				}
				break;
			default:
				assert(0);
				throw rina::Exception("Invalid version");
		}
	}

	sec_man = new MASecurityManager("/usr/local/irati/mad/creds");
	sec_man->set_rib_daemon(RIBFactory::getProxy());
	rib_con_handler.setSecurityManager(sec_man);

	LOG_DBG("Initialized");
}

RIBFactory::~RIBFactory() throw (){
	// Destroy RIBDaemon
	rina::rib::fini();
	// FIXME destroy con handlers and resp handlers
	LOG_INFO("RIBFactory destructor called");
}

/*
 * Internal API
 */

rina::rib::rib_handle_t RIBFactory::getRIBHandle(
						const uint64_t& version,
						const std::string& ae_name){
	rina::cdap_rib::vers_info_t vers;
	vers.version_ = (long) version;

	return ribd->get(vers, ae_name);
}

//Process IPCP create event to all RIB versions
void RIBFactory::createIPCPevent(int ipcp_id){

	std::map<rib_handle_t, uint64_t>::const_iterator it;

	for(it = ribs.begin(); it != ribs.end(); ++it){
		switch(it->second){
			case 0x1ULL:
				rib_v1::createIPCPObj(it->first, ipcp_id);
				break;
			default:
				assert(0);
				break;
		}
	}
}

//Process IPCP create event to all RIB versions
void RIBFactory::destroyIPCPevent(int ipcp_id){

	std::map<rib_handle_t, uint64_t>::const_iterator it;

	for(it = ribs.begin(); it != ribs.end(); ++it){
		switch(it->second){
			case 0x1ULL:
				rib_v1::destroyIPCPObj(it->first, ipcp_id);
				break;
			default:
				assert(0);
				break;
		}
	}
}

MASecurityManager * RIBFactory::getSecurityManager()
{
	return sec_man;
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

// Class MA SDU Protection Handler
MASDUProtectionHandler::MASDUProtectionHandler()
{
	secman = 0;

	/* Initialise OpenSSL library */
	ERR_load_crypto_strings();
	OpenSSL_add_all_algorithms();
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	OPENSSL_config(NULL);
#endif
}

void MASDUProtectionHandler::set_security_manager(rina::ISecurityManager * sec_man)
{
	secman = sec_man;
}

/// Right now only supports AES256 encryption in CBC mode. Assumes authentication
/// policy is SSH2
void MASDUProtectionHandler::protect_sdu(rina::ser_obj_t& sdu, int port_id)
{
	/*SSH2SecurityContext * sc = 0;
	unsigned char * ciphertext = 0;
	int len = 0;
	int ciphertext_len = 0;
	EVP_CIPHER_CTX *ctx = 0;

	sc = static_cast<SSH2SecurityContext *> (secman->get_security_context(port_id));
	if (!sc || !sc->crypto_tx_enabled)
		return; */

	/* Create and initialise the context */
	/*if(!(ctx = EVP_CIPHER_CTX_new())) {
		LOG_ERR("Problems initialising context");
		return;
	}*/

	/* Initialise the encryption operation. IMPORTANT - ensure you use a key
	 * and IV size appropriate for your cipher
	 * In this example we are using 256 bit AES (i.e. a 256 bit key). The
	 * IV size for *most* modes is the same as the block size. For AES this
	 * is 128 bits */
	/*if(1 != EVP_EncryptInit_ex(ctx,
				   EVP_aes_256_cbc(),
				   NULL,
				   sc->encrypt_key_client.data,
				   sc->mac_key_client.data)) {
		LOG_ERR("Problems initialising AES 256 CBC context");
		return;
	}*/

	//ciphertext = new unsigned char[1500];
	/* Provide the message to be encrypted, and obtain the encrypted output.
	 * EVP_EncryptUpdate can be called multiple times if necessary
	 */
	/*if(1 != EVP_EncryptUpdate(ctx,
				  ciphertext,
				  &len,
				  sdu.message_,
				  sdu.size_)) {
		LOG_ERR("Problems encrypting SDU");
		delete[] ciphertext;
		return;
	}
	ciphertext_len = len;*/

	/* Finalise the encryption. Further ciphertext bytes may be written at
	 * this stage.
	 */
	/*if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) {
		LOG_ERR("Problems finalising encryption");
		delete[] ciphertext;
		return;
	}
	ciphertext_len += len;*/

	/* Clean up */
	/*EVP_CIPHER_CTX_free(ctx);

	delete[] sdu.message_;
	sdu.message_ = ciphertext;
	sdu.size_ = ciphertext_len;*/
}

/// Right now only supports AES256 encryption in CBC mode. Assumes authentication
/// policy is SSH2
void MASDUProtectionHandler::unprotect_sdu(rina::ser_obj_t& sdu, int port_id)
{
	/*SSH2SecurityContext * sc = 0;
	EVP_CIPHER_CTX *ctx = 0;
	unsigned char * plaintext = 0;
	int len = 0;
	int plaintext_len = 0;

	sc = static_cast<SSH2SecurityContext *> (secman->get_security_context(port_id));
	if (!sc || !sc->crypto_rx_enabled)
		return;*/

	/* Create and initialise the context */
	/*if(!(ctx = EVP_CIPHER_CTX_new())) {
		LOG_ERR("Problems initialising context");
		return;
	}*/

	/* Initialise the decryption operation. IMPORTANT - ensure you use a key
	 * and IV size appropriate for your cipher
	 * In this example we are using 256 bit AES (i.e. a 256 bit key). The
	 * IV size for *most* modes is the same as the block size. For AES this
	 * is 128 bits */
	/*if(1 != EVP_DecryptInit_ex(ctx,
				   EVP_aes_256_cbc(),
				   NULL,
				   sc->encrypt_key_client.data,
				   sc->mac_key_client.data)) {
		LOG_ERR("Problems initialising AES 256 CBC context");
		return;
	}

	plaintext = new unsigned char[1500];*/
	/* Provide the message to be decrypted, and obtain the plaintext output.
	 * EVP_DecryptUpdate can be called multiple times if necessary
	 */
	/*if(1 != EVP_DecryptUpdate(ctx,
				  plaintext,
				  &len,
				  sdu.message_,
				  sdu.size_)) {
		LOG_ERR("Problems decrypting sdu");
		delete[] plaintext;
		return;
	}
	plaintext_len = len;*/

	/* Finalise the decryption. Further plaintext bytes may be written at
	 * this stage.
	 */
	/*if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) {
		LOG_ERR("Problems finalising decryption");
		delete[] plaintext;
		return;
	}
	plaintext_len += len;*/

	/* Clean up */
	/*EVP_CIPHER_CTX_free(ctx);

	delete[] sdu.message_;
	sdu.message_ = plaintext;
	sdu.size_ = plaintext_len;*/
}

// Class Key Manager Security Manager
MASecurityManager::MASecurityManager(const std::string& creds_location)
{
	PolicyParameter parameter;

	ssh2_auth_ps = 0;
	none_auth_ps = 0;
	sdup = 0;

	auth_none_sec_profile.authPolicy.name_ = IAuthPolicySet::AUTH_NONE;
	auth_none_sec_profile.authPolicy.version_ = "1";

	auth_ssh2_sec_profile.authPolicy.name_ = IAuthPolicySet::AUTH_SSH2;
	auth_ssh2_sec_profile.authPolicy.version_ = "1";
	parameter.name_ = "keyExchangeAlg";
	parameter.value_ = "EDH";
	auth_ssh2_sec_profile.authPolicy.parameters_.push_back(parameter);
	parameter.name_ = "keystore";
	parameter.value_ = creds_location;
	auth_ssh2_sec_profile.authPolicy.parameters_.push_back(parameter);
	parameter.name_ = "keystorePass";
	parameter.value_ = "test";
	auth_ssh2_sec_profile.authPolicy.parameters_.push_back(parameter);
	auth_ssh2_sec_profile.encryptPolicy.name_ = "default";
	auth_ssh2_sec_profile.encryptPolicy.version_ = "1";
	parameter.name_ = "encryptAlg";
	parameter.value_ = "AES256";
	auth_ssh2_sec_profile.encryptPolicy.parameters_.push_back(parameter);
	parameter.name_ = "macAlg";
	parameter.value_ = "SHA256";
	auth_ssh2_sec_profile.encryptPolicy.parameters_.push_back(parameter);
	parameter.name_ = "compressAlg";
	parameter.value_ = "deflate";
	auth_ssh2_sec_profile.encryptPolicy.parameters_.push_back(parameter);
}

MASecurityManager::~MASecurityManager()
{
	if (ps)
		delete ps;

	if (ssh2_auth_ps)
		delete ssh2_auth_ps;

	if (none_auth_ps)
		delete none_auth_ps;

	if (sdup)
		delete sdup;
}

rina::AuthSDUProtectionProfile MASecurityManager::get_sec_profile(const std::string& auth_policy_name)
{
	if (auth_policy_name == IAuthPolicySet::AUTH_SSH2)
		return auth_ssh2_sec_profile;
	else
		return auth_none_sec_profile;
}

void MASecurityManager::set_application_process(rina::ApplicationProcess * ap)
{
}

void MASecurityManager::set_rib_daemon(rina::rib::RIBDaemonProxy * ribd)
{
	sdup = new MASDUProtectionHandler();
	sdup->set_security_manager(this);
	rina::cdap::set_sdu_protection_handler(sdup);

	ps = new DummySecurityManagerPs();
	ssh2_auth_ps = new AuthSSH2PolicySet(ribd, this);
	add_auth_policy_set(IAuthPolicySet::AUTH_SSH2, ssh2_auth_ps);

	none_auth_ps = new AuthNonePolicySet(this);
	add_auth_policy_set(IAuthPolicySet::AUTH_NONE, none_auth_ps);

	LOG_INFO("Security Manager initialized");
}

rina::IAuthPolicySet::AuthStatus MASecurityManager::update_crypto_state(const rina::CryptoState& state,
						     	     	     	rina::IAuthPolicySet * caller)
{
	return IAuthPolicySet::SUCCESSFULL;
}

RIBConHandler::RIBConHandler() : AppConHandlerInterface()
{
	sec_man = 0;
}

void RIBConHandler::setSecurityManager(MASecurityManager * sec_man_)
{
	sec_man = sec_man_;
}

void RIBConHandler::connect(const rina::cdap::CDAPMessage& message,
			    rina::cdap_rib::con_handle_t &con) {
	(void) message;
	(void) con;
}

void RIBConHandler::connectResult(const rina::cdap::CDAPMessage& message,
				  rina::cdap_rib::con_handle_t &con)
{
	std::string object_name;

	if (con.dest_.ap_name_ == "pristine.local.key.ma") {
		object_name = "/keyContainers/id=A.kmdemo";
	} else if (con.dest_.ap_name_ == "pristine.local.key.ma2") {
		object_name = "/keyContainers/id=B.kmdemo";
	} else {
		LOG_INFO("Connected to the Manager");
		return;
	}

	LOG_INFO("Connected to Local Key Management Agent, subscribing to key container %s...",
		 object_name.c_str());

	try {
		cdap_rib::flags_t flags;
		cdap_rib::filt_info_t filt;
		cdap_rib::obj_info_t obj_info;
		cdap::StringEncoder encoder;

		obj_info.class_ = "keyContainer";
		obj_info.name_ = object_name;
		obj_info.inst_ = 0;

		RIBFactory::getProxy()->remote_read(con,
						    obj_info,
						    flags,
						    filt,
						    this);
	} catch (Exception &e) {
		LOG_ERR("Problems encoding and sending CDAP message to KMA %s: %s",
				con.dest_.ap_name_.c_str(),
				e.what());
	}
}
void RIBConHandler::release(int message_id,
				const rina::cdap_rib::con_handle_t &con) {
	(void) message_id;
	(void) con;
}
void RIBConHandler::releaseResult(
		const rina::cdap_rib::res_info_t &res,
		const rina::cdap_rib::con_handle_t &con) {
	(void) res;
	(void) con;
}
void RIBConHandler::process_authentication_message(const rina::cdap::CDAPMessage& message,
				    	    	   const rina::cdap_rib::con_handle_t &con)
{
	int result = 0;
	rina::IAuthPolicySet * ps = 0;

	ps = sec_man->get_auth_policy_set(con.auth_.name);
	result = ps->process_incoming_message(message,
					      con.port_id);

	if (result == rina::IAuthPolicySet::IN_PROGRESS) {
		LOG_INFO("Authentication still in progress");
		return;
	}

	if (result == rina::IAuthPolicySet::FAILED) {
		LOG_ERR("Authentication failed");
		return;
	}

	LOG_INFO("Authentication was successful, waiting for M_CONNECT_R");
}

void RIBConHandler::remoteReadResult(const rina::cdap_rib::con_handle_t &con,
		      	      	     const rina::cdap_rib::obj_info_t &obj,
				     const rina::cdap_rib::res_info_t &res)
{
	LOG_INFO("Got read result for object name %s", obj.name_.c_str());
}

};//namespace mad
};//namespace rinad

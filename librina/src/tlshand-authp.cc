//
// TLS Handshake authentication policy
//
//    Eduard Grasa <eduard.grasa@i2cat.net>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA  02110-1301  USA
//

#include <time.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#define RINA_PREFIX "librina.tls-handshake"

#include "librina/logs.h"
#include "librina/tlshand-authp.h"
#include "auth-policies.pb.h"

namespace rina {

//TLSHandAuthOptions encoder and decoder operations
void decode_tls_hand_auth_options(const ser_obj_t &message,
				  TLSHandAuthOptions &options)
{
	rina::auth::policies::googleprotobuf::authOptsTLSHandshake_t gpb_options;

	gpb_options.ParseFromArray(message.message_, message.size_);

	for(int i=0; i<gpb_options.cipher_suites_size(); i++) {
		options.cipher_suites.push_back(gpb_options.cipher_suites(i));
	}

	for(int i=0; i<gpb_options.compress_methods_size(); i++) {
		options.compress_methods.push_back(gpb_options.compress_methods(i));
	}

	options.random.utc_unix_time = gpb_options.utc_unix_time();

	if (gpb_options.has_random_bytes()) {
		  options.random.random_bytes.data =
				  new unsigned char[gpb_options.random_bytes().size()];
		  memcpy(options.random.random_bytes.data,
			 gpb_options.random_bytes().data(),
			 gpb_options.random_bytes().size());
		  options.random.random_bytes.length = gpb_options.random_bytes().size();
	}
}

void encode_tls_hand_auth_options(const TLSHandAuthOptions& options,
			          ser_obj_t& result)
{
	rina::auth::policies::googleprotobuf::authOptsTLSHandshake_t gpb_options;

	for(std::list<std::string>::const_iterator it = options.cipher_suites.begin();
			it != options.cipher_suites.end(); ++it) {
		gpb_options.add_cipher_suites(*it);
	}

	for(std::list<std::string>::const_iterator it = options.compress_methods.begin();
			it != options.compress_methods.end(); ++it) {
		gpb_options.add_compress_methods(*it);
	}

	gpb_options.set_utc_unix_time(options.random.utc_unix_time);

	if (options.random.random_bytes.length > 0) {
		gpb_options.set_random_bytes(options.random.random_bytes.data,
					     options.random.random_bytes.length);
	}

	int size = gpb_options.ByteSize();
	result.message_ = new char[size];
	result.size_ = size;
	gpb_options.SerializeToArray(result.message_, size);
}

void encode_server_hello_tls_hand(const TLSHandRandom& random,
				  const std::string& cipher_suite,
				  const std::string& compress_method,
				  const std::string& version,
				  ser_obj_t& result)
{
	rina::auth::policies::googleprotobuf::serverHelloTLSHandshake_t gpb_hello;

	gpb_hello.set_random_bytes(random.random_bytes.data,
				   random.random_bytes.length);
	gpb_hello.set_utc_unix_time(random.utc_unix_time);
	gpb_hello.set_version(version);
	gpb_hello.set_cipher_suite(cipher_suite);
	gpb_hello.set_compress_method(compress_method);

	int size = gpb_hello.ByteSize();
	result.message_ = new char[size];
	result.size_ = size;
	gpb_hello.SerializeToArray(result.message_ , size);
}

void decode_server_hello_tls_hand(const ser_obj_t &message,
				  TLSHandRandom& random,
				  std::string& cipher_suite,
				  std::string& compress_method,
				  std::string& version)
{
	rina::auth::policies::googleprotobuf::serverHelloTLSHandshake_t gpb_hello;

	gpb_hello.ParseFromArray(message.message_, message.size_);

	if (gpb_hello.has_random_bytes()) {
		random.random_bytes.data =
				  new unsigned char[gpb_hello.random_bytes().size()];
		memcpy(random.random_bytes.data,
		       gpb_hello.random_bytes().data(),
		       gpb_hello.random_bytes().size());
		random.random_bytes.length = gpb_hello.random_bytes().size();
	}

	random.utc_unix_time = gpb_hello.utc_unix_time();
	version = gpb_hello.version();
	cipher_suite = gpb_hello.cipher_suite();
	compress_method = gpb_hello.compress_method();
}

// Class TLSHandSecurityContext
const std::string TLSHandSecurityContext::CIPHER_SUITE = "cipherSuite";
const std::string TLSHandSecurityContext::COMPRESSION_METHOD = "compressionMethod";
const std::string TLSHandSecurityContext::KEYSTORE_PATH = "keystore";
const std::string TLSHandSecurityContext::KEYSTORE_PASSWORD = "keystorePass";

TLSHandSecurityContext::~TLSHandSecurityContext()
{
}

CryptoState TLSHandSecurityContext::get_crypto_state(bool enable_crypto_tx,
					 	     bool enable_crypto_rx)
{
	CryptoState result;
	result.enable_crypto_tx = enable_crypto_tx;
	result.enable_crypto_rx = enable_crypto_rx;
	//TODO
	result.port_id = id;

	return result;
}

TLSHandSecurityContext::TLSHandSecurityContext(int session_id,
				   	       const AuthSDUProtectionProfile& profile)
		: ISecurityContext(session_id)
{
	cipher_suite = profile.authPolicy.get_param_value_as_string(CIPHER_SUITE);
	compress_method = profile.encryptPolicy.get_param_value_as_string(COMPRESSION_METHOD);
	keystore_path = profile.authPolicy.get_param_value_as_string(KEYSTORE_PATH);
	if (keystore_path == std::string()) {
		//TODO set the configuration directory as the default keystore path
	}
	keystore_password = profile.authPolicy.get_param_value_as_string(KEYSTORE_PASSWORD);
	crcPolicy = profile.crcPolicy;
	ttlPolicy = profile.ttlPolicy;
	encrypt_policy_config = profile.encryptPolicy;
	con.port_id = session_id;

	timer_task = NULL;
	state = BEGIN;
}

TLSHandSecurityContext::TLSHandSecurityContext(int session_id,
					       const AuthSDUProtectionProfile& profile,
					       TLSHandAuthOptions * options)
		: ISecurityContext(session_id)
{
	std::string option = options->cipher_suites.front();
	if (option != "TODO") {
		LOG_ERR("Unsupported cipher suite: %s",
			option.c_str());
		throw Exception();
	} else {
		cipher_suite = option;
	}

	option = options->compress_methods.front();
	if (option != "TODO") {
		LOG_ERR("Unsupported compression method: %s",
			option.c_str());
		throw Exception();
	} else {
		compress_method = option;
	}

	client_random = options->random;

	keystore_path = profile.authPolicy.get_param_value_as_string(KEYSTORE_PATH);
	if (keystore_path == std::string()) {
		//TODO set the configuration directory as the default keystore path
	}
	keystore_password = profile.authPolicy.get_param_value_as_string(KEYSTORE_PASSWORD);
	crcPolicy = profile.crcPolicy;
	ttlPolicy = profile.ttlPolicy;
	encrypt_policy_config = profile.encryptPolicy;
	con.port_id = session_id;
	timer_task = NULL;

	state = BEGIN;
}

//Class AuthTLSHandPolicySet
const int AuthTLSHandPolicySet::DEFAULT_TIMEOUT = 10000;
const std::string AuthTLSHandPolicySet::SERVER_HELLO = "Server Hello";

AuthTLSHandPolicySet::AuthTLSHandPolicySet(rib::RIBDaemonProxy * ribd,
		     	     	           ISecurityManager * sm) :
		IAuthPolicySet(IAuthPolicySet::AUTH_TLSHAND)
{
	rib_daemon = ribd;
	sec_man = sm;
	timeout = DEFAULT_TIMEOUT;
}

AuthTLSHandPolicySet::~AuthTLSHandPolicySet()
{
}

cdap_rib::auth_policy_t AuthTLSHandPolicySet::get_auth_policy(int session_id,
			   	        		      const AuthSDUProtectionProfile& profile)
{
	if (profile.authPolicy.name_ != type) {
		LOG_ERR("Wrong policy name: %s, expected: %s",
				profile.authPolicy.name_.c_str(),
				type.c_str());
		throw Exception();
	}

	ScopedLock sc_lock(lock);

	if (sec_man->get_security_context(session_id) != 0) {
		LOG_ERR("A security context already exists for session_id: %d",
			session_id);
		throw Exception();
	}

	LOG_DBG("Initiating authentication for session_id: %d", session_id);
	cdap_rib::auth_policy_t auth_policy;
	auth_policy.name = IAuthPolicySet::AUTH_TLSHAND;
	auth_policy.versions.push_back(profile.authPolicy.version_);

	TLSHandSecurityContext * sc = new TLSHandSecurityContext(session_id,
								 profile);
	sc->client_random.utc_unix_time = (unsigned int) time(NULL);
	sc->client_random.random_bytes.data = new unsigned char[28];
	sc->client_random.random_bytes.length = 28;
	if (RAND_bytes(sc->client_random.random_bytes.data,
		       sc->client_random.random_bytes.length) == 0) {
		LOG_ERR("Problems generating client random bytes: %s",
		        ERR_error_string(ERR_get_error(), NULL));
		delete sc;
		throw Exception();
	}

	TLSHandAuthOptions options;
	options.cipher_suites.push_back(sc->cipher_suite);
	options.compress_methods.push_back(sc->compress_method);
	options.random = sc->client_random;

	encode_tls_hand_auth_options(options,
				     auth_policy.options);

	//Store security context
	sc->state = TLSHandSecurityContext::WAIT_SERVER_HELLO;
	sec_man->add_security_context(sc);

	return auth_policy;
}

IAuthPolicySet::AuthStatus AuthTLSHandPolicySet::initiate_authentication(const cdap_rib::auth_policy_t& auth_policy,
								         const AuthSDUProtectionProfile& profile,
								         int session_id)
{
	if (auth_policy.name != type) {
		LOG_ERR("Wrong policy name: %s", auth_policy.name.c_str());
		return IAuthPolicySet::FAILED;
	}

	if (auth_policy.versions.front() != RINA_DEFAULT_POLICY_VERSION) {
		LOG_ERR("Unsupported policy version: %s",
				auth_policy.versions.front().c_str());
		return IAuthPolicySet::FAILED;
	}

	ScopedLock sc_lock(lock);

	if (sec_man->get_security_context(session_id) != 0) {
		LOG_ERR("A security context already exists for session_id: %d", session_id);
		return IAuthPolicySet::FAILED;
	}

	LOG_DBG("Initiating authentication for session_id: %d", session_id);
	TLSHandAuthOptions options;
	decode_tls_hand_auth_options(auth_policy.options, options);

	TLSHandSecurityContext * sc;
	try {
		sc = new TLSHandSecurityContext(session_id, profile, &options);
	} catch (Exception &e){
		return IAuthPolicySet::FAILED;
	}

	//Generate server random
	sc->server_random.utc_unix_time = (unsigned int) time(NULL);
	sc->server_random.random_bytes.data = new unsigned char[28];
	sc->server_random.random_bytes.length = 28;
	if (RAND_bytes(sc->server_random.random_bytes.data,
			sc->server_random.random_bytes.length) == 0) {
		LOG_ERR("Problems generating server random bytes: %s",
				ERR_error_string(ERR_get_error(), NULL));
		delete sc;
		return IAuthPolicySet::FAILED;
	}

	//Send Server Hello
	try {
		cdap_rib::flags_t flags;
		cdap_rib::filt_info_t filt;
		cdap_rib::obj_info_t obj_info;
		cdap::StringEncoder encoder;

		obj_info.class_ = SERVER_HELLO;
		obj_info.name_ = SERVER_HELLO;
		obj_info.inst_ = 0;
		encode_server_hello_tls_hand(sc->server_random,
					     sc->cipher_suite,
					     sc->compress_method,
					     RINA_DEFAULT_POLICY_VERSION,
					     obj_info.value_);

		rib_daemon->remote_write(sc->con, obj_info, flags, filt, NULL);
	} catch (Exception &e) {
		LOG_ERR("Problems encoding and sending CDAP message: %s", e.what());
		sec_man->destroy_security_context(sc->id);
		return IAuthPolicySet::FAILED;
	}

	return IAuthPolicySet::IN_PROGRESS;
}

int AuthTLSHandPolicySet::process_incoming_message(const cdap::CDAPMessage& message,
						   int session_id)
{
	if (message.op_code_ != cdap::CDAPMessage::M_WRITE) {
		LOG_ERR("Wrong operation type");
		return IAuthPolicySet::FAILED;
	}

	if (message.obj_class_ == SERVER_HELLO) {
		return process_server_hello_message(message, session_id);
	}

	return rina::IAuthPolicySet::FAILED;
}

int AuthTLSHandPolicySet::process_server_hello_message(const cdap::CDAPMessage& message,
				 	 	       int session_id)
{
	TLSHandSecurityContext * sc;

	if (message.obj_value_.message_ == 0) {
		LOG_ERR("Null object value");
		return IAuthPolicySet::FAILED;
	}

	sc = dynamic_cast<TLSHandSecurityContext *>(sec_man->get_security_context(session_id));
	if (!sc) {
		LOG_ERR("Could not retrieve Security Context for session: %d", session_id);
		return IAuthPolicySet::FAILED;
	}

	ScopedLock sc_lock(lock);

	if (sc->state != TLSHandSecurityContext::WAIT_SERVER_HELLO) {
		LOG_ERR("Wrong session state: %d", sc->state);
		sec_man->remove_security_context(session_id);
		delete sc;
		return IAuthPolicySet::FAILED;
	}

	decode_server_hello_tls_hand(message.obj_value_,
				     sc->server_random,
				     sc->cipher_suite,
				     sc->compress_method,
				     sc->version);

	sc->state = TLSHandSecurityContext::WAIT_SERVER_CERTIFICATE;

	return IAuthPolicySet::IN_PROGRESS;
}

int AuthTLSHandPolicySet::set_policy_set_param(const std::string& name,
                         	 	       const std::string& value)
{
        LOG_DBG("No policy-set-specific parameters to set (%s, %s)",
                        name.c_str(), value.c_str());
        return -1;
}

IAuthPolicySet::AuthStatus AuthTLSHandPolicySet::crypto_state_updated(int port_id)
{
	TLSHandSecurityContext * sc;

	ScopedLock sc_lock(lock);

	sc = dynamic_cast<TLSHandSecurityContext *>(sec_man->get_security_context(port_id));
	if (!sc) {
		LOG_ERR("Could not retrieve TLS Handshake security context for port-id: %d",
			port_id);
		return IAuthPolicySet::FAILED;
	}

	//TODO
	return IAuthPolicySet::FAILED;
}

}

//
// Security Manager
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

#include <cstdlib>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/md5.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/ssl.h>

#define RINA_PREFIX "librina.security-manager"

#include "librina/logs.h"
#include "librina/security-manager.h"
#include "auth-policies.pb.h"
#include "core.h"

namespace rina {

//Class AuthPolicySet
const std::string IAuthPolicySet::AUTH_NONE = "PSOC_authentication-none";
const std::string IAuthPolicySet::AUTH_PASSWORD = "PSOC_authentication-password";
const std::string IAuthPolicySet::AUTH_SSH2 = "PSOC_authentication-ssh2";
const std::string IAuthPolicySet::AUTH_TLSHAND = "PSOC_authentication-tlshandshake";

IAuthPolicySet::IAuthPolicySet(const std::string& type_)
{
	type = type_;
}

//Class AuthNonePolicySet
cdap_rib::auth_policy_t AuthNonePolicySet::get_auth_policy(int session_id,
							   const cdap_rib::ep_info_t& peer_ap,
					                   const AuthSDUProtectionProfile& profile)
{
	(void) peer_ap;

	if (profile.authPolicy.name_ != type) {
		LOG_ERR("Wrong policy name: %s", profile.authPolicy.name_.c_str());
		throw Exception();
	}

	ISecurityContext * sc = new ISecurityContext(session_id,
						     IAuthPolicySet::AUTH_NONE);
	sc->crcPolicy = profile.crcPolicy;
	sc->ttlPolicy = profile.ttlPolicy;
	sc->con.port_id = session_id;
	sec_man->add_security_context(sc);

	cdap_rib::auth_policy_t result;
	result.name = IAuthPolicySet::AUTH_NONE;
	result.versions.push_back(profile.authPolicy.version_);
	return result;
}

rina::IAuthPolicySet::AuthStatus AuthNonePolicySet::initiate_authentication(const cdap_rib::auth_policy_t& auth_policy,
									    const AuthSDUProtectionProfile& profile,
									    const cdap_rib::ep_info_t& peer_ap,
								      	    int session_id)
{
	(void) peer_ap;

	if (auth_policy.name != type) {
		LOG_ERR("Wrong policy name: %s", auth_policy.name.c_str());
		return rina::IAuthPolicySet::FAILED;
	}

	if (auth_policy.versions.front() != RINA_DEFAULT_POLICY_VERSION) {
		LOG_ERR("Unsupported policy version: %s",
				auth_policy.versions.front().c_str());
		return rina::IAuthPolicySet::FAILED;
	}

	ISecurityContext * sc = new ISecurityContext(session_id,
						     IAuthPolicySet::AUTH_NONE);
	sc->crcPolicy = profile.crcPolicy;
	sc->ttlPolicy = profile.ttlPolicy;
	sc->con.port_id = session_id;
	sec_man->add_security_context(sc);

	return rina::IAuthPolicySet::SUCCESSFULL;
}

// No authentication messages exchanged
int AuthNonePolicySet::process_incoming_message(const cdap::CDAPMessage& message,
						int session_id)
{
	//This function should never be called for this authenticaiton policy
	return IAuthPolicySet::FAILED;
}

int AuthNonePolicySet::set_policy_set_param(const std::string& name,
                                            const std::string& value)
{
        LOG_DBG("No policy-set-specific parameters to set (%s, %s)",
                        name.c_str(), value.c_str());
        return -1;
}

IAuthPolicySet::AuthStatus AuthNonePolicySet::crypto_state_updated(int port_id)
{
	LOG_ERR("Cryptographic SDU Protection is not supported by this policy, on port_id %d",
		port_id);
	return FAILED;
}

//Class CancelAuthTimerTask
void CancelAuthTimerTask::run()
{
	ISecurityContext * sc = sec_man->remove_security_context(session_id);
	if (sc) {
		delete sc;
	}
}

// Class AuthPasswordSecurityContext
AuthPasswordSecurityContext::AuthPasswordSecurityContext(int session_id) :
		ISecurityContext(session_id, IAuthPolicySet::AUTH_PASSWORD)
{
	challenge = 0;
	timer_task = 0;
	challenge_length = 0;
}

//Class AuthPasswdPolicySet
const std::string AuthPasswordPolicySet::PASSWORD = "password";
const std::string AuthPasswordPolicySet::CIPHER = "cipher";
const std::string AuthPasswordPolicySet::CHALLENGE_LENGTH = "challenge-length";
const std::string AuthPasswordPolicySet::CHALLENGE_REQUEST = "challenge request";
const std::string AuthPasswordPolicySet::CHALLENGE_REPLY = "challenge reply";
const int AuthPasswordPolicySet::DEFAULT_TIMEOUT = 10000;

AuthPasswordPolicySet::AuthPasswordPolicySet(rib::RIBDaemonProxy * ribd, ISecurityManager * sm) :
		IAuthPolicySet(IAuthPolicySet::AUTH_PASSWORD)
{
	rib_daemon = ribd;
	sec_man = sm;
	timeout = DEFAULT_TIMEOUT;
}

// No credentials required, since the process being authenticated
// will have to demonstrate that it knows the password by encrypting
// a random challenge with a password string
cdap_rib::auth_policy_t AuthPasswordPolicySet::get_auth_policy(int session_id,
							       const cdap_rib::ep_info_t& peer_ap,
						               const AuthSDUProtectionProfile& profile)
{
	(void) peer_ap;

	if (profile.authPolicy.name_ != type) {
		LOG_ERR("Wrong policy name: %s", profile.authPolicy.name_.c_str());
		throw Exception();
	}

	AuthPasswordSecurityContext * sc = new AuthPasswordSecurityContext(session_id);
	sc->password = profile.authPolicy.get_param_value_as_string(PASSWORD);
	if (sc->password == std::string()) {
		LOG_ERR("Could not find password parameter");
		throw Exception();
	}
	sc->challenge_length = sc->password.length();
	sc->crcPolicy = profile.crcPolicy;
	sc->ttlPolicy = profile.ttlPolicy;
	sc->con.port_id = session_id;
	sc->auth_policy_name = IAuthPolicySet::AUTH_PASSWORD;
	sec_man->add_security_context(sc);

	cdap_rib::auth_policy_t result;
	result.name = IAuthPolicySet::AUTH_PASSWORD;
	result.versions.push_back(profile.authPolicy.version_);
	return result;
}

std::string * AuthPasswordPolicySet::generate_random_challenge(int length)
{
	static const char alphanum[] =
			"0123456789"
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"abcdefghijklmnopqrstuvwxyz";

	char *s = new char[length];

	for (int i = 0; i < length; ++i) {
		s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
	}

	s[length] = 0;

	std::string * result = new std::string(s);
	delete s;

	return result;
}

std::string AuthPasswordPolicySet::encrypt_challenge(const std::string& challenge,
						     const std::string password)
{
	size_t j = 0;
	char * s = new char[challenge.size()];

	//Simple XOR-based encryption, as a proof of concept
	for (size_t i=0; i<challenge.size(); i++) {
		s[i] = challenge[i] ^ password[j];
		j++;
		if (j == password.size()) {
			j = 0;
		}
	}

	std::string result = std::string(s, challenge.size());
	delete s;

	return result;
}

std::string AuthPasswordPolicySet::decrypt_challenge(const std::string& encrypted_challenge,
						     const std::string& password)
{
	return encrypt_challenge(encrypted_challenge, password);
}

rina::IAuthPolicySet::AuthStatus AuthPasswordPolicySet::initiate_authentication(const cdap_rib::auth_policy_t& auth_policy,
										const AuthSDUProtectionProfile& profile,
										const cdap_rib::ep_info_t& peer_ap,
								      	        int session_id)
{
	(void) peer_ap;

	if (auth_policy.name != type) {
		LOG_ERR("Wrong policy name: %s", auth_policy.name.c_str());
		return rina::IAuthPolicySet::FAILED;
	}

	if (auth_policy.versions.front() != RINA_DEFAULT_POLICY_VERSION) {
		LOG_ERR("Unsupported policy version: %s",
				auth_policy.versions.front().c_str());
		return rina::IAuthPolicySet::FAILED;
	}

	AuthPasswordSecurityContext * sc = new AuthPasswordSecurityContext(session_id);
	sc->password = profile.authPolicy.get_param_value_as_string(PASSWORD);
	if (sc->password == std::string()) {
		LOG_ERR("Could not find password parameter");
		throw Exception();
	}
	sc->challenge_length = sc->password.length();
	sc->crcPolicy = profile.crcPolicy;
	sc->ttlPolicy = profile.ttlPolicy;
	sc->con.port_id = session_id;
	sc->auth_policy_name = IAuthPolicySet::AUTH_PASSWORD;
	sec_man->add_security_context(sc);

	ScopedLock scopedLock(lock);

	//1 Generate a random password string and send it to the AP being authenticated
	sc->challenge = generate_random_challenge(sc->challenge_length);

	try {
		cdap_rib::flags_t flags;
		cdap_rib::filt_info_t filt;
		cdap_rib::obj_info_t obj_info;
		cdap::StringEncoder encoder;

		obj_info.class_ = CHALLENGE_REQUEST;
		obj_info.name_ = CHALLENGE_REQUEST;
		obj_info.inst_ = 0;
		encoder.encode(*(sc->challenge),
			       obj_info.value_);

		//object class contains challenge request or reply
		//object name contains cipher name
		rib_daemon->remote_write(sc->con,
					 obj_info,
					 flags,
					 filt,
					 NULL);
	} catch (Exception &e) {
		LOG_ERR("Problems encoding and sending CDAP message: %s", e.what());
	}

	//2 set timer to clean up pending authentication session upon timer expiry
	sc->timer_task = new CancelAuthTimerTask(sec_man, session_id);
	timer.scheduleTask(sc->timer_task, timeout);

	return rina::IAuthPolicySet::IN_PROGRESS;
}

int AuthPasswordPolicySet::process_challenge_request(const std::string& challenge,
						     int session_id)
{
	ScopedLock scopedLock(lock);

	AuthPasswordSecurityContext * sc =
			dynamic_cast<AuthPasswordSecurityContext *>(sec_man->get_security_context(session_id));
	if (!sc) {
		LOG_ERR("Could not find pending security context for session_id %d",
			session_id);
		return IAuthPolicySet::FAILED;
	}

	try {
		cdap_rib::flags_t flags;
		cdap_rib::filt_info_t filt;
		cdap_rib::obj_info_t obj_info;
		cdap::StringEncoder encoder;

		obj_info.class_ = CHALLENGE_REPLY;
		obj_info.name_ = CHALLENGE_REPLY;
		obj_info.inst_ = 0;
		encoder.encode(encrypt_challenge(challenge, sc->password),
			       obj_info.value_);

		//object class contains challenge request or reply
		//object name contains cipher name
		rib_daemon->remote_write(sc->con,
					 obj_info,
					 flags,
					 filt,
					 NULL);
	} catch (Exception &e) {
		LOG_ERR("Problems encoding and sending CDAP message: %s", e.what());
	}

	return IAuthPolicySet::IN_PROGRESS;
}

int AuthPasswordPolicySet::process_challenge_reply(const std::string& encrypted_challenge,
						   int session_id)
{
	int result = IAuthPolicySet::FAILED;

	ScopedLock scopedLock(lock);

	AuthPasswordSecurityContext * sc =
			dynamic_cast<AuthPasswordSecurityContext *>(sec_man->get_security_context(session_id));
	if (!sc) {
		LOG_ERR("Could not find pending security context for session_id %d",
			session_id);
		return IAuthPolicySet::FAILED;
	}

	timer.cancelTask(sc->timer_task);

	std::string recovered_challenge = decrypt_challenge(encrypted_challenge,
							    sc->password);
	if (*(sc->challenge) == recovered_challenge) {
		result = IAuthPolicySet::SUCCESSFULL;
	} else {
		LOG_DBG("Authentication failed");
	}

	return result;
}

int AuthPasswordPolicySet::process_incoming_message(const cdap::CDAPMessage& message,
						     int session_id)
{
	cdap::StringEncoder encoder;
	std::string challenge;

	if (message.op_code_ != cdap::CDAPMessage::M_WRITE) {
		LOG_ERR("Wrong operation type");
		return IAuthPolicySet::FAILED;
	}

	if (message.obj_value_.message_ == 0) {
		LOG_ERR("Null object value");
		return IAuthPolicySet::FAILED;
	}

	if (!message.obj_value_.message_) {
		LOG_ERR("Object value of wrong type");
		return IAuthPolicySet::FAILED;
	}

	try {
		encoder.decode(message.obj_value_, challenge);
	} catch  (Exception &e) {
		LOG_ERR("Error decoding challenge: %s", e.what());
		return IAuthPolicySet::FAILED;
	}

	if (message.obj_class_ == CHALLENGE_REQUEST) {
		return process_challenge_request(challenge,
						 session_id);
	}

	if (message.obj_class_ == CHALLENGE_REPLY) {
		return process_challenge_reply(challenge,
					       session_id);
	}

	LOG_ERR("Wrong message type");
	return IAuthPolicySet::FAILED;
}

// Allow modification of password via set_policy_set param
int AuthPasswordPolicySet::set_policy_set_param(const std::string& name,
                                            	const std::string& value)
{
        LOG_DBG("Unknown policy-set-specific parameters to set (%s, %s)",
                        name.c_str(), value.c_str());
        return -1;
}

IAuthPolicySet::AuthStatus AuthPasswordPolicySet::crypto_state_updated(int port_id)
{
	LOG_ERR("Encryption is not supported by this policy, on port_id %d", port_id);
	return IAuthPolicySet::FAILED;
}

//AuthSSH2Options encoder and decoder operations
void decode_ssh2_auth_options(const ser_obj_t &message, SSH2AuthOptions &options)
{
	rina::auth::policies::googleprotobuf::authOptsSSH2_t gpb_options;

	gpb_options.ParseFromArray(message.message_, message.size_);

	for(int i=0; i<gpb_options.key_exch_algs_size(); i++) {
		options.key_exch_algs.push_back(gpb_options.key_exch_algs(i));
	}

	for(int i=0; i<gpb_options.encrypt_algs_size(); i++) {
		options.encrypt_algs.push_back(gpb_options.encrypt_algs(i));
	}

	for(int i=0; i<gpb_options.mac_algs_size(); i++) {
		options.mac_algs.push_back(gpb_options.mac_algs(i));
	}

	for(int i=0; i<gpb_options.compress_algs_size(); i++) {
		options.compress_algs.push_back(gpb_options.compress_algs(i));
	}

	if (gpb_options.has_dh_public_key()) {
		  options.dh_public_key.data =
				  new unsigned char[gpb_options.dh_public_key().size()];
		  memcpy(options.dh_public_key.data,
			 gpb_options.dh_public_key().data(),
			 gpb_options.dh_public_key().size());
		  options.dh_public_key.length = gpb_options.dh_public_key().size();
	}
}

void encode_ssh2_auth_options(const SSH2AuthOptions& options,
			      ser_obj_t& result)
{
	rina::auth::policies::googleprotobuf::authOptsSSH2_t gpb_options;

	for(std::list<std::string>::const_iterator it = options.key_exch_algs.begin();
			it != options.key_exch_algs.end(); ++it) {
		gpb_options.add_key_exch_algs(*it);
	}

	for(std::list<std::string>::const_iterator it = options.encrypt_algs.begin();
			it != options.encrypt_algs.end(); ++it) {
		gpb_options.add_encrypt_algs(*it);
	}

	for(std::list<std::string>::const_iterator it = options.mac_algs.begin();
			it != options.mac_algs.end(); ++it) {
		gpb_options.add_mac_algs(*it);
	}

	for(std::list<std::string>::const_iterator it = options.compress_algs.begin();
			it != options.compress_algs.end(); ++it) {
		gpb_options.add_compress_algs(*it);
	}

	if (options.dh_public_key.length > 0) {
		gpb_options.set_dh_public_key(options.dh_public_key.data,
					      options.dh_public_key.length);
	}

	int size = gpb_options.ByteSize();
	result.message_ = new unsigned char[size];
	result.size_ = size;
	gpb_options.SerializeToArray(result.message_, size);
}

void encode_client_chall_reply_ssh2(const UcharArray& client_chall_reply,
				    const UcharArray& server_chall,
				    ser_obj_t& result)
{
	rina::auth::policies::googleprotobuf::clientChallReplySSH2_t gpb_chall;


	if (client_chall_reply.length > 0) {
		gpb_chall.set_client_challenge_rep(client_chall_reply.data,
						   client_chall_reply.length);
	}

	if (server_chall.length > 0) {
		gpb_chall.set_server_challenge(server_chall.data,
					       server_chall.length);
	}

	int size = gpb_chall.ByteSize();
	result.message_ = new unsigned char[size];
	result.size_ = size;
	gpb_chall.SerializeToArray(result.message_ , size);
}

//AuthSSH2Options encoder and decoder operations
void decode_client_chall_reply_ssh2(const ser_obj_t &message,
				    UcharArray& client_chall_reply,
				    UcharArray& server_chall)
{
	rina::auth::policies::googleprotobuf::clientChallReplySSH2_t gpb_chall;

	gpb_chall.ParseFromArray(message.message_, message.size_);

	if (gpb_chall.has_client_challenge_rep()) {
		client_chall_reply.data =
				  new unsigned char[gpb_chall.client_challenge_rep().size()];
		memcpy(client_chall_reply.data,
		       gpb_chall.client_challenge_rep().data(),
		       gpb_chall.client_challenge_rep().size());
		client_chall_reply.length = gpb_chall.client_challenge_rep().size();
	}

	if (gpb_chall.has_server_challenge()) {
		server_chall.data =
				  new unsigned char[gpb_chall.server_challenge().size()];
		memcpy(server_chall.data,
		       gpb_chall.server_challenge().data(),
		       gpb_chall.server_challenge().size());
		server_chall.length = gpb_chall.server_challenge().size();
	}
}

bool CryptoState::operator==(const CryptoState &other) const
{
	return other.compress_alg == compress_alg &&
			other.enable_crypto_rx == enable_crypto_rx &&
			other.enable_crypto_tx == enable_crypto_tx &&
			other.encrypt_alg == encrypt_alg &&
			other.encrypt_key_rx.length == encrypt_key_rx.length &&
			other.encrypt_key_tx.length == encrypt_key_tx.length &&
			other.iv_rx.length == iv_rx.length &&
			other.iv_tx.length == iv_tx.length &&
			other.mac_alg == mac_alg &&
			other.mac_key_rx.length == mac_key_rx.length &&
			other.mac_key_tx.length == mac_key_tx.length;
}

bool CryptoState::operator!=(const CryptoState &other) const
{
	return !(*this == other);
}

void CryptoState::from_c_crypto_state(CryptoState & cs,
				      struct sdup_crypto_state * ccs)
{
	if (ccs->compress_alg)
		cs.compress_alg = ccs->compress_alg;
	if (ccs->mac_alg)
		cs.mac_alg = ccs->mac_alg;
	if (ccs->enc_alg)
		cs.encrypt_alg = ccs->enc_alg;

	cs.enable_crypto_rx = ccs->enable_crypto_rx;
	cs.enable_crypto_tx = ccs->enable_crypto_tx;

	if (ccs->encrypt_key_rx) {
		cs.encrypt_key_rx.length = ccs->encrypt_key_rx->size;
		cs.encrypt_key_rx.data = new unsigned char[cs.encrypt_key_rx.length];
		memcpy(cs.encrypt_key_rx.data, ccs->encrypt_key_rx->data,
		       cs.encrypt_key_rx.length);
	}

	if (ccs->encrypt_key_tx) {
		cs.encrypt_key_tx.length = ccs->encrypt_key_tx->size;
		cs.encrypt_key_tx.data = new unsigned char[cs.encrypt_key_tx.length];
		memcpy(cs.encrypt_key_tx.data, ccs->encrypt_key_tx->data,
		       cs.encrypt_key_tx.length);
	}

	if (ccs->iv_rx) {
		cs.iv_rx.length = ccs->iv_rx->size;
		cs.iv_rx.data = new unsigned char[cs.iv_rx.length];
		memcpy(cs.iv_rx.data, ccs->iv_rx->data,
		       cs.iv_rx.length);
	}

	if (ccs->iv_tx) {
		cs.iv_tx.length = ccs->iv_tx->size;
		cs.iv_tx.data = new unsigned char[cs.iv_tx.length];
		memcpy(cs.iv_tx.data, ccs->iv_tx->data,
		       cs.iv_tx.length);
	}

	if (ccs->mac_key_rx) {
		cs.mac_key_rx.length = ccs->mac_key_rx->size;
		cs.mac_key_rx.data = new unsigned char[cs.mac_key_rx.length];
		memcpy(cs.mac_key_rx.data, ccs->mac_key_rx->data,
		       cs.mac_key_rx.length);
	}

	if (ccs->mac_key_tx) {
		cs.mac_key_tx.length = ccs->mac_key_tx->size;
		cs.mac_key_tx.data = new unsigned char[cs.mac_key_tx.length];
		memcpy(cs.mac_key_tx.data, ccs->mac_key_tx->data,
		       cs.mac_key_tx.length);
	}
}

// Class crypto state
struct sdup_crypto_state * CryptoState::to_c_crypto_state() const
{
	struct sdup_crypto_state * result;

	result = new sdup_crypto_state();
	result->enable_crypto_rx = enable_crypto_rx;
	result->enable_crypto_tx = enable_crypto_tx;
	result->port_id = port_id;
	result->compress_alg = stringToCharArray(compress_alg);
	result->enc_alg = stringToCharArray(encrypt_alg);
	result->mac_alg = stringToCharArray(mac_alg);

	result->encrypt_key_rx = new buffer();
	result->encrypt_key_rx->size = encrypt_key_rx.length;
	if (encrypt_key_rx.length > 0) {
		result->encrypt_key_rx->data = new unsigned char[encrypt_key_rx.length];
		memcpy(result->encrypt_key_rx->data, encrypt_key_rx.data,
				encrypt_key_rx.length);
	}

	result->encrypt_key_tx = new buffer();
	result->encrypt_key_tx->size = encrypt_key_tx.length;
	if (encrypt_key_tx.length > 0) {
		result->encrypt_key_tx->data = new unsigned char[encrypt_key_tx.length];
		memcpy(result->encrypt_key_tx->data, encrypt_key_tx.data,
				encrypt_key_tx.length);
	}

	result->mac_key_rx = new buffer();
	result->mac_key_rx->size = mac_key_rx.length;
	if (mac_key_rx.length > 0) {
		result->mac_key_rx->data = new unsigned char[mac_key_rx.length];
		memcpy(result->mac_key_rx->data, mac_key_rx.data,
				mac_key_rx.length);
	}

	result->mac_key_tx = new buffer();
	result->mac_key_tx->size = mac_key_tx.length;
	if (mac_key_tx.length > 0) {
		result->mac_key_tx->data = new unsigned char[mac_key_tx.length];
		memcpy(result->mac_key_tx->data, mac_key_tx.data,
				mac_key_tx.length);
	}

	result->iv_rx = new buffer();
	result->iv_rx->size = iv_rx.length;
	if (iv_rx.length > 0) {
		result->iv_rx->data = new unsigned char[iv_rx.length];
		memcpy(result->iv_rx->data, iv_rx.data,
				iv_rx.length);
	}

	result->iv_tx = new buffer();
	result->iv_tx->size = iv_tx.length;
	if (iv_tx.length > 0) {
		result->iv_tx->data = new unsigned char[iv_tx.length];
		memcpy(result->iv_tx->data, iv_tx.data,
				iv_tx.length);
	}

	return result;
}

std::string CryptoState::toString(void) const
{
	std::stringstream ss;

	ss << "Enable crypto rx: " << enable_crypto_rx
	   << "; enable crypto tx: " << enable_crypto_tx << std::endl;
	ss << "Encrypt alg: " << encrypt_alg << "; Compress alg: "
	   << compress_alg << "; MAC alg: " << mac_alg << std::endl;
	return ss.str();
}

// Class SSH2SecurityContext
const std::string SSH2SecurityContext::KEY_EXCHANGE_ALGORITHM = "keyExchangeAlg";
const std::string SSH2SecurityContext::ENCRYPTION_ALGORITHM = "encryptAlg";
const std::string SSH2SecurityContext::MAC_ALGORITHM = "macAlg";
const std::string SSH2SecurityContext::COMPRESSION_ALGORITHM = "compressAlg";
const std::string SSH2SecurityContext::KEYSTORE_PATH = "keystore";
const std::string SSH2SecurityContext::KEYSTORE_PASSWORD = "keystorePass";
const std::string SSH2SecurityContext::KEY = "key";
const std::string SSH2SecurityContext::PUBLIC_KEY = "public_key";
const std::string SSH2SecurityContext::KNOWN_IPCPS = "known_ipcps";

SSH2SecurityContext::~SSH2SecurityContext()
{
	if (dh_state) {
		DH_free(dh_state);
		dh_state = NULL;
	}

	if (dh_peer_pub_key) {
		BN_free(dh_peer_pub_key);
		dh_peer_pub_key = NULL;
	}

	if (auth_keypair) {
		RSA_free(auth_keypair);
		auth_keypair = NULL;
	}

	if (auth_peer_pub_key) {
		RSA_free(auth_peer_pub_key);
		auth_peer_pub_key = NULL;
	}
}

CryptoState SSH2SecurityContext::get_crypto_state(bool enable_crypto_tx,
						  bool enable_crypto_rx,
						  bool isserver)
{
	CryptoState result;

	result.encrypt_alg = encrypt_alg;
	result.mac_alg = mac_alg;
	result.compress_alg = compress_alg;

	result.enable_crypto_tx = enable_crypto_tx;
	result.enable_crypto_rx = enable_crypto_rx;
	result.port_id = id;
	if (isserver) {
		result.encrypt_key_rx = encrypt_key_client;
		result.encrypt_key_tx = encrypt_key_server;
		result.mac_key_rx = mac_key_client;
		result.mac_key_tx = mac_key_server;
	} else {
		result.encrypt_key_tx = encrypt_key_client;
		result.encrypt_key_rx = encrypt_key_server;
		result.mac_key_tx = mac_key_client;
		result.mac_key_rx = mac_key_server;
	}

	return result;
}

SSH2SecurityContext::SSH2SecurityContext(int session_id,
		 	   	         const std::string & peer_app_name,
				   	 const AuthSDUProtectionProfile& profile)
		: ISecurityContext(session_id, IAuthPolicySet::AUTH_SSH2)
{
	key_exch_alg = profile.authPolicy.get_param_value_as_string(KEY_EXCHANGE_ALGORITHM);
	encrypt_alg = profile.encryptPolicy.get_param_value_as_string(ENCRYPTION_ALGORITHM);
	mac_alg = profile.encryptPolicy.get_param_value_as_string(MAC_ALGORITHM);
	compress_alg = profile.encryptPolicy.get_param_value_as_string(COMPRESSION_ALGORITHM);
	keystore_path = profile.authPolicy.get_param_value_as_string(KEYSTORE_PATH);
	if (keystore_path == std::string()) {
		//TODO set the configuration directory as the default keystore path
	}
	keystore_password = profile.authPolicy.get_param_value_as_string(KEYSTORE_PASSWORD);
	crcPolicy = profile.crcPolicy;
	ttlPolicy = profile.ttlPolicy;
	encrypt_policy_config = profile.encryptPolicy;
	con.port_id = session_id;
	peer_ap_name = peer_app_name;
	crypto_rx_enabled = false;
	crypto_tx_enabled = false;

	dh_peer_pub_key = NULL;
	dh_state = NULL;
	auth_keypair = NULL;
	auth_peer_pub_key = NULL;
	timer_task = NULL;

	state = BEGIN;
}

SSH2SecurityContext::SSH2SecurityContext(int session_id,
					 const std::string& peer_app_name,
					 const AuthSDUProtectionProfile& profile,
					 SSH2AuthOptions * options)
		: ISecurityContext(session_id, IAuthPolicySet::AUTH_SSH2)
{
	std::string option = options->key_exch_algs.front();
	if (option != SSL_TXT_EDH) {
		LOG_ERR("Unsupported key exchange algorithm: %s",
			option.c_str());
		throw Exception();
	} else {
		key_exch_alg = option;
	}

	option = options->encrypt_algs.front();
	if (option != SSL_TXT_AES128 && option != SSL_TXT_AES256) {
		LOG_ERR("Unsupported encryption algorithm: %s",
			option.c_str());
		throw Exception();
	} else {
		encrypt_alg = option;
	}

	option = options->mac_algs.front();
	if (option != SSL_TXT_MD5 &&
	    option != SSL_TXT_SHA1 &&
	    option != SSL_TXT_SHA256) {
		LOG_ERR("Unsupported MAC algorithm: %s",
			option.c_str());
		throw Exception();
	} else {
		mac_alg = option;
	}

	option = options->compress_algs.front();
	if (option != "deflate"){
		LOG_ERR("Unsupported compression algorithm: %s",
			option.c_str());
		throw Exception();
	} else {
		compress_alg = option;
	}

	keystore_path = profile.authPolicy.get_param_value_as_string(KEYSTORE_PATH);
	if (keystore_path == std::string()) {
		//TODO set the configuration directory as the default keystore path
	}
	keystore_password = profile.authPolicy.get_param_value_as_string(KEYSTORE_PASSWORD);
	crcPolicy = profile.crcPolicy;
	ttlPolicy = profile.ttlPolicy;
	encrypt_policy_config = profile.encryptPolicy;
	con.port_id = session_id;
	peer_ap_name = peer_app_name;
	crypto_rx_enabled = false;
	crypto_tx_enabled = false;

	dh_peer_pub_key = NULL;
	dh_state = NULL;
	auth_keypair = NULL;
	auth_peer_pub_key = NULL;
	timer_task = NULL;

	state = BEGIN;
}

//OpenSSL 1.1 compatibility layer
#if OPENSSL_VERSION_NUMBER < 0x10100000L
int DH_set0_pqg(DH *dh, BIGNUM *p, BIGNUM *q, BIGNUM *g)
{
	dh->p = p;
	dh->q = q;
	dh->g = g;

	return 1;
}

void DH_get0_key(const DH *dh, const BIGNUM **pub_key, const BIGNUM **priv_key)
{
	if (pub_key)
		*pub_key = dh->pub_key;

	if (priv_key)
		*priv_key = dh->priv_key;
}

void DH_get0_pqg(const DH *dh, const BIGNUM **p,
		 const BIGNUM **q, const BIGNUM **g)
{
	if (p)
		*p = dh->p;

	if (q)
		*q = dh->q;

	if (g)
		*g = dh->g;
}
#endif

//Class AuthSSH2
const int AuthSSH2PolicySet::DEFAULT_TIMEOUT = 10000;
const std::string AuthSSH2PolicySet::EDH_EXCHANGE = "Ephemeral Diffie-Hellman exchange";
const int AuthSSH2PolicySet::MIN_RSA_KEY_PAIR_LENGTH = 256;
const std::string AuthSSH2PolicySet::CLIENT_CHALLENGE = "Client challenge";
const std::string AuthSSH2PolicySet::CLIENT_CHALLENGE_REPLY = "Client challenge reply and server challenge";
const std::string AuthSSH2PolicySet::SERVER_CHALLENGE_REPLY = "Server challenge reply";

AuthSSH2PolicySet::AuthSSH2PolicySet(rib::RIBDaemonProxy * ribd, ISecurityManager * sm) :
		IAuthPolicySet(IAuthPolicySet::AUTH_SSH2)
{
	rib_daemon = ribd;
	sec_man = sm;
	timeout = DEFAULT_TIMEOUT;
	dh_parameters = 0;

	//Generate G and P parameters in a separate thread (takes a bit of time)
	edh_init_params();
	if (!dh_parameters) {
		LOG_ERR("Error initializing DH parameters");
	}
}

AuthSSH2PolicySet::~AuthSSH2PolicySet()
{
	if (dh_parameters) {
		DH_free(dh_parameters);
		dh_parameters = NULL;
	}
}

void AuthSSH2PolicySet::edh_init_params()
{
	int codes;
	BIGNUM *p, *g;

	static unsigned char dh2048_p[]={
		0xC4,0x25,0x37,0x63,0x56,0x46,0xDA,0x97,0x3A,0x51,0x98,0xA1,
		0xD1,0xA1,0xD0,0xA0,0x78,0x58,0x64,0x31,0x74,0x6D,0x1D,0x85,
		0x25,0x38,0x3E,0x0C,0x88,0x1F,0xFF,0x07,0x5E,0x73,0xFF,0x16,
		0x52,0x22,0x45,0xC0,0x1B,0xBA,0xC9,0x8E,0x84,0x92,0x90,0x42,
		0x32,0x88,0xF7,0x94,0x0B,0xB2,0x03,0xF1,0x15,0xA1,0xD0,0x31,
		0x49,0x44,0xFD,0xA0,0x46,0x11,0x06,0x38,0x6F,0x06,0x2F,0xBB,
		0xA9,0x0B,0xB1,0xC8,0xB5,0x8F,0xFE,0x7A,0x7F,0x4E,0x94,0x19,
		0xCE,0x7A,0x1A,0xA9,0xB5,0xE8,0x9F,0x05,0x19,0x2D,0x39,0x26,
		0xF5,0xC6,0x3A,0x80,0xC0,0xCA,0xE3,0x66,0x22,0x12,0x1C,0x46,
		0xAC,0x46,0x6F,0x2C,0x36,0x29,0x1C,0x6B,0xFD,0x35,0xFA,0x90,
		0x87,0x75,0x90,0xA8,0x32,0x1B,0xFE,0x2F,0x32,0x9D,0x62,0x91,
		0x3A,0x1A,0x8B,0xEC,0xDB,0xB5,0x26,0x74,0x7E,0xE3,0x7A,0xA6,
		0x5C,0xBA,0xEA,0xCF,0x68,0x95,0x04,0x96,0xB9,0x0F,0x68,0x7D,
		0x3F,0xC6,0x2E,0xA1,0xBA,0x10,0x8E,0x83,0x3C,0x52,0x50,0x30,
		0xDC,0x0A,0x5D,0x95,0x67,0x27,0x64,0x00,0x9A,0x18,0x13,0x86,
		0xC9,0xC9,0xAD,0x4B,0x4E,0x77,0x9F,0x92,0xFD,0x0E,0x41,0xDB,
		0x15,0xEE,0x00,0x6F,0xA7,0xDF,0x89,0xEC,0xD4,0x33,0x14,0xA5,
		0x57,0xA1,0x99,0x0F,0x59,0x4C,0x15,0x8B,0x17,0x8D,0xC1,0x1A,
		0x2E,0x70,0xD0,0x8E,0x0B,0x07,0x57,0xB8,0xB1,0x87,0xB9,0x03,
		0x97,0x70,0x69,0x95,0x0D,0x8C,0x2E,0x4E,0xC1,0x2E,0x47,0x1F,
		0x59,0xDB,0xB1,0x82,0x37,0x06,0xA9,0x99,0xC1,0x77,0x39,0x1C,
		0x1A,0xC0,0xA7,0xB3,
		};
	static unsigned char dh2048_g[]={
		0x02,
		};

	if ((dh_parameters = DH_new()) == NULL) {
		LOG_ERR("Error initializing Diffie-Hellman state");
		return;
	}

	p = BN_bin2bn(dh2048_p,sizeof(dh2048_p),NULL);
	if (p == NULL) {
		LOG_ERR("Problems converting P to big number");
		DH_free(dh_parameters);
		return;
	}

	g = BN_bin2bn(dh2048_g,sizeof(dh2048_g),NULL);
	if (g == NULL) {
		LOG_ERR("Problems converting G to big number");
		DH_free(dh_parameters);
		BN_free(p);
		return;
	}

	if (DH_set0_pqg(dh_parameters, p, NULL, g) != 1) {
		LOG_ERR("Problems setting P and G");
		DH_free(dh_parameters);
		BN_free(p);
		BN_free(g);
		return;
	}

	if (DH_check(dh_parameters, &codes) != 1) {
		LOG_ERR("Error checking parameters");
		DH_free(dh_parameters);
		return;
	}

	if (codes != 0)
	{
		LOG_ERR("Diffie-Hellman check has failed");
		DH_free(dh_parameters);
		return;
	}
}

unsigned char * AuthSSH2PolicySet::BN_to_binary(BIGNUM *b, int *len)
{
	unsigned char *ret;

	int size = BN_num_bytes(b);
	ret = (unsigned char*)calloc(size, sizeof(unsigned char));
	*len = BN_bn2bin(b, ret);

	return ret;
}

cdap_rib::auth_policy_t AuthSSH2PolicySet::get_auth_policy(int session_id,
							   const cdap_rib::ep_info_t& peer_ap,
					            	   const AuthSDUProtectionProfile& profile)
{
	const BIGNUM *pub_key;

	if (profile.authPolicy.name_ != type) {
		LOG_ERR("Wrong policy name: %s, expected: %s",
				profile.authPolicy.name_.c_str(),
				type.c_str());
		throw Exception();
	}

	ScopedLock sc_lock(lock);

	if (sec_man->get_security_context(session_id) != 0) {
		LOG_ERR("A security context already exists for session_id: %d", session_id);
		throw Exception();
	}

	LOG_DBG("Initiating authentication for session_id: %d", session_id);
	cdap_rib::auth_policy_t auth_policy;
	auth_policy.name = IAuthPolicySet::AUTH_SSH2;
	auth_policy.versions.push_back(profile.authPolicy.version_);

	SSH2SecurityContext * sc = new SSH2SecurityContext(session_id,
							   peer_ap.ap_name_,
							   profile);

	//Load authentication keys from the keystore
	if (load_authentication_keys(sc) != 0) {
		delete sc;
		throw Exception();
	}

	//Initialize Diffie-Hellman machinery and generate private/public key pairs
	if (edh_init_keys(sc) != 0) {
		delete sc;
		throw Exception();
	}

	SSH2AuthOptions options;
	options.key_exch_algs.push_back(sc->key_exch_alg);
	options.encrypt_algs.push_back(sc->encrypt_alg);
	options.mac_algs.push_back(sc->mac_alg);
	options.compress_algs.push_back(sc->compress_alg);
	DH_get0_key(sc->dh_state, &pub_key, NULL);
	options.dh_public_key.length = BN_num_bytes(pub_key);
	options.dh_public_key.data = new unsigned char[options.dh_public_key.length];
	if (BN_bn2bin(pub_key, options.dh_public_key.data) <= 0 ) {
		LOG_ERR("Error transforming big number to binary");
		delete sc;
		throw Exception();
	}

	encode_ssh2_auth_options(options,
				 auth_policy.options);

	//Store security context
	sc->state = SSH2SecurityContext::WAIT_EDH_EXCHANGE;
	sec_man->add_security_context(sc);

	return auth_policy;
}

int AuthSSH2PolicySet::load_authentication_keys(SSH2SecurityContext * sc)
{
	BIO * keystore;
	std::stringstream ss;

	ss << sc->keystore_path << "/" << SSH2SecurityContext::KEY;
	keystore =  BIO_new_file(ss.str().c_str(), "r");
	if (!keystore) {
		LOG_ERR("Problems opening keystore file at: %s",
			ss.str().c_str());
		return -1;
	}

	//TODO fix problems with reading private keys from encrypted repos
	//we should use sc->keystore_pass.c_str() as the last argument
	sc->auth_keypair = PEM_read_bio_RSAPrivateKey(keystore, NULL, 0, NULL);
	ss.str(std::string());
	ss.clear();
	BIO_free(keystore);

	if (!sc->auth_keypair) {
		LOG_ERR("Problems reading RSA key pair from keystore: %s",
			ERR_error_string(ERR_get_error(), NULL));
		return -1;
	}

	//Read peer public key from keystore
	ss << sc->keystore_path << "/" << sc->peer_ap_name;
	keystore =  BIO_new_file(ss.str().c_str(), "r");
	if (!keystore) {
		LOG_ERR("Problems opening keystore file at: %s",
			ss.str().c_str());
		return -1;
	}

	//TODO fix problems with reading private keys from encrypted repos
	//we should use sc->keystore_pass.c_str() as the last argument
	sc->auth_peer_pub_key = PEM_read_bio_RSA_PUBKEY(keystore, NULL, 0, NULL);
	BIO_free(keystore);

	if (!sc->auth_peer_pub_key) {
		LOG_ERR("Problems reading RSA public key from keystore: %s",
			ERR_error_string(ERR_get_error(), NULL));
		return -1;
	}

	int rsa_size = RSA_size(sc->auth_keypair);
	if (rsa_size < MIN_RSA_KEY_PAIR_LENGTH) {
		LOG_ERR("RSA keypair size is too low. Minimum: %d, actual: %d",
				MIN_RSA_KEY_PAIR_LENGTH, sc->challenge.length);
		RSA_free(sc->auth_keypair);
		return -1;
	}

	//Since we'll use RSA encryption with RSA_PKCS1_OAEP_PADDING padding, the
	//maximum length of the data to be encrypted must be less than RSA_size(rsa) - 41
	sc->challenge.length = rsa_size - 42;

	LOG_DBG("Read RSA keys from keystore");
	return 0;
}

int AuthSSH2PolicySet::edh_init_keys(SSH2SecurityContext * sc)
{
	DH *dh_state;
	const BIGNUM *p, *g;

	// Init own parameters
	if (!dh_parameters) {
		LOG_ERR("Diffie-Hellman parameters not yet initialized");
		return -1;
	}

	if ((dh_state = DH_new()) == NULL) {
		LOG_ERR("Error initializing Diffie-Hellman state");
		return -1;
	}

	// Set P and G (re-use defaults or use the ones sent by the peer)
	DH_get0_pqg(dh_parameters, &p, NULL, &g);
	DH_set0_pqg(dh_state, BN_dup(p), NULL, BN_dup(g));

	// Generate the public and private key pair
	if (DH_generate_key(dh_state) != 1) {
		LOG_ERR("Error generating public and private key pair: %s",
			ERR_error_string(ERR_get_error(), NULL));
		DH_free(dh_state);
		return -1;
	}

	sc->dh_state = dh_state;

	return 0;
}

IAuthPolicySet::AuthStatus AuthSSH2PolicySet::initiate_authentication(const cdap_rib::auth_policy_t& auth_policy,
								      const AuthSDUProtectionProfile& profile,
								      const cdap_rib::ep_info_t& peer_ap,
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
	SSH2AuthOptions options;
	decode_ssh2_auth_options(auth_policy.options, options);

	SSH2SecurityContext * sc;
	try {
		sc = new SSH2SecurityContext(session_id,
					     peer_ap.ap_name_,
					     profile,
					     &options);
	} catch (Exception &e){
		return IAuthPolicySet::FAILED;
	}

	//Load authentication keys from the keystore
	if (load_authentication_keys(sc) != 0) {
		delete sc;
		return IAuthPolicySet::FAILED;
	}

	//Initialize Diffie-Hellman machinery and generate private/public key pairs
	if (edh_init_keys(sc) != 0) {
		delete sc;
		return IAuthPolicySet::FAILED;
	}

	//Add peer public key to security context
	sc->dh_peer_pub_key = BN_bin2bn(options.dh_public_key.data,
			       	        options.dh_public_key.length,
			       	        NULL);
	if (!sc->dh_peer_pub_key) {
		LOG_ERR("Error converting public key to a BIGNUM");
		delete sc;
		return IAuthPolicySet::FAILED;
	}

	//Generate the shared secret
	if (edh_generate_shared_secret(sc) != 0) {
		delete sc;
		return IAuthPolicySet::FAILED;
	}

	// Configure kernel SDU protection policy with shared secret and algorithms
	// tell it to enable decryption
	AuthStatus result = sec_man->update_crypto_state(sc->get_crypto_state(false, true, true),
						         this);
	if (result == IAuthPolicySet::FAILED) {
		delete sc;
		return result;
	}

	sec_man->add_security_context(sc);
	sc->state = SSH2SecurityContext::REQUESTED_ENABLE_DECRYPTION_SERVER;
	if (result == IAuthPolicySet::SUCCESSFULL) {
		result = decryption_enabled_server(sc);
	}
	return result;
}

int AuthSSH2PolicySet::edh_generate_shared_secret(SSH2SecurityContext * sc)
{
	if((sc->shared_secret.data =
			(unsigned char*) OPENSSL_malloc(sizeof(unsigned char) * (DH_size(sc->dh_state)))) == NULL) {
		LOG_ERR("Error allocating memory for shared secret");
		return -1;
	}

	if((sc->shared_secret.length =
			DH_compute_key(sc->shared_secret.data, sc->dh_peer_pub_key, sc->dh_state)) < 0) {
		LOG_ERR("Error computing shared secret: %s",
			ERR_error_string(ERR_get_error(), NULL));
		return -1;
	}

	std::string hash_base;

	if (sc->encrypt_alg == SSL_TXT_AES128) {
		sc->encrypt_key_client.length = 16;
		sc->encrypt_key_client.data = new unsigned char[16];
		sc->encrypt_key_server.length = 16;
		sc->encrypt_key_server.data = new unsigned char[16];

		hash_base = std::string((const char *)sc->shared_secret.data, sc->shared_secret.length) + "A";
		MD5((const unsigned char*)hash_base.c_str(), hash_base.length(), sc->encrypt_key_client.data);

		hash_base = std::string((const char *)sc->shared_secret.data, sc->shared_secret.length) + "B";
		MD5((const unsigned char*)hash_base.c_str(), hash_base.length(), sc->encrypt_key_server.data);
	} else if (sc->encrypt_alg == SSL_TXT_AES256) {
		sc->encrypt_key_client.length = 32;
		sc->encrypt_key_client.data = new unsigned char[32];
		sc->encrypt_key_server.length = 32;
		sc->encrypt_key_server.data = new unsigned char[32];

		hash_base = std::string((const char *)sc->shared_secret.data, sc->shared_secret.length) + "A";
		SHA256((const unsigned char*)hash_base.c_str(), hash_base.length(), sc->encrypt_key_client.data);

		hash_base = std::string((const char *)sc->shared_secret.data, sc->shared_secret.length) + "B";
		SHA256((const unsigned char*)hash_base.c_str(), hash_base.length(), sc->encrypt_key_server.data);
	}

	if (sc->mac_alg == SSL_TXT_MD5) {
		sc->mac_key_client.length = 16;
		sc->mac_key_client.data = new unsigned char[16];
		sc->mac_key_server.length = 16;
		sc->mac_key_server.data = new unsigned char[16];

		hash_base = std::string((const char *)sc->shared_secret.data, sc->shared_secret.length) + "C";
		MD5((const unsigned char*)hash_base.c_str(), hash_base.length(), sc->mac_key_client.data);

		hash_base = std::string((const char *)sc->shared_secret.data, sc->shared_secret.length) + "D";
		MD5((const unsigned char*)hash_base.c_str(), hash_base.length(), sc->mac_key_server.data);
	} else if (sc->mac_alg == SSL_TXT_SHA1){
		sc->mac_key_client.length = 20;
		sc->mac_key_client.data = new unsigned char[20];
		sc->mac_key_server.length = 20;
		sc->mac_key_server.data = new unsigned char[20];

		hash_base = std::string((const char *)sc->shared_secret.data, sc->shared_secret.length) + "C";
		SHA1((const unsigned char*)hash_base.c_str(), hash_base.length(), sc->mac_key_client.data);

		hash_base = std::string((const char *)sc->shared_secret.data, sc->shared_secret.length) + "D";
		SHA1((const unsigned char*)hash_base.c_str(), hash_base.length(), sc->mac_key_server.data);
	} else if (sc->mac_alg == SSL_TXT_SHA256){
		sc->mac_key_client.length = 32;
		sc->mac_key_client.data = new unsigned char[32];
		sc->mac_key_server.length = 32;
		sc->mac_key_server.data = new unsigned char[32];

		hash_base = std::string((const char *)sc->shared_secret.data, sc->shared_secret.length) + "C";
		SHA256((const unsigned char*)hash_base.c_str(), hash_base.length(), sc->mac_key_client.data);

		hash_base = std::string((const char *)sc->shared_secret.data, sc->shared_secret.length) + "D";
		SHA256((const unsigned char*)hash_base.c_str(), hash_base.length(), sc->mac_key_server.data);
	}

	LOG_DBG("Generated client encryption key of length %d bytes: %s",
			sc->encrypt_key_client.length,
			sc->encrypt_key_client.toString().c_str());
	LOG_DBG("Generated server encryption key of length %d bytes: %s",
			sc->encrypt_key_server.length,
			sc->encrypt_key_server.toString().c_str());
	LOG_DBG("Generated client mac key of length %d bytes: %s",
			sc->mac_key_client.length,
			sc->mac_key_client.toString().c_str());
	LOG_DBG("Generated server mac key of length %d bytes: %s",
			sc->mac_key_server.length,
			sc->mac_key_server.toString().c_str());

	return 0;
}

IAuthPolicySet::AuthStatus AuthSSH2PolicySet::crypto_state_updated(int port_id)
{
	SSH2SecurityContext * sc;

	ScopedLock sc_lock(lock);

	sc = dynamic_cast<SSH2SecurityContext *>(sec_man->get_security_context(port_id));
	if (!sc) {
		LOG_ERR("Could not retrieve SSH2 security context for port-id: %d", port_id);
		return IAuthPolicySet::FAILED;
	}

	switch(sc->state) {
	case SSH2SecurityContext::REQUESTED_ENABLE_DECRYPTION_SERVER:
		return decryption_enabled_server(sc);
	case SSH2SecurityContext::REQUESTED_ENABLE_ENCRYPTION_SERVER:
		return encryption_enabled_server(sc);
	case SSH2SecurityContext::REQUESTED_ENABLE_ENCRYPTION_DECRYPTION_CLIENT:
		return encryption_decryption_enabled_client(sc);
	default:
		LOG_ERR("Wrong security context state: %d", sc->state);
		sec_man->destroy_security_context(sc->id);
		return IAuthPolicySet::FAILED;
	}
}

IAuthPolicySet::AuthStatus AuthSSH2PolicySet::decryption_enabled_server(SSH2SecurityContext * sc)
{
	const BIGNUM *pub_key;

	if (sc->state != SSH2SecurityContext::REQUESTED_ENABLE_DECRYPTION_SERVER) {
		LOG_ERR("Wrong state of policy");
		sec_man->destroy_security_context(sc->id);
		return IAuthPolicySet::FAILED;
	}

	LOG_DBG("Decryption enabled for port-id: %d", sc->id);
	sc->crypto_rx_enabled = true;

	// Prepare message for the peer, send selected algorithms and public key
	SSH2AuthOptions auth_options;
	auth_options.key_exch_algs.push_back(sc->key_exch_alg);
	auth_options.encrypt_algs.push_back(sc->encrypt_alg);
	auth_options.mac_algs.push_back(sc->mac_alg);
	auth_options.compress_algs.push_back(sc->compress_alg);
	DH_get0_key(sc->dh_state, &pub_key, NULL);
	auth_options.dh_public_key.length = BN_num_bytes(pub_key);
	auth_options.dh_public_key.data = new unsigned char[auth_options.dh_public_key.length];
	if (BN_bn2bin(pub_key, auth_options.dh_public_key.data) <= 0 ) {
		LOG_ERR("Error transforming big number to binary");
		delete sc;
		throw Exception();
	}

	//Send message to peer with selected algorithms and public key
	try {
		cdap_rib::flags_t flags;
		cdap_rib::filt_info_t filt;
		cdap_rib::obj_info_t obj_info;
		cdap::StringEncoder encoder;

		obj_info.class_ = EDH_EXCHANGE;
		obj_info.name_ = EDH_EXCHANGE;
		obj_info.inst_ = 0;
		encode_ssh2_auth_options(auth_options,
					 obj_info.value_);

		rib_daemon->remote_write(sc->con,
					 obj_info,
					 flags,
					 filt,
					 NULL);
	} catch (Exception &e) {
		LOG_ERR("Problems encoding and sending CDAP message: %s",
			e.what());
		sec_man->destroy_security_context(sc->id);
		return IAuthPolicySet::FAILED;
	}

	// Configure kernel SDU protection policy with shared secret and algorithms
	// tell it to enable encryption
	AuthStatus result = sec_man->update_crypto_state(sc->get_crypto_state(true, false, true),
						         this);
	if (result == IAuthPolicySet::FAILED) {
		sec_man->destroy_security_context(sc->id);
		return result;
	}

	sc->state = SSH2SecurityContext::REQUESTED_ENABLE_ENCRYPTION_SERVER;
	if (result == IAuthPolicySet::SUCCESSFULL) {
		result = encryption_enabled_server(sc);
	}
	return result;
}

IAuthPolicySet::AuthStatus AuthSSH2PolicySet::encryption_enabled_server(SSH2SecurityContext * sc)
{
	if (sc->state != SSH2SecurityContext::REQUESTED_ENABLE_ENCRYPTION_SERVER) {
		LOG_ERR("Wrong state of policy");
		sec_man->destroy_security_context(sc->id);
		return IAuthPolicySet::FAILED;
	}

	LOG_DBG("Encryption enabled for port-id: %d", sc->id);
	sc->crypto_tx_enabled = true;
	sc->state = SSH2SecurityContext::WAIT_CLIENT_CHALLENGE;

	encryption_ready_condition.enable_condition();
	return IAuthPolicySet::IN_PROGRESS;
}

int AuthSSH2PolicySet::process_incoming_message(const cdap::CDAPMessage& message,
						int session_id)
{
	if (message.op_code_ != cdap::CDAPMessage::M_WRITE) {
		LOG_ERR("Wrong operation type");
		return IAuthPolicySet::FAILED;
	}

	if (message.obj_class_ == EDH_EXCHANGE) {
		return process_edh_exchange_message(message, session_id);
	}

	if (message.obj_class_ == CLIENT_CHALLENGE) {
		return process_client_challenge_message(message, session_id);
	}

	if (message.obj_class_ == CLIENT_CHALLENGE_REPLY) {
		return process_client_challenge_reply_message(message, session_id);
	}

	if (message.obj_class_ == SERVER_CHALLENGE_REPLY) {
		return process_server_challenge_reply_message(message, session_id);
	}

	return rina::IAuthPolicySet::FAILED;
}

int AuthSSH2PolicySet::process_edh_exchange_message(const cdap::CDAPMessage& message,
						    int session_id)
{
	SSH2SecurityContext * sc;
	SSH2AuthOptions options;

	if (message.obj_value_.message_ == 0) {
		LOG_ERR("Null object value");
		return IAuthPolicySet::FAILED;
	}

	decode_ssh2_auth_options(message.obj_value_, options);

	ScopedLock sc_lock(lock);

	sc = dynamic_cast<SSH2SecurityContext *>(sec_man->get_security_context(session_id));
	if (!sc) {
		LOG_ERR("Could not retrieve Security Context for session: %d", session_id);
		return IAuthPolicySet::FAILED;
	}

	if (sc->state != SSH2SecurityContext::WAIT_EDH_EXCHANGE) {
		LOG_ERR("Wrong session state: %d", sc->state);
		sec_man->remove_security_context(session_id);
		return IAuthPolicySet::FAILED;
	}

	//Add peer public key to security context
	sc->dh_peer_pub_key = BN_bin2bn(options.dh_public_key.data,
			       	        options.dh_public_key.length,
			       	        NULL);
	if (!sc->dh_peer_pub_key) {
		LOG_ERR("Error converting public key to a BIGNUM");
		delete sc;
		return rina::IAuthPolicySet::FAILED;
	}

	//Generate the shared secret
	if (edh_generate_shared_secret(sc) != 0) {
		delete sc;
		return rina::IAuthPolicySet::FAILED;
	}

	// Configure kernel SDU protection policy with shared secret and algorithms
	// tell it to enable decryption and encryption
	AuthStatus result = sec_man->update_crypto_state(sc->get_crypto_state(true, true, false),
						         this);
	if (result == IAuthPolicySet::FAILED) {
		sec_man->destroy_security_context(sc->id);
		return result;
	}

	sc->state = SSH2SecurityContext::REQUESTED_ENABLE_ENCRYPTION_DECRYPTION_CLIENT;
	if (result == IAuthPolicySet::SUCCESSFULL) {
		result = encryption_decryption_enabled_client(sc);
	}
	return result;
}

IAuthPolicySet::AuthStatus AuthSSH2PolicySet::encryption_decryption_enabled_client(SSH2SecurityContext * sc)
{
	if (sc->state != SSH2SecurityContext::REQUESTED_ENABLE_ENCRYPTION_DECRYPTION_CLIENT) {
		LOG_ERR("Wrong state of policy");
		sec_man->destroy_security_context(sc->id);
		return IAuthPolicySet::FAILED;
	}

	LOG_DBG("Encryption and decryption enabled for port-id: %d", sc->id);
	sc->crypto_rx_enabled = true;
	sc->crypto_tx_enabled = true;

	//Client authenticates server: generate random challenge, encrypt
	//with public key and authenticate server
	UcharArray encrypted_challenge;
	if (generate_and_encrypt_challenge(sc, encrypted_challenge) != 0) {
		sec_man->destroy_security_context(sc->id);
		return IAuthPolicySet::FAILED;
	}

	sc->state = SSH2SecurityContext::WAIT_CLIENT_CHALLENGE_REPLY;

	//Send message to peer with selected algorithms and public key
	try {
		cdap_rib::flags_t flags;
		cdap_rib::filt_info_t filt;
		cdap_rib::obj_info_t obj_info;
		cdap::StringEncoder encoder;

		obj_info.class_ = CLIENT_CHALLENGE;
		obj_info.name_ = CLIENT_CHALLENGE;
		obj_info.inst_ = 0;
		encrypted_challenge.get_seralized_object(obj_info.value_);

		rib_daemon->remote_write(sc->con, obj_info, flags, filt, NULL);
	} catch (Exception &e) {
		LOG_ERR("Problems encoding and sending CDAP message: %s", e.what());
		sec_man->destroy_security_context(sc->id);
		return IAuthPolicySet::FAILED;
	}

	return IAuthPolicySet::IN_PROGRESS;
}

int AuthSSH2PolicySet::generate_random_challenge(SSH2SecurityContext * sc)
{
	sc->challenge.data = new unsigned char[sc->challenge.length];
	int result = RAND_bytes(sc->challenge.data, sc->challenge.length);
	if (result != 1) {
		LOG_ERR("Error generating random number: %s",
			ERR_error_string(ERR_get_error(), NULL));
		return -1;
	}

	return 0;
}

int AuthSSH2PolicySet::encrypt_chall_with_pub_key(SSH2SecurityContext * sc,
						  UcharArray& encrypted_chall)
{
	encrypted_chall.data = new unsigned char[RSA_size(sc->auth_keypair)];
	encrypted_chall.length = RSA_public_encrypt(sc->challenge.length,
						   sc->challenge.data,
						   encrypted_chall.data,
						   sc->auth_peer_pub_key,
						   RSA_PKCS1_OAEP_PADDING);

	if (encrypted_chall.length == -1) {
		LOG_ERR("Error encrypting challenge with RSA public key: %s",
			ERR_error_string(ERR_get_error(), NULL));
		return -1;
	}

	return 0;
}

int AuthSSH2PolicySet::process_client_challenge_message(const cdap::CDAPMessage& message, int session_id)
{
	SSH2SecurityContext * sc;

	if (message.obj_value_.message_ == 0) {
		LOG_ERR("Null object value");
		return IAuthPolicySet::FAILED;
	}

	encryption_ready_condition.wait_until_condition_is_true();

	ScopedLock sc_lock(lock);

	sc = dynamic_cast<SSH2SecurityContext *>(sec_man->get_security_context(session_id));
	if (!sc) {
		LOG_ERR("Could not retrieve Security Context for session: %d", session_id);
		return IAuthPolicySet::FAILED;
	}

	if (sc->state != SSH2SecurityContext::WAIT_CLIENT_CHALLENGE) {
		LOG_ERR("Wrong session state: %d", sc->state);
		sec_man->remove_security_context(session_id);
		return IAuthPolicySet::FAILED;
	}

	// Decrypt challenge with private key, XOR with shared secret
	// compute MD5 hash and sent back to client
	UcharArray encrypted_challenge(&message.obj_value_);
	UcharArray hashed_challenge;
	if (decrypt_combine_and_hash(sc, encrypted_challenge, hashed_challenge) != 0) {
		sec_man->destroy_security_context(sc->id);
		return IAuthPolicySet::FAILED;
	}

	//Server authenticates client: generate random challenge, encrypt
	//with public key and authenticate server
	UcharArray encrypted_server_challenge;
	if (generate_and_encrypt_challenge(sc, encrypted_server_challenge) != 0) {
		sec_man->destroy_security_context(sc->id);
		return IAuthPolicySet::FAILED;
	}

	sc->state = SSH2SecurityContext::WAIT_SERVER_CHALLENGE_REPLY;

	//Send message to peer with selected algorithms and public key
	try {
		cdap_rib::flags_t flags;
		cdap_rib::filt_info_t filt;
		cdap_rib::obj_info_t obj_info;
		cdap::StringEncoder encoder;

		obj_info.class_ = CLIENT_CHALLENGE_REPLY;
		obj_info.name_ = CLIENT_CHALLENGE_REPLY;
		obj_info.inst_ = 0;
		encode_client_chall_reply_ssh2(hashed_challenge,
					       encrypted_server_challenge,
					       obj_info.value_);
		rib_daemon->remote_write(sc->con, obj_info, flags, filt, NULL);
	} catch (Exception &e) {
		LOG_ERR("Problems encoding and sending CDAP message: %s", e.what());
		sec_man->destroy_security_context(sc->id);
		return IAuthPolicySet::FAILED;
	}

	return IAuthPolicySet::IN_PROGRESS;
}

int AuthSSH2PolicySet::generate_and_encrypt_challenge(SSH2SecurityContext * sc,
				   	   	      UcharArray& challenge)
{
	if (generate_random_challenge(sc) != 0) {
		return -1;
	}

	if (encrypt_chall_with_pub_key(sc, challenge) != 0) {
		return -1;
	}

	return 0;
}


int AuthSSH2PolicySet::decrypt_combine_and_hash(SSH2SecurityContext * sc,
						const UcharArray& challenge,
						UcharArray& result)
{
	UcharArray decrypt_challenge;
	if (decrypt_chall_with_priv_key(sc, challenge, decrypt_challenge) != 0) {
		return -1;
	}

	int j = 0;
	for (int i=0; i< decrypt_challenge.length; i++) {
		decrypt_challenge.data[i] = decrypt_challenge.data[i] ^ sc->shared_secret.data[j];
		j++;
		if (j == sc->shared_secret.length) {
			j = 0;
		}
	}

	result.length = 16;
	result.data = new unsigned char[16];
	MD5(decrypt_challenge.data, decrypt_challenge.length, result.data);

	return 0;
}

int AuthSSH2PolicySet::decrypt_chall_with_priv_key(SSH2SecurityContext * sc,
			        	  	   const UcharArray& encrypted_challenge,
			        	  	   UcharArray& challenge)
{
	challenge.data = new unsigned char[RSA_size(sc->auth_keypair)];
	challenge.length = RSA_private_decrypt(encrypted_challenge.length,
			 	 	       encrypted_challenge.data,
			 	 	       challenge.data,
			 	 	       sc->auth_keypair,
			 	 	       RSA_PKCS1_OAEP_PADDING);

	if (challenge.length == -1) {
		LOG_ERR("Error decrypting challenge with RSA private key: %s",
			ERR_error_string(ERR_get_error(), NULL));
		return -1;
	}

	return 0;
}

int AuthSSH2PolicySet::process_client_challenge_reply_message(const cdap::CDAPMessage& message,
							      int session_id)
{
	SSH2SecurityContext * sc;

	if (message.obj_value_.message_ == 0) {
		LOG_ERR("Null object value");
		return IAuthPolicySet::FAILED;
	}

	ScopedLock sc_lock(lock);

	sc = dynamic_cast<SSH2SecurityContext *>(sec_man->get_security_context(session_id));
	if (!sc) {
		LOG_ERR("Could not retrieve Security Context for session: %d", session_id);
		return IAuthPolicySet::FAILED;
	}

	if (sc->state != SSH2SecurityContext::WAIT_CLIENT_CHALLENGE_REPLY) {
		LOG_ERR("Wrong session state: %d", sc->state);
		sec_man->remove_security_context(session_id);
		return IAuthPolicySet::FAILED;
	}

	// XOR original challenge with shared secret, compute MD5 hash and check if
	// result is equal to received challenge
	UcharArray received_challenge;
	UcharArray server_challenge;
	decode_client_chall_reply_ssh2(message.obj_value_,
				       received_challenge,
				       server_challenge);

	if (check_challenge_reply(sc, received_challenge) != 0) {
		sec_man->remove_security_context(session_id);
		return IAuthPolicySet::FAILED;
	}

	// Decrypt server challenge with private key, XOR with shared secret
	// compute MD5 hash and sent back to client
	UcharArray hashed_ser_challenge;
	if (decrypt_combine_and_hash(sc, server_challenge, hashed_ser_challenge) != 0) {
		sec_man->destroy_security_context(sc->id);
		return IAuthPolicySet::FAILED;
	}

	sc->state = SSH2SecurityContext::DONE;

	//Send message to peer with selected algorithms and public key
	try {
		cdap_rib::flags_t flags;
		cdap_rib::filt_info_t filt;
		cdap_rib::obj_info_t obj_info;
		cdap::StringEncoder encoder;

		obj_info.class_ = SERVER_CHALLENGE_REPLY;
		obj_info.name_ = SERVER_CHALLENGE_REPLY;
		obj_info.inst_ = 0;
		hashed_ser_challenge.get_seralized_object(obj_info.value_);

		rib_daemon->remote_write(sc->con, obj_info, flags, filt,  NULL);
	} catch (Exception &e) {
		LOG_ERR("Problems encoding and sending CDAP message: %s", e.what());
		sec_man->destroy_security_context(sc->id);
		return IAuthPolicySet::FAILED;
	}

	return IAuthPolicySet::IN_PROGRESS;
}

int AuthSSH2PolicySet::check_challenge_reply(SSH2SecurityContext * sc,
			  	  	     UcharArray& received_challenge)
{
	UcharArray hashed_challenge;

	int j = 0;
	for (int i=0; i< sc->challenge.length; i++) {
		sc->challenge.data[i] = sc->challenge.data[i] ^ sc->shared_secret.data[j];
		j++;
		if (j == sc->shared_secret.length) {
			j = 0;
		}
	}

	hashed_challenge.length = 16;
	hashed_challenge.data = new unsigned char[16];
	MD5(sc->challenge.data, sc->challenge.length, hashed_challenge.data);

	if (hashed_challenge != received_challenge) {
		LOG_ERR("Error authenticating server. Hashed challenge: %s, received challenge: %s",
			hashed_challenge.toString().c_str(),
			received_challenge.toString().c_str());
		return -1;
	}

	LOG_INFO("Remote peer successfully authenticated");
	return 0;
}

int AuthSSH2PolicySet::process_server_challenge_reply_message(const cdap::CDAPMessage& message, int session_id)
{
	SSH2SecurityContext * sc;

	if (message.obj_value_.message_ == 0) {
		LOG_ERR("Null object value");
		return IAuthPolicySet::FAILED;
	}

	ScopedLock sc_lock(lock);

	sc = dynamic_cast<SSH2SecurityContext *>(sec_man->get_security_context(session_id));
	if (!sc) {
		LOG_ERR("Could not retrieve Security Context for session: %d", session_id);
		return IAuthPolicySet::FAILED;
	}

	if (sc->state != SSH2SecurityContext::WAIT_SERVER_CHALLENGE_REPLY) {
		LOG_ERR("Wrong session state: %d", sc->state);
		sec_man->remove_security_context(session_id);
		return IAuthPolicySet::FAILED;
	}

	// XOR original challenge with shared secret, compute MD5 hash and check if
	// result is equal to received challenge
	UcharArray received_challenge(&message.obj_value_);
	if (check_challenge_reply(sc, received_challenge) != 0) {
		sec_man->remove_security_context(session_id);
		return IAuthPolicySet::FAILED;
	}

	sc->state = SSH2SecurityContext::DONE;
	return IAuthPolicySet::SUCCESSFULL;
}

int AuthSSH2PolicySet::set_policy_set_param(const std::string& name,
                         	 	      const std::string& value)
{
        LOG_DBG("No policy-set-specific parameters to set (%s, %s)",
                        name.c_str(), value.c_str());
        return -1;
}

//Class ISecurity Manager
ISecurityManager::~ISecurityManager()
{
	std::list<IAuthPolicySet*> entries = auth_policy_sets.getEntries();
	std::list<IAuthPolicySet*>::iterator it;
	IAuthPolicySet * currentPs = 0;
	for (it = entries.begin(); it != entries.end(); ++it) {
		currentPs = (*it);
		app->psDestroy(rina::ApplicationEntity::SECURITY_MANAGER_AE_NAME,
			       currentPs->type,
			       currentPs);
	}

	std::list<ISecurityContext*> contexts = security_contexts.getEntries();
	std::list<ISecurityContext*>::iterator it2;
	ISecurityContext * context = 0;
	for (it2 = contexts.begin(); it2 != contexts.end(); ++it2) {
		context = (*it2);
		if (context) {
			delete context;
		}
	}
}

int ISecurityManager::add_auth_policy_set(const std::string& auth_type)
{
        IAuthPolicySet *candidate = NULL;

        if (!app) {
                LOG_ERR("bug: NULL app reference");
                return -1;
        }

        if (get_auth_policy_set(auth_type)) {
                LOG_INFO("authentication policy set %s already included in the security-manager",
                         auth_type.c_str());
                return 0;
        }

        candidate = (IAuthPolicySet *) app->psCreate(ApplicationEntity::SECURITY_MANAGER_AE_NAME,
        					     auth_type,
        					     this);
        if (!candidate) {
                LOG_ERR("failed to allocate instance of policy set %s", auth_type.c_str());
                return -1;
        }

        // Install the new one.
        add_auth_policy_set(auth_type, candidate);

        return 0;
}

void ISecurityManager::add_auth_policy_set(const std::string& auth_type,
					   IAuthPolicySet * ps)
{
        auth_policy_sets.put(auth_type, ps);
        LOG_INFO("Authentication policy-set %s added to the security-manager",
                        auth_type.c_str());
}

int ISecurityManager::set_policy_set_param(const std::string& path,
                         	 	   const std::string& name,
                         	 	   const std::string& value)
{
	IPolicySet * selected_ps;

        LOG_DBG("set_policy_set_param(%s, %s) called",
                name.c_str(), value.c_str());

        if (path == std::string()) {
        	// This request is for the component itself
        	LOG_ERR("No such parameter '%s' exists", name.c_str());
        	return -1;
        } else if (path == selected_ps_name) {
        	selected_ps = ps;
        } else {
        	selected_ps = auth_policy_sets.find(path);
        }

        if (!selected_ps) {
                LOG_ERR("Invalid component address '%s'", path.c_str());
                return -1;
        }

        return selected_ps->set_policy_set_param(name, value);
}

IAuthPolicySet * ISecurityManager::get_auth_policy_set(const std::string& auth_type)
{
	return auth_policy_sets.find(auth_type);
}

ISecurityContext * ISecurityManager::get_security_context(int context_id)
{
	return security_contexts.find(context_id);
}

ISecurityContext * ISecurityManager::remove_security_context(int context_id)
{
	return security_contexts.erase(context_id);
}

void ISecurityManager::destroy_security_context(int context_id)
{
	ISecurityContext * ctx = remove_security_context(context_id);
	if (ctx) {
		delete ctx;
		ctx = NULL;
	}
}

void ISecurityManager::add_security_context(ISecurityContext * context)
{
	if (!context) {
		LOG_ERR("Bogus security context passed, not storing it");
		return;
	}

	security_contexts.put(context->id, context);
}

void ISecurityManager::eventHappened(InternalEvent * event)
{
	NMinusOneFlowDeallocatedEvent * n_event = 0;
	ISecurityContext * context = 0;

	if (event->type == InternalEvent::APP_N_MINUS_1_FLOW_DEALLOCATED) {
		n_event = dynamic_cast<NMinusOneFlowDeallocatedEvent *>(event);
		context = security_contexts.erase(n_event->port_id_);
		if (context) {
			delete context;
		}
	}
}

}


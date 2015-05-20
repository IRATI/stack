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
#include <openssl/ssl.h>

#define RINA_PREFIX "librina.security-manager"

#include "librina/logs.h"
#include "librina/security-manager.h"
#include "auth-policies.pb.h"

namespace rina {

//Class AuthPolicySet
const std::string IAuthPolicySet::AUTH_NONE = "PSOC_authentication-none";
const std::string IAuthPolicySet::AUTH_PASSWORD = "PSOC_authentication-password";
const std::string IAuthPolicySet::AUTH_SSHRSA = "PSOC_authentication-sshrsa";
const std::string IAuthPolicySet::AUTH_SSHDSA = "PSOC_authentication-sshdsa";

IAuthPolicySet::IAuthPolicySet(const std::string& type_)
{
	type = type_;
}

//Class AuthNonePolicySet
AuthPolicy AuthNonePolicySet::get_auth_policy(int session_id,
					      const AuthSDUProtectionProfile& profile)
{
	if (profile.authPolicy.name_ != type) {
		LOG_ERR("Wrong policy name: %s", profile.authPolicy.name_.c_str());
		throw Exception();
	}

	ISecurityContext * sc = new ISecurityContext(session_id);
	sc->crcPolicy = profile.crcPolicy;
	sc->ttlPolicy = profile.ttlPolicy;
	sec_man->add_security_context(sc);

	AuthPolicy result;
	result.name_ = IAuthPolicySet::AUTH_NONE;
	result.versions_.push_back(profile.authPolicy.version_);
	return result;
}

rina::IAuthPolicySet::AuthStatus AuthNonePolicySet::initiate_authentication(const AuthPolicy& auth_policy,
									    const AuthSDUProtectionProfile& profile,
								      	    int session_id)
{
	if (auth_policy.name_ != type) {
		LOG_ERR("Wrong policy name: %s", auth_policy.name_.c_str());
		return rina::IAuthPolicySet::FAILED;
	}

	if (auth_policy.versions_.front() != RINA_DEFAULT_POLICY_VERSION) {
		LOG_ERR("Unsupported policy version: %s",
				auth_policy.versions_.front().c_str());
		return rina::IAuthPolicySet::FAILED;
	}

	ISecurityContext * sc = new ISecurityContext(session_id);
	sc->crcPolicy = profile.crcPolicy;
	sc->ttlPolicy = profile.ttlPolicy;
	sec_man->add_security_context(sc);

	return rina::IAuthPolicySet::SUCCESSFULL;
}

// No authentication messages exchanged
int AuthNonePolicySet::process_incoming_message(const CDAPMessage& message,
						 int session_id)
{
	(void) message;
	(void) session_id;

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
		ISecurityContext(session_id)
{
	challenge = 0;
	timer_task = 0;
	challenge_length = 0;
}

//Class AuthPasswdPolicySet
const std::string AuthPasswordPolicySet::PASSWORD = "password";
const std::string AuthPasswordPolicySet::CIPHER = "cipher";
const std::string AuthPasswordPolicySet::CHALLENGE_LENGTH = "challenge-length";
const std::string AuthPasswordPolicySet::DEFAULT_CIPHER = "default_cipher";
const std::string AuthPasswordPolicySet::CHALLENGE_REQUEST = "challenge request";
const std::string AuthPasswordPolicySet::CHALLENGE_REPLY = "challenge reply";
const int AuthPasswordPolicySet::DEFAULT_TIMEOUT = 10000;

AuthPasswordPolicySet::AuthPasswordPolicySet(IRIBDaemon * ribd, ISecurityManager * sm) :
		IAuthPolicySet(IAuthPolicySet::AUTH_PASSWORD)
{
	rib_daemon = ribd;
	sec_man = sm;
	cipher = DEFAULT_CIPHER;
	timeout = DEFAULT_TIMEOUT;
}

// No credentials required, since the process being authenticated
// will have to demonstrate that it knows the password by encrypting
// a random challenge with a password string
AuthPolicy AuthPasswordPolicySet::get_auth_policy(int session_id,
						  const AuthSDUProtectionProfile& profile)
{
	if (profile.authPolicy.name_ != type) {
		LOG_ERR("Wrong policy name: %s", profile.authPolicy.name_.c_str());
		throw Exception();
	}

	AuthPasswordSecurityContext * sc = new AuthPasswordSecurityContext(session_id);
	sc->password = profile.authPolicy.get_param_value(PASSWORD);
	if (string2int(profile.authPolicy.get_param_value(CHALLENGE_LENGTH), sc->challenge_length) != 0) {
		LOG_ERR("Error parsing challenge length string as integer");
		delete sc;
		throw Exception();
	}
	sc->cipher = profile.authPolicy.get_param_value(CIPHER);
	sc->crcPolicy = profile.crcPolicy;
	sc->ttlPolicy = profile.ttlPolicy;
	sec_man->add_security_context(sc);

	rina::AuthPolicy result;
	result.name_ = IAuthPolicySet::AUTH_PASSWORD;
	result.versions_.push_back(profile.authPolicy.version_);
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

rina::IAuthPolicySet::AuthStatus AuthPasswordPolicySet::initiate_authentication(const AuthPolicy& auth_policy,
										const AuthSDUProtectionProfile& profile,
								      	        int session_id)
{
	if (auth_policy.name_ != type) {
		LOG_ERR("Wrong policy name: %s", auth_policy.name_.c_str());
		return rina::IAuthPolicySet::FAILED;
	}

	if (auth_policy.versions_.front() != RINA_DEFAULT_POLICY_VERSION) {
		LOG_ERR("Unsupported policy version: %s",
				auth_policy.versions_.front().c_str());
		return rina::IAuthPolicySet::FAILED;
	}

	AuthPasswordSecurityContext * sc = new AuthPasswordSecurityContext(session_id);
	sc->password = profile.authPolicy.get_param_value(PASSWORD);
	if (string2int(profile.authPolicy.get_param_value(CHALLENGE_LENGTH), sc->challenge_length) != 0) {
		LOG_ERR("Error parsing challenge length string as integer");
		delete sc;
		return rina::IAuthPolicySet::FAILED;
	}
	sc->cipher = profile.authPolicy.get_param_value(CIPHER);
	sc->crcPolicy = profile.crcPolicy;
	sc->ttlPolicy = profile.ttlPolicy;
	sec_man->add_security_context(sc);

	ScopedLock scopedLock(lock);

	//1 Generate a random password string and send it to the AP being authenticated
	sc->challenge = generate_random_challenge(sc->challenge_length);

	try {
		RIBObjectValue robject_value;
		robject_value.type_ = RIBObjectValue::stringtype;
		robject_value.string_value_ = *(sc->challenge);

		RemoteProcessId remote_id;
		remote_id.port_id_ = session_id;

		//object class contains challenge request or reply
		//object name contains cipher name
		rib_daemon->remoteWriteObject(CHALLENGE_REQUEST, cipher,
				robject_value, 0, remote_id, 0);
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
		RIBObjectValue robject_value;
		robject_value.type_ = RIBObjectValue::stringtype;
		robject_value.string_value_ = encrypt_challenge(challenge, sc->password);

		RemoteProcessId remote_id;
		remote_id.port_id_ = session_id;

		//object class contains challenge request or reply
		//object name contains cipher name
		rib_daemon->remoteWriteObject(CHALLENGE_REPLY, cipher,
				robject_value, 0, remote_id, 0);
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

	std::string recovered_challenge = decrypt_challenge(encrypted_challenge, sc->password);
	if (*(sc->challenge) == recovered_challenge) {
		result = IAuthPolicySet::SUCCESSFULL;
	} else {
		LOG_DBG("Authentication failed");
	}

	return result;
}

int AuthPasswordPolicySet::process_incoming_message(const CDAPMessage& message,
						     int session_id)
{
	StringObjectValue * string_value = 0;
	const std::string * challenge = 0;

	if (message.op_code_ != CDAPMessage::M_WRITE) {
		LOG_ERR("Wrong operation type");
		return IAuthPolicySet::FAILED;
	}

	if (message.obj_value_ == 0) {
		LOG_ERR("Null object value");
		return IAuthPolicySet::FAILED;
	}

	string_value = dynamic_cast<StringObjectValue *>(message.obj_value_);
	if (!string_value) {
		LOG_ERR("Object value of wrong type");
		return IAuthPolicySet::FAILED;
	}

	challenge = (const std::string *) string_value->get_value();
	if (!challenge) {
		LOG_ERR("Error decoding challenge");
		return IAuthPolicySet::FAILED;
	}

	if (message.obj_class_ == CHALLENGE_REQUEST) {
		return process_challenge_request(*challenge,
						 session_id);
	}

	if (message.obj_class_ == CHALLENGE_REPLY) {
		return process_challenge_reply(*challenge,
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

//AuthSSHRSAOptions encoder and decoder operations
SSHRSAAuthOptions * decode_ssh_rsa_auth_options(const SerializedObject &message) {
	rina::auth::policies::googleprotobuf::authOptsSSHRSA_t gpb_options;
	SSHRSAAuthOptions * result = new SSHRSAAuthOptions();

	gpb_options.ParseFromArray(message.message_, message.size_);

	for(int i=0; i<gpb_options.key_exch_algs_size(); i++) {
		result->key_exch_algs.push_back(gpb_options.key_exch_algs(i));
	}

	for(int i=0; i<gpb_options.encrypt_algs_size(); i++) {
		result->encrypt_algs.push_back(gpb_options.encrypt_algs(i));
	}

	for(int i=0; i<gpb_options.mac_algs_size(); i++) {
		result->mac_algs.push_back(gpb_options.mac_algs(i));
	}

	for(int i=0; i<gpb_options.compress_algs_size(); i++) {
		result->compress_algs.push_back(gpb_options.compress_algs(i));
	}

	if (gpb_options.has_dh_public_key()) {
		  result->dh_public_key.array =
				  new unsigned char[gpb_options.dh_public_key().size()];
		  memcpy(result->dh_public_key.array,
			 gpb_options.dh_public_key().data(),
			 gpb_options.dh_public_key().size());
		  result->dh_public_key.length = gpb_options.dh_public_key().size();
	}

	if (gpb_options.has_dh_g()) {
		  result->dh_parameter_g.array =
				  new unsigned char[gpb_options.dh_g().size()];
		  memcpy(result->dh_parameter_g.array,
			 gpb_options.dh_g().data(),
			 gpb_options.dh_g().size());
		  result->dh_parameter_g.length = gpb_options.dh_g().size();
	}

	if (gpb_options.has_dh_p()) {
		  result->dh_parameter_p.array =
				  new unsigned char[gpb_options.dh_p().size()];
		  memcpy(result->dh_parameter_p.array,
			 gpb_options.dh_p().data(),
			 gpb_options.dh_p().size());
		  result->dh_parameter_g.length = gpb_options.dh_p().size();
	}

	return result;
}

SerializedObject * encode_ssh_rsa_auth_options(const SSHRSAAuthOptions& options){
	rina::auth::policies::googleprotobuf::authOptsSSHRSA_t gpb_options;

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
		gpb_options.set_dh_public_key(options.dh_public_key.array,
					      options.dh_public_key.length);
	}

	if (options.dh_parameter_g.length > 0) {
		gpb_options.set_dh_g(options.dh_parameter_g.array,
				     options.dh_parameter_g.length);
	}

	if (options.dh_parameter_p.length > 0) {
		gpb_options.set_dh_p(options.dh_parameter_p.array,
				     options.dh_parameter_p.length);
	}

	int size = gpb_options.ByteSize();
	char *serialized_message = new char[size];
	gpb_options.SerializeToArray(serialized_message, size);
	SerializedObject *object = new SerializedObject(serialized_message, size);

	return object;
}

// Class SSHRSASecurityContext
SSHRSASecurityContext::~SSHRSASecurityContext()
{
	if (dh_state) {
		DH_free(dh_state);
	}

	if (dh_peer_pub_key) {
		BN_free(dh_peer_pub_key);
	}
}

//Class AuthSSHRSA
const int AuthSSHRSAPolicySet::DEFAULT_TIMEOUT = 10000;
const std::string AuthSSHRSAPolicySet::KEY_EXCHANGE_ALGORITHM = "keyExchangeAlg";
const std::string AuthSSHRSAPolicySet::ENCRYPTION_ALGORITHM = "encryptAlg";
const std::string AuthSSHRSAPolicySet::MAC_ALGORITHM = "macAlg";
const std::string AuthSSHRSAPolicySet::COMPRESSION_ALGORITHM = "compressAlg";
const std::string AuthSSHRSAPolicySet::EDH_EXCHANGE = "Ephemeral Diffie-Hellman exchange";

AuthSSHRSAPolicySet::AuthSSHRSAPolicySet(IRIBDaemon * ribd, ISecurityManager * sm) :
		IAuthPolicySet(IAuthPolicySet::AUTH_SSHRSA)
{
	rib_daemon = ribd;
	sec_man = sm;
	timeout = DEFAULT_TIMEOUT;
	dh_parameters = 0;
}

AuthSSHRSAPolicySet::~AuthSSHRSAPolicySet()
{
	if (dh_parameters) {
		DH_free(dh_parameters);
	}
}

AuthPolicy AuthSSHRSAPolicySet::get_auth_policy(int session_id,
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
		LOG_ERR("A security context already exists for session_id: %d", session_id);
		throw Exception();
	}

	AuthPolicy auth_policy;
	auth_policy.name_ = IAuthPolicySet::AUTH_SSHRSA;
	auth_policy.versions_.push_back(profile.authPolicy.version_);

	SSHRSASecurityContext * sc = new SSHRSASecurityContext(session_id);
	sc->key_exch_alg = profile.authPolicy.get_param_value(KEY_EXCHANGE_ALGORITHM);
	sc->encrypt_alg = profile.authPolicy.get_param_value(ENCRYPTION_ALGORITHM);
	sc->mac_alg = profile.authPolicy.get_param_value(MAC_ALGORITHM);
	sc->compress_alg = profile.authPolicy.get_param_value(COMPRESSION_ALGORITHM);
	sc->crcPolicy = profile.crcPolicy;
	sc->ttlPolicy = profile.ttlPolicy;

	//Initialize Diffie-Hellman machinery and generate private/public key pairs
	if (edh_init_keys(sc) != 0) {
		delete sc;
		throw Exception();
	}

	SSHRSAAuthOptions options;
	options.key_exch_algs.push_back(sc->key_exch_alg);
	options.encrypt_algs.push_back(sc->encrypt_alg);
	options.mac_algs.push_back(sc->mac_alg);
	options.compress_algs.push_back(sc->compress_alg);
	options.dh_public_key.length = BN_bn2bin(sc->dh_state->pub_key,
						 options.dh_public_key.array);
	options.dh_parameter_g = BN_bn2bin(sc->dh_state->g,
			 	 	   options.dh_parameter_g.array);
	options.dh_parameter_p = BN_bn2bin(sc->dh_state->p,
			 	 	   options.dh_parameter_p.array);

	if (options.dh_public_key.length <= 0 ||
			options.dh_parameter_g.length <= 0 ||
			options.dh_parameter_p.length <= 0) {
		LOG_ERR("Error transforming big number to binary");
		delete sc;
		throw Exception();
	}

	SerializedObject * sobj = encode_ssh_rsa_auth_options(options);
	if (!sobj) {
		LOG_ERR("Problems encoding SSHRSAAuthOptions");
		delete sc;
		throw Exception();
	}

	auth_policy.options_ = *sobj;
	delete sobj;

	//Store security context
	sc->state = SSHRSASecurityContext::WAIT_EDH_EXCHANGE;
	sec_man->add_security_context(sc);

	return auth_policy;
}

int AuthSSHRSAPolicySet::edh_init_parameters()
{
	int codes;

	if ((dh_parameters = DH_new()) == NULL) {
		LOG_ERR("Error initializing Diffie-Hellman state");
		return -1;
	}

	if (DH_generate_parameters_ex(dh_parameters, 2048, DH_GENERATOR_2, NULL) != 1) {
		LOG_ERR("Error generating Diffie-Hellman parameters");
		DH_free(dh_parameters);
		return -1;
	}

	if (DH_check(dh_parameters, &codes) != 1) {
		LOG_ERR("Error checking parameters");
		DH_free(dh_parameters);
		return -1;
	}

	if (codes != 0)
	{
		LOG_ERR("Diffie-Hellman check has failed");
		DH_free(dh_parameters);
		return -1;
	}

	return 0;
}

int AuthSSHRSAPolicySet::edh_init_keys(SSHRSASecurityContext * sc)
{
	DH *dh_state;

	// Init own parameters
	if (!dh_parameters && edh_init_parameters() != 0) {
		LOG_ERR("Error initializing Diffie-Hellman parameters");
		return -1;
	}

	if ((dh_state = DH_new()) == NULL) {
		LOG_ERR("Error initializing Diffie-Hellman state");
		return -1;
	}

	// Re-use P and G generated before
	dh_state->p = BN_dup(dh_parameters->p);
	dh_state->g = BN_dup(dh_parameters->g);

	// Generate the public and private key pair
	if (DH_generate_key(dh_state) != 1) {
		LOG_ERR("Error generating public and private key pair");
		DH_free(dh_state);
		return -1;
	}

	sc->dh_state = dh_state;

	return 0;
}

rina::IAuthPolicySet::AuthStatus AuthSSHRSAPolicySet::initiate_authentication(const AuthPolicy& auth_policy,
									      const AuthSDUProtectionProfile& profile,
								      	      int session_id)
{
	(void) profile;

	if (auth_policy.name_ != type) {
		LOG_ERR("Wrong policy name: %s", auth_policy.name_.c_str());
		return rina::IAuthPolicySet::FAILED;
	}

	if (auth_policy.versions_.front() != RINA_DEFAULT_POLICY_VERSION) {
		LOG_ERR("Unsupported policy version: %s",
				auth_policy.versions_.front().c_str());
		return rina::IAuthPolicySet::FAILED;
	}

	ScopedLock sc_lock(lock);

	if (sec_man->get_security_context(session_id) != 0) {
		LOG_ERR("A security context already exists for session_id: %d", session_id);
		return rina::IAuthPolicySet::FAILED;
	}

	SSHRSAAuthOptions * options = decode_ssh_rsa_auth_options(auth_policy.options_);
	if (!options) {
		LOG_ERR("Could not decode SSHARSA options");
		return rina::IAuthPolicySet::FAILED;
	}

	SSHRSASecurityContext * sc = new SSHRSASecurityContext(session_id);
	std::string current_alg = options->key_exch_algs.front();
	if (current_alg != SSL_TXT_EDH) {
		LOG_ERR("Unsupported key exchange algorithm: %s",
			current_alg.c_str());
		delete sc;
		delete options;
		return rina::IAuthPolicySet::FAILED;
	} else {
		sc->key_exch_alg = current_alg;
	}

	current_alg = options->encrypt_algs.front();
	if (current_alg != SSL_TXT_AES128 && current_alg != SSL_TXT_AES256) {
		LOG_ERR("Unsupported encryption algorithm: %s",
			current_alg.c_str());
		delete sc;
		delete options;
		return rina::IAuthPolicySet::FAILED;
	} else {
		sc->encrypt_alg = current_alg;
	}

	current_alg = options->mac_algs.front();
	if (current_alg != SSL_TXT_MD5 && current_alg != SSL_TXT_SHA1) {
		LOG_ERR("Unsupported MAC algorithm: %s",
			current_alg.c_str());
		delete sc;
		delete options;
		return rina::IAuthPolicySet::FAILED;
	} else {
		sc->mac_alg = current_alg;
	}

	//TODO check compression algorithm when we use them

	//Initialize Diffie-Hellman machinery and generate private/public key pairs
	BIGNUM * g = BN_bin2bn(options->dh_parameter_g.array,
			       options->dh_parameter_g.length,
			       NULL);
	if (!g) {
		LOG_ERR("Error converting parameter g to a BIGNUM");
		delete sc;
		delete options;
		return rina::IAuthPolicySet::FAILED;
	}

	BIGNUM * p = BN_bin2bn(options->dh_parameter_p.array,
			       options->dh_parameter_p.length,
			       NULL);
	if (!p) {
		LOG_ERR("Error converting parameter p to a BIGNUM");
		delete sc;
		delete options;
		delete g;
		return rina::IAuthPolicySet::FAILED;
	}

	if (edh_init_keys_with_params(sc, g, p) != 0) {
		delete sc;
		delete options;
		delete p;
		delete g;
		return rina::IAuthPolicySet::FAILED;
	}

	//Add peer public key to security context
	sc->dh_peer_pub_key = BN_bin2bn(options->dh_public_key.array,
			       	        options->dh_public_key.length,
			       	        NULL);
	if (!sc->dh_peer_pub_key) {
		LOG_ERR("Error converting public key to a BIGNUM");
		delete sc;
		delete options;
		return rina::IAuthPolicySet::FAILED;
	}

	//Options is not needed anymore
	delete options;

	//Generate the shared secret
	if (edh_generate_shared_secret(sc) != 0) {
		delete sc;
		return rina::IAuthPolicySet::FAILED;
	}

	// TODO configure kernel SDU protection policy with shared secret
	// tell it to encrypt/decrypt all messages right after sending the next one

	// Prepare message for the peer, send selected algorithms and public key
	SSHRSAAuthOptions auth_options;
	auth_options.key_exch_algs.push_back(sc->key_exch_alg);
	auth_options.encrypt_algs.push_back(sc->encrypt_alg);
	auth_options.mac_algs.push_back(sc->mac_alg);
	auth_options.compress_algs.push_back(sc->compress_alg);
	auth_options.dh_public_key.length = BN_bn2bin(sc->dh_state->pub_key,
						      auth_options.dh_public_key.array);

	if (auth_options.dh_public_key.length <= 0) {
		LOG_ERR("Error transforming big number to binary");
		delete sc;
		return rina::IAuthPolicySet::FAILED;
	}

	SerializedObject * sobj = encode_ssh_rsa_auth_options(auth_options);
	if (!sobj) {
		LOG_ERR("Problems encoding SSHRSAAuthOptions");
		delete sc;
		return rina::IAuthPolicySet::FAILED;
	}

	sc->state = SSHRSASecurityContext::EDH_COMPLETED;
	sec_man->add_security_context(sc);

	//Send message to peer with selected algorithms and public key
	try {
		RIBObjectValue robject_value;
		robject_value.type_ = RIBObjectValue::bytestype;
		robject_value.bytes_value_ = *sobj;

		RemoteProcessId remote_id;
		remote_id.port_id_ = session_id;

		//object class contains challenge request or reply
		//object name contains cipher name
		rib_daemon->remoteWriteObject(EDH_EXCHANGE, EDH_EXCHANGE,
				robject_value, 0, remote_id, 0);
	} catch (Exception &e) {
		LOG_ERR("Problems encoding and sending CDAP message: %s", e.what());
		delete sc;
		delete sobj;
		return rina::IAuthPolicySet::FAILED;
	}

	delete sobj;

	return rina::IAuthPolicySet::IN_PROGRESS;
}

int AuthSSHRSAPolicySet::edh_init_keys_with_params(SSHRSASecurityContext * sc,
			      	      	           BIGNUM * g,
			      	      	           BIGNUM * p)
{
	DH *dh_state;
	int codes;

	/* Generate the parameters to be used */
	if ((dh_state = DH_new()) == NULL) {
		LOG_ERR("Error initializing Diffie-Hellman state");
		return -1;
	}

	dh_state->g = g;
	dh_state->p = p;

	if (DH_check(dh_state, &codes) != 1) {
		LOG_ERR("Error checking private key against parameters");
		DH_free(dh_state);
		return -1;
	}

	if (codes != 0)
	{
		LOG_ERR("Diffie-Hellman check has failed");
		DH_free(dh_state);
		return -1;
	}

	/* Generate the public and private key pair */
	if (DH_generate_key(dh_state) != 1) {
		LOG_ERR("Error generating public and private key pair");
		DH_free(dh_state);
		return -1;
	}

	sc->dh_state = dh_state;

	return 0;
}

int AuthSSHRSAPolicySet::edh_generate_shared_secret(SSHRSASecurityContext * sc)
{
	if((sc->shared_secret.array =
			(unsigned char*) OPENSSL_malloc(sizeof(unsigned char) * (DH_size(sc->dh_state)))) == NULL) {
		LOG_ERR("Error allocating memory for shared secret");
		return -1;
	}

	if((sc->shared_secret.length =
			DH_compute_key(sc->shared_secret.array, sc->dh_peer_pub_key, sc->dh_state)) < 0) {
		LOG_ERR("Error computing shared secret");
		OPENSSL_free(sc->shared_secret.array);
		return -1;
	}

	LOG_DBG("Computed shared secret: %s", sc->shared_secret.toString().c_str());

	return 0;
}

int AuthSSHRSAPolicySet::process_incoming_message(const CDAPMessage& message, int session_id)
{
	ScopedLock sc_lock(lock);

	if (message.obj_class_ == EDH_EXCHANGE) {
		return process_edh_exchange_message(message, session_id);
	}

	return rina::IAuthPolicySet::FAILED;
}

int AuthSSHRSAPolicySet::process_edh_exchange_message(const CDAPMessage& message, int session_id)
{
	ByteArrayObjectValue * bytes_value;
	SSHRSASecurityContext * sc;
	const SerializedObject * sobj;

	if (message.op_code_ != CDAPMessage::M_WRITE) {
		LOG_ERR("Wrong operation type");
		return IAuthPolicySet::FAILED;
	}

	if (message.obj_value_ == 0) {
		LOG_ERR("Null object value");
		return IAuthPolicySet::FAILED;
	}

	bytes_value = dynamic_cast<ByteArrayObjectValue *>(message.obj_value_);
	if (!bytes_value) {
		LOG_ERR("Object value of wrong type");
		return IAuthPolicySet::FAILED;
	}

	sobj = static_cast<const SerializedObject *>(bytes_value->get_value());
	SSHRSAAuthOptions * options = decode_ssh_rsa_auth_options(*sobj);
	if (!options) {
		LOG_ERR("Could not decode SSHARSA options");
		return rina::IAuthPolicySet::FAILED;
	}

	sc = dynamic_cast<SSHRSASecurityContext *>(sec_man->get_security_context(session_id));
	if (!sc) {
		LOG_ERR("Could not retrieve Security Context for session: %d", session_id);
		delete options;
		return IAuthPolicySet::FAILED;
	}

	if (sc->state != SSHRSASecurityContext::WAIT_EDH_EXCHANGE) {
		LOG_ERR("Wrong session state: %d", sc->state);
		sec_man->remove_security_context(session_id);
		delete sc;
		delete options;
		return IAuthPolicySet::FAILED;
	}

	//Add peer public key to security context
	sc->dh_peer_pub_key = BN_bin2bn(options->dh_public_key.array,
			       	        options->dh_public_key.length,
			       	        NULL);
	if (!sc->dh_peer_pub_key) {
		LOG_ERR("Error converting public key to a BIGNUM");
		delete sc;
		delete options;
		return rina::IAuthPolicySet::FAILED;
	}

	//Options is not needed anymore
	delete options;

	//Generate the shared secret
	if (edh_generate_shared_secret(sc) != 0) {
		delete sc;
		return rina::IAuthPolicySet::FAILED;
	}

	sc->state = SSHRSASecurityContext::EDH_COMPLETED;

	//TODO, configure the shared secret in the kernel SDU protection module

	//TODO continue with authentication

	return rina::IAuthPolicySet::IN_PROGRESS;
}

int AuthSSHRSAPolicySet::set_policy_set_param(const std::string& name,
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
        auth_policy_sets.put(auth_type, candidate);
        LOG_INFO("Authentication policy-set %s added to the security-manager",
                        auth_type.c_str());

        return 0;
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


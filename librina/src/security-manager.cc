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

	int size = gpb_options.ByteSize();
	char *serialized_message = new char[size];
	gpb_options.SerializeToArray(serialized_message, size);
	SerializedObject *object = new SerializedObject(serialized_message, size);

	return object;
}

//Class AuthSSHRSA
const int AuthSSHRSAPolicySet::DEFAULT_TIMEOUT = 10000;
const std::string AuthSSHRSAPolicySet::KEY_EXCHANGE_ALGORITHM = "keyExchangeAlg";
const std::string AuthSSHRSAPolicySet::ENCRYPTION_ALGORITHM = "encryptAlg";
const std::string AuthSSHRSAPolicySet::MAC_ALGORITHM = "macAlg";
const std::string AuthSSHRSAPolicySet::COMPRESSION_ALGORITHM = "compressAlg";

AuthSSHRSAPolicySet::AuthSSHRSAPolicySet(IRIBDaemon * ribd, ISecurityManager * sm) :
		IAuthPolicySet(IAuthPolicySet::AUTH_SSHRSA)
{
	rib_daemon = ribd;
	sec_man = sm;
	timeout = DEFAULT_TIMEOUT;
}

AuthPolicy AuthSSHRSAPolicySet::get_auth_policy(int session_id,
						const AuthSDUProtectionProfile& profile)
{
	if (profile.authPolicy.name_ != type) {
		LOG_ERR("Wrong policy name: %s", profile.authPolicy.name_.c_str());
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

	SSHRSAAuthOptions options;
	options.key_exch_algs.push_back(sc->key_exch_alg);
	options.encrypt_algs.push_back(sc->encrypt_alg);
	options.mac_algs.push_back(sc->mac_alg);
	options.compress_algs.push_back(sc->compress_alg);

	SerializedObject * sobj = encode_ssh_rsa_auth_options(options);
	if (!sobj) {
		LOG_ERR("Problems encoding SSHRSAAuthOptions");
		delete sc;
		throw Exception();
	}

	auth_policy.options_ = *sobj;
	delete sobj;

	//Store security context
	sec_man->add_security_context(sc);

	return auth_policy;
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


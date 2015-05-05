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

#define RINA_PREFIX "librina.security-manager"

#include "librina/logs.h"
#include "librina/security-manager.h"

namespace rina {

//Class AuthPolicySet
const std::string IAuthPolicySet::AUTH_NONE = "authentication-none";
const std::string IAuthPolicySet::AUTH_PASSWORD = "authentication-password";
const std::string IAuthPolicySet::AUTH_SSHRSA = "authentication-sshrsa";
const std::string IAuthPolicySet::AUTH_SSHDSA = "authentication-sshdsa";

IAuthPolicySet::IAuthPolicySet(CDAPMessage::AuthTypes type_)
{
	type = cdapTypeToString(type_);
}

const std::string IAuthPolicySet::cdapTypeToString(CDAPMessage::AuthTypes type)
{
	switch(type) {
	case CDAPMessage::AUTH_NONE:
		return AUTH_NONE;
	case CDAPMessage::AUTH_PASSWD:
		return AUTH_PASSWORD;
	case CDAPMessage::AUTH_SSHRSA:
		return AUTH_SSHRSA;
	case CDAPMessage::AUTH_SSHDSA:
		return AUTH_SSHDSA;
	default:
		throw Exception("Unknown authentication type");
	}
}

//Class AuthNonePolicySet
rina::AuthValue AuthNonePolicySet::get_my_credentials(int session_id)
{
	(void) session_id;

	rina::AuthValue result;
	return result;
}

rina::IAuthPolicySet::AuthStatus AuthNonePolicySet::initiate_authentication(rina::AuthValue credentials,
								      	    int session_id)
{
	(void) credentials;
	(void) session_id;

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

//Class CancelPasswdAuthTimerTask
void CancelPasswdAuthTimerTask::run()
{
	ps->remove_session_info(session_id);
}

//Class AuthPasswdPolicySet
const std::string AuthPasswordPolicySet::PASSWORD = "password";
const std::string AuthPasswordPolicySet::DEFAULT_CIPHER = "default_cipher";
const std::string AuthPasswordPolicySet::CHALLENGE_REQUEST = "challenge request";
const std::string AuthPasswordPolicySet::CHALLENGE_REPLY = "challenge reply";
const int AuthPasswordPolicySet::DEFAULT_TIMEOUT = 10000;

AuthPasswordPolicySet::AuthPasswordPolicySet(const std::string password_,
			int challenge_length_, IRIBDaemon * ribd) :
		IAuthPolicySet(rina::CDAPMessage::AUTH_PASSWD)
{
	password = password_;
	challenge_length = challenge_length_;
	rib_daemon = ribd;
	cipher = DEFAULT_CIPHER;
	timeout = DEFAULT_TIMEOUT;
}

// No credentials required, since the process being authenticated
// will have to demonstrate that it knows the password by encrypting
// a random challenge with a password string
rina::AuthValue AuthPasswordPolicySet::get_my_credentials(int session_id)
{
	(void) session_id;

	rina::AuthValue result;
	return result;
}

std::string * AuthPasswordPolicySet::generate_random_challenge()
{
	static const char alphanum[] =
			"0123456789"
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"abcdefghijklmnopqrstuvwxyz";

	char *s = new char[challenge_length];

	for (int i = 0; i < challenge_length; ++i) {
		s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
	}

	s[challenge_length] = 0;

	std::string * result = new std::string(s);
	delete s;

	return result;
}

std::string AuthPasswordPolicySet::encrypt_challenge(const std::string& challenge)
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

std::string AuthPasswordPolicySet::decrypt_challenge(const std::string& encrypted_challenge)
{
	return encrypt_challenge(encrypted_challenge);
}

rina::IAuthPolicySet::AuthStatus AuthPasswordPolicySet::initiate_authentication(rina::AuthValue credentials,
								      	    int session_id)
{
	(void) credentials;
	(void) session_id;

	ScopedLock scopedLock(lock);

	//1 Generate a random password string and send it to the AP being authenticated
	std::string * challenge = generate_random_challenge();

	try {
		RIBObjectValue robject_value;
		robject_value.type_ = RIBObjectValue::stringtype;
		robject_value.string_value_ = *challenge;

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
	CancelPasswdAuthTimerTask * timer_task =
			new CancelPasswdAuthTimerTask(this, session_id);
	timer.scheduleTask(timer_task, timeout);

	AuthPasswordSessionInformation * session_info =
			new AuthPasswordSessionInformation(timer_task, challenge);
	pending_sessions.put(session_id, session_info);

	return rina::IAuthPolicySet::IN_PROGRESS;
}

void AuthPasswordPolicySet::remove_session_info(int session_id)
{
	ScopedLock scopedLock(lock);

	AuthPasswordSessionInformation * session_info =
			pending_sessions.erase(session_id);
	if (session_info) {
		delete session_info;
	}
}

int AuthPasswordPolicySet::process_challenge_request(const std::string& challenge,
						     int session_id)
{
	try {
		RIBObjectValue robject_value;
		robject_value.type_ = RIBObjectValue::stringtype;
		robject_value.string_value_ = encrypt_challenge(challenge);

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

	AuthPasswordSessionInformation * session_info = pending_sessions.erase(session_id);
	if (!session_info) {
		LOG_DBG("Could not find pending auth session information for session_id %d",
			session_id);
		return IAuthPolicySet::FAILED;
	}

	timer.cancelTask(session_info->timer_task);

	std::string recovered_challenge = decrypt_challenge(encrypted_challenge);
	if (*(session_info->challenge) == recovered_challenge) {
		result = IAuthPolicySet::SUCCESSFULL;
	} else {
		LOG_DBG("Authentication failed; challenge: %s; recovered_challenge: %s ",
				(session_info->challenge)->c_str(),
				recovered_challenge.c_str());
	}

	delete session_info;
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
	if (name == PASSWORD) {
		password = value;
		LOG_INFO("Updated password to: %s", password.c_str());
		return 0;
	}

        LOG_DBG("Unknown policy-set-specific parameters to set (%s, %s)",
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

IAuthPolicySet * ISecurityManager::get_auth_policy_set(const std::string& auth_type)
{
	return auth_policy_sets.find(auth_type);
}

ISecurityContext * ISecurityManager::get_security_context(int context_id)
{
	return security_contexts.find(context_id);
}

void ISecurityManager::eventHappened(InternalEvent * event)
{
	NMinusOneFlowDeallocatedEvent * n_event = 0;
	ISecurityContext * context = 0;

	if (event->type == InternalEvent::APP_N_MINUS_1_FLOW_DEALLOCATED) {
		n_event = dynamic_cast<NMinusOneFlowDeallocatedEvent *>(event);
		context = security_contexts.erase(n_event->cdap_session_descriptor_.port_id_);
		if (context) {
			delete context;
		}
	}
}

}


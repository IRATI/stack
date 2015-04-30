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

int AuthNonePolicySet::set_policy_set_param(const std::string& name,
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


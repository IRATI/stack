//
// Security Manager
//
//    Eduard Grasa <eduard.grasa@i2cat.net>
//    Vincenzo Maffione <v.maffione@nextworks.it>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#define IPCP_MODULE "security-manager"
#include "ipcp-logging.h"

#include "ipcp/components.h"

namespace rinad {

//Class SecurityManager
void IPCPSecurityManager::set_application_process(rina::ApplicationProcess * ap)
{
	if (!ap)
		return;

	app = ap;
	ipcp = dynamic_cast<IPCProcess*>(app);
	if (!ipcp) {
		LOG_IPCP_ERR("Bogus instance of IPCP passed, return");
		return;
	}
}

void IPCPSecurityManager::set_dif_configuration(const rina::DIFConfiguration& dif_configuration)
{
	config = dif_configuration.sm_configuration_;

	// If no policy set is specified, use default
	if (config.policy_set_.name_ == std::string()) {
		config.policy_set_.name_ = rina::IPolicySet::DEFAULT_PS_SET_NAME;
	}

	// If no default authentication policy is specified, use AUTH_NONE
	if (config.default_auth_profile.authPolicy.name_ == std::string()) {
		config.default_auth_profile.authPolicy.name_ = rina::IAuthPolicySet::AUTH_NONE;
		config.default_auth_profile.authPolicy.version_ = RINA_DEFAULT_POLICY_VERSION;
	}

	std::string ps_name = dif_configuration.sm_configuration_.policy_set_.name_;
	if (select_policy_set(std::string(), ps_name) != 0) {
		throw rina::Exception("Cannot create Security Manager policy-set");
	}

        add_auth_policy_set(rina::IAuthPolicySet::AUTH_NONE);
        add_auth_policy_set(rina::IAuthPolicySet::AUTH_PASSWORD);
        add_auth_policy_set(rina::IAuthPolicySet::AUTH_SSH2);
}

rina::AuthSDUProtectionProfile IPCPSecurityManager::get_auth_sdup_profile(const std::string& under_dif_name)
{
	std::map<std::string, rina::AuthSDUProtectionProfile>::const_iterator it =
			config.specific_auth_profiles.find(under_dif_name);
	if (it == config.specific_auth_profiles.end()) {
		return config.default_auth_profile;
	} else {
		return it->second;
	}
}

rina::IAuthPolicySet::AuthStatus IPCPSecurityManager::enable_encryption(const rina::EncryptionProfile& profile,
						   	   	        rina::IAuthPolicySet * caller)
{
	unsigned int handle;

	rina::ScopedLock sc_lock(lock);
	try{
		LOG_DBG("Requesting the kernel to enable encryption on port-id: %d",
			profile.port_id);
		handle = rina::kernelIPCProcess->enableEncryption(profile);
	} catch(rina::Exception &e) {
		return rina::IAuthPolicySet::FAILED;
	}

	pending_enable_encryption_requests[handle] = caller;

	return rina::IAuthPolicySet::IN_PROGRESS;
}

void IPCPSecurityManager::process_enable_encryption_response(const rina::EnableEncryptionResponseEvent& event)
{
	rina::IAuthPolicySet * caller;

	lock.lock();

	std::map<unsigned int, rina::IAuthPolicySet *>::iterator it =
			pending_enable_encryption_requests.find(event.sequenceNumber);
	if (it == pending_enable_encryption_requests.end()) {
		lock.unlock();
		LOG_WARN("Could not find pending request for enable encryption response event %u",
			  event.sequenceNumber);
		return;
	} else  {
		caller = it->second;
		pending_enable_encryption_requests.erase(it);
	}

	lock.unlock();

	if (event.result != 0) {
		LOG_ERR("Enabling flow encryption failed, authentication failed");
		destroy_security_context(event.port_id);
		ipcp->enrollment_task_->authentication_completed(event.port_id, false);
		return;
	}

	rina::IAuthPolicySet::AuthStatus status = caller->encryption_enabled(event.port_id);
	if (status == rina::IAuthPolicySet::FAILED) {
		ipcp->enrollment_task_->authentication_completed(event.port_id, false);
		return;
	} else if (status == rina::IAuthPolicySet::SUCCESSFULL) {
		ipcp->enrollment_task_->authentication_completed(event.port_id, true);
		return;
	}

	//Authentication is still in progress, do nothing
	return;
}

} //namespace rinad

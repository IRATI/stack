//
// Security Manager
//
//    Eduard Grasa <eduard.grasa@i2cat.net>
//    Vincenzo Maffione <v.maffione@nextworks.it>
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

	ipcp->rib_daemon_->set_security_manager(this);
}

void IPCPSecurityManager::set_dif_configuration(const rina::DIFInformation& dif_information)
{
	config = dif_information.dif_configuration_.sm_configuration_;

        // If no policy set is specified, use default
	if (config.policy_set_.name_ == std::string()) {
                LOG_DBG("IPCPSecurityManager: No policy Set specified, use default");
		config.policy_set_.name_ = rina::IPolicySet::DEFAULT_PS_SET_NAME;
	}

	// If no default authentication policy is specified, use AUTH_NONE
	if (config.default_auth_profile.authPolicy.name_ == std::string()) {
                LOG_DBG("IPCPSecurityManager: No Auth policy Set specified, use AUTH-NONE");
		config.default_auth_profile.authPolicy.name_ = rina::IAuthPolicySet::AUTH_NONE;
		config.default_auth_profile.authPolicy.version_ = RINA_DEFAULT_POLICY_VERSION;
	}

	std::string ps_name = dif_information.dif_configuration_.sm_configuration_.policy_set_.name_;
	if (select_policy_set(std::string(), ps_name) != 0) {
		throw rina::Exception("Cannot create Security Manager policy-set");
	}

	//Add the auth policy sets supported by the DIF configuration
        add_auth_policy_set(config.default_auth_profile.authPolicy.name_);
        std::map<std::string, rina::AuthSDUProtectionProfile>::const_iterator iterator;
        for (iterator = config.specific_auth_profiles.begin();
        		iterator != config.specific_auth_profiles.end();
        		++iterator) {
        	add_auth_policy_set(iterator->second.authPolicy.name_);
        }
}

IPCPSecurityManager::~IPCPSecurityManager()
{
	delete ps;
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

rina::IAuthPolicySet::AuthStatus IPCPSecurityManager::update_crypto_state(const rina::CryptoState& state,
						   	   	          rina::IAuthPolicySet * caller)
{
	unsigned int handle;

	rina::ScopedLock sc_lock(lock);
	try{
		LOG_DBG("Requesting the kernel to update crypto state on port-id: %d",
			state.port_id);
		handle = rina::kernelIPCProcess->updateCryptoState(state);
	} catch(rina::Exception &e) {
		return rina::IAuthPolicySet::FAILED;
	}

	pending_update_crypto_state_requests[handle] = caller;

	return rina::IAuthPolicySet::IN_PROGRESS;
}

void IPCPSecurityManager::process_update_crypto_state_response(const rina::UpdateCryptoStateResponseEvent& event)
{
	rina::IAuthPolicySet * caller;

	lock.lock();

        std::map<unsigned int, rina::IAuthPolicySet *>::iterator it =
			pending_update_crypto_state_requests.find(event.sequenceNumber);
	if (it == pending_update_crypto_state_requests.end()) {
		lock.unlock();
		LOG_WARN("Could not find pending request for update crypto state response event %u",
			  event.sequenceNumber);
		return;
	} else  {
		caller = it->second;
		pending_update_crypto_state_requests.erase(it);
	}

	lock.unlock();

	if (event.result != 0) {
		LOG_ERR("Update crypto state failed, authentication failed");
		destroy_security_context(event.port_id);
		ipcp->enrollment_task_->authentication_completed(event.port_id, false);
		return;
	}

	rina::IAuthPolicySet::AuthStatus status = caller->crypto_state_updated(event.port_id);
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

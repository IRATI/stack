//
// Common utilities for Key Manager and Key Management Agent
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

#ifndef KM_COMMON_HPP
#define KM_COMMON_HPP

#include <string>
#include <librina/security-manager.h>
#include <librina/timer.h>

class DummySecurityManagerPs : public rina::ISecurityManagerPs
{
public:
	DummySecurityManagerPs() {};
	~DummySecurityManagerPs() {};

	int isAllowedToJoinDAF(const rina::cdap_rib::con_handle_t & con,
			       const rina::Neighbor& newMember,
			       rina::cdap_rib::auth_policy_t & auth);
	int storeAccessControlCreds(const rina::cdap_rib::auth_policy_t & auth,
				    const rina::cdap_rib::con_handle_t & con);
	int getAccessControlCreds(rina::cdap_rib::auth_policy_t & auth,
			          const rina::cdap_rib::con_handle_t & con);
	void checkRIBOperation(const rina::cdap_rib::auth_policy_t & auth,
			       const rina::cdap_rib::con_handle_t & con,
			       const rina::cdap::cdap_m_t::Opcode opcode,
			       const std::string obj_name,
			       rina::cdap_rib::res_info_t& res);
	int set_policy_set_param(const std::string& name,
				 const std::string& value);
};

#endif//KM_COMMON_HPP

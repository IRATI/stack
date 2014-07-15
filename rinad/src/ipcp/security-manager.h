/*
 * Security Manager
 *
 *    Eduard Grasa <eduard.grasa@i2cat.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef IPCP_SECURITY_MANAGER_HH
#define IPCP_SECURITY_MANAGER_HH

#ifdef __cplusplus

#include "ipcp/components.h"

namespace rinad {

class SecurityManager: public ISecurityManager {
public:
	SecurityManager();
	void set_ipc_process(IPCProcess * ipc_process);
	void set_dif_configuration(const rina::DIFConfiguration& dif_configuration);
	bool isAllowedToJoinDIF(const rina::Neighbor& newMember);
	bool acceptFlow(const Flow& newFlow);

private:
	IPCProcess * ipc_process_;
};

}

#endif

#endif

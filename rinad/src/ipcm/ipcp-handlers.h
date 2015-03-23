/*
 * IPCP transaction states
 *
 *    Marc Sune <marc.sune (at) bisdn.de>
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

#ifndef __IPCP_HANDLERS_H__
#define __IPCP_HANDLERS_H__

#include <assert.h>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>
#include <utility>

#include <librina/common.h>
#include <librina/ipc-manager.h>
#include <librina/patterns.h>

#include "ipcm.h"

namespace rinad {

/**
* Standard IPCP related transaction state
*/
class IPCPTransState: public TransactionState{

public:
	IPCPTransState(const Addon* callee, int _ipcp_id)
					: TransactionState(callee),
						ipcp_id(_ipcp_id){}
	virtual ~IPCPTransState(){};

	//IPCP identifier
	int ipcp_id;
};

/**
* IPCP registration transaction state
*/
class IPCPregTransState: public TransactionState{

public:
	IPCPregTransState(const Addon* callee, int _ipcp_id,
					int _slave_ipcp_id) :
					TransactionState(callee),
					ipcp_id(_ipcp_id),
					slave_ipcp_id(_slave_ipcp_id){}
	virtual ~IPCPregTransState(){};

	//IPCP identifier
	int ipcp_id;
	int slave_ipcp_id;
};

}//rinad namespace

#endif  /* __IPCP_HANDLERS_H__ */

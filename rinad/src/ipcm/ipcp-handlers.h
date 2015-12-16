/*
 * IPCP transaction states
 *
 *    Marc Sune <marc.sune (at) bisdn.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
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
#include <librina/application.h>
#include <librina/ipc-manager.h>
#include <librina/patterns.h>

#include "ipcm.h"

namespace rinad {

/**
* Standard IPCP related transaction state
*/
class IPCPTransState: public TransactionState{

public:
	IPCPTransState(Addon* callee, Promise* promise, int _ipcp_id)
					: TransactionState(callee, promise),
						ipcp_id(_ipcp_id){}
	virtual ~IPCPTransState(){};

	//IPCP identifier
	int ipcp_id;
};

/**
* IPCP registration transaction state
*/
class IPCPregTransState: public IPCPTransState {

public:
	IPCPregTransState(Addon* callee, Promise* promise, int _ipcp_id,
					int _slave_ipcp_id) :
					IPCPTransState(callee, promise,
								_ipcp_id),
					slave_ipcp_id(_slave_ipcp_id){}
	virtual ~IPCPregTransState(){};

	//IPCP identifier
	int slave_ipcp_id;
};

/**
* IPCP plugin load transaction state
*/
class IPCPpluginTransState: public TransactionState{

public:
	IPCPpluginTransState(Addon* callee, Promise* promise, int _ipcp_id,
			   const std::string& _name, bool _l)
					: TransactionState(callee, promise),
					  ipcp_id(_ipcp_id),
					  plugin_name(_name), load(_l) {}
	virtual ~IPCPpluginTransState(){};

	// IPCP identifier
	int ipcp_id;

	// Plugin name
	std::string plugin_name;

	// Is this a load or unload operation ?
	bool load;
};

/**
* IPCP select-policy-set transaction state
*/
class IPCPSelectPsTransState: public TransactionState{

public:
	IPCPSelectPsTransState(Addon* callee, Promise* promise,
			       unsigned _ipcp_id,
			       const rina::PsInfo& _ps_info, int _id)
					: TransactionState(callee, promise),
					  ipcp_id(_ipcp_id),
					  ps_info(_ps_info), id(_id) { }
	virtual ~IPCPSelectPsTransState(){};

	// IPCP identifier
	unsigned int ipcp_id;

	// The policy set to be selected
	rina::PsInfo ps_info;

	// IPCP or port-id identifier (to be used for the catalog)
	unsigned int id;
};

}//rinad namespace

#endif  /* __IPCP_HANDLERS_H__ */

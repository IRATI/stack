/*
 * MISC transaction states
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

#ifndef __MISC_H__
#define __MISC_H__

#include <assert.h>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>
#include <utility>

#include <librina/common.h>
#include <librina/ipc-manager.h>
#include <librina/patterns.h>

#include "ipcp-handlers.h"

namespace rinad {

/**
* RIB query transaction state
*/
class RIBqTransState: public IPCPTransState{

public:
	RIBqTransState(Addon* callee, Promise* _promise, int _ipcp_id)
				:IPCPTransState(callee, _promise, _ipcp_id){}
	virtual ~RIBqTransState(){};

	//Output result
	std::string result;
};

}//rinad namespace

#endif  /* __MISC_H__ */

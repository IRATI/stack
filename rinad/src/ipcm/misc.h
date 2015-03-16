/*
 * MISC transaction states
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

#include "ipcp.h"

namespace rinad {

/**
* RIB query transaction state
*/
class RIBqTransState: public IPCPTransState{

public:
	RIBqTransState(const Addon* _callee, const int _tid, int _ipcp_id)
					:IPCPTransState(_callee, tid,
								_ipcp_id){}
	virtual ~RIBqTransState(){};

	//Output result
	std::string result;
};

}//rinad namespace

#endif  /* __MISC_H__ */

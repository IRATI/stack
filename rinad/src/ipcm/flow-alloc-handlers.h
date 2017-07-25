/*
 * FLOW_ALLOC transaction states
 *
 *    Marc Sune <marc.sune (at) bisdn.de>
 *    Eduard Grasa          <eduard.grasa@i2cat.net>
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

#ifndef __FLOW_ALLOC_H__
#define __FLOW_ALLOC_H__

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
* Flow allocation transaction state
*/
class FlowAllocTransState: public TransactionState{

public:
	FlowAllocTransState(Addon* callee, Promise* promise, int _slave_ipcp_id,
				rina::FlowRequestEvent& _req_e,
				bool once):
					TransactionState(callee, promise),
					slave_ipcp_id(_slave_ipcp_id),
					req_event(_req_e),
					try_only_a_dif(once){ }
	virtual ~FlowAllocTransState(){};

        int slave_ipcp_id;
        rina::FlowRequestEvent req_event;
        bool try_only_a_dif;
};

}//rinad namespace

#endif  /* __FLOW_ALLOC_H__ */

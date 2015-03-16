/*
 * FLOW_ALLOC transaction states
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
	FlowAllocTransState(const Addon* callee, const int tid,
				int _slave_ipcp_id,
				rina::FlowRequestEvent& _req_e,
				bool once):
					TransactionState(callee, tid),
					slave_ipcp_id(_slave_ipcp_id),
					req_event(_req_e),
					try_only_a_dif(once)
					{}
	virtual ~FlowAllocTransState(){};

        int slave_ipcp_id;
        rina::FlowRequestEvent req_event;
        bool try_only_a_dif;
};

}//rinad namespace

#endif  /* __FLOW_ALLOC_H__ */

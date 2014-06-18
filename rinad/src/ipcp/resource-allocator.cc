//
// Resource Allocator
//
//    Bernat Gaston <bernat.gaston@i2cat.net>
//    Eduard Grasa <eduard.grasa@i2cat.net>
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

#include <sstream>

#include "resource-allocator.h"

namespace rinad {

//Class NMinusOneFlowManager
NMinusOneFlowManager::NMinusOneFlowManager() {
	rib_daemon_ = 0;
	ipc_process_ = 0;
	cdap_session_manager_ = 0;
}

void NMinusOneFlowManager::set_ipc_process(IPCProcess * ipc_process) {
	ipc_process_ = ipc_process;
	rib_daemon_ = ipc_process->get_rib_daemon();
	cdap_session_manager_ = ipc_process->get_cdap_session_manager();
}

void NMinusOneFlowManager::populateRIB(){
}

}

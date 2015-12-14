/*
 * IPC Manager console
 *
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
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

#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <sstream>

#define RINA_PREFIX     "ipcm.scripting"
#include <librina/common.h>
#include <librina/ipc-manager.h>
#include <librina/logs.h>

#include "../ipcm.h"
#include "rina-configuration.h"
#include "scripting.h"

using namespace std;


namespace rinad {

//Static members
const std::string ScriptingEngine::NAME = "scripting";

ScriptingEngine::ScriptingEngine() : Addon(ScriptingEngine::NAME){

	//TODO: create another thread when necessary

	LOG_DBG("Starting...");

	IPCManager->apply_configuration();

	LOG_DBG("Finalizing...");
}

ScriptingEngine::~ScriptingEngine(){

}

} //namespace rinad

/*
 * Background Task Manager
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

#include "bgtm.h"

#include <signal.h>
#include <unistd.h>

#define RINA_PREFIX "ipcm.mad.bg"
#include <librina/logs.h>
#include <flowm.h>

namespace rinad {
namespace mad {

/**
* Signal handlers
*/

/*
//Handler to stop ciosrv
void interrupt_handler(int sig) {
	//Signal background task manager to stop
	flow_manager->stopIOLoop();
}
*/

//Main I/O loop
void* BGTaskManager::run(void* unused){

	while(keep_running){

		//TODO: launch monitoring and other bg tasks

		sleep(1); //XXX: remove
		//fprintf(stderr, "Iteration\n");
	}

	return NULL;
}

//Constructors destructors(singleton)
BGTaskManager::BGTaskManager():keep_running(true){
	//Program signal handlee (capture control+C)
	//signal(SIGINT, interrupt_handler);
	LOG_DBG("Initialized");
}


BGTaskManager::~BGTaskManager(void){

}

}; //namespace mad
}; //namespace rinad

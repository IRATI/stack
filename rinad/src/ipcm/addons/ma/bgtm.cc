/*
 * Background Task Manager
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

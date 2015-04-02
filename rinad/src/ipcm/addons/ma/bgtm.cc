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
	(void) sig;
	//Signal background task manager to stop
	flow_manager->stopIOLoop();
}
*/

//Main I/O loop
void* BGTaskManager::run(void* unused){

	(void)unused;

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

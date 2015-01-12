#include "bgtm.h"

#include <signal.h>
#include <unistd.h>

#define RINA_PREFIX "mad.bg"
#include <librina/logs.h>

namespace rinad {
namespace mad {

//Singleton instance
Singleton<BGTaskManager_> BGTaskManager;


/**
* Signal handlers
*/

//Handler to stop ciosrv
void interrupt_handler(int sig) {
	(void) sig;
	//Signal background task manager to stop
	BGTaskManager->keep_running = false;
}

//Main I/O loop
void* BGTaskManager_::run(void* unused){

	(void)unused;

	while(keep_running){

		//TODO: launch monitoring and other bg tasks

		sleep(1); //XXX: remove
		fprintf(stderr, "Iteration\n");
	}

	return NULL;
}

//Public init/destroy APIs
void BGTaskManager_::init(){
	//Program signal handlee (capture control+C)
	signal(SIGINT, interrupt_handler);
}

void BGTaskManager_::destroy(){
}

//Constructors destructors(singleton)
BGTaskManager_::BGTaskManager_():keep_running(true){

	//Program signal handlee (capture control+C)
	signal(SIGINT, interrupt_handler);
}


BGTaskManager_::~BGTaskManager_(void){

}

}; //namespace mad
}; //namespace rinad

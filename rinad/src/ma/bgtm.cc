#include "bgtm.h"

#include <signal.h>
#include <unistd.h>

#define RINA_PREFIX "mad.bg"
#include <librina/logs.h>

namespace rinad {
namespace mad {

//Static initializations
BGTaskManager* BGTaskManager::inst = NULL;


/**
* Signal handlers
*/
//Handler to stop ciosrv
void interrupt_handler(int sig) {
	(void) sig;
	//Signal background task manager to stop
	BGTaskManager::get()->keep_running = false;
}

/**
* Background Task manager
*/
void BGTaskManager::init(){

	//Create instance
	if(inst){
		throw Exception(
			"Invalid double call to BGTaskManager::init()");	
	}
	inst = new BGTaskManager();

	//Program signal handlee (capture control+C)
	signal(SIGINT, interrupt_handler);
	
	//Run
	inst->run(NULL);
}

void BGTaskManager::destroy(void){
	delete BGTaskManager::inst;
}

//Constructors destructors
BGTaskManager::BGTaskManager(){
	
}

void* BGTaskManager::run(void* unused){

	(void)unused;
	keep_running = true;

	while(keep_running){

		//TODO: launch monitoring and other bg tasks		

		sleep(1); //XXX: remove
		fprintf(stderr, "Iteration\n");	
	}

	return NULL;
}


BGTaskManager::~BGTaskManager(void){
	
}

}; //namespace mad 
}; //namespace rinad

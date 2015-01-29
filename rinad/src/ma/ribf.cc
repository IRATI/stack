#include "ribf.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "agent.h"
#define RINA_PREFIX "mad.ribf"
#include <librina/logs.h>

#include "ribs/ribd_v1.h"

namespace rinad{
namespace mad{

/*
* RIBFactory
*/

//Singleton instance
Singleton<RIBFactory_> RIBFactory;


//Initialization and destruction routines
void RIBFactory_::init(std::list<uint64_t> supported_versions){
	for (std::list<uint64_t>::iterator it = supported_versions.begin();
			it != supported_versions.end(); it++)
	{
		createRIB(*it);
	}

	LOG_DBG("Initialized");
}

void RIBFactory_::destroy(void){

}



//Constructors destructors
RIBFactory_::RIBFactory_(){
	//TODO: register to flow events in librina and spawn workers
}

RIBFactory_::~RIBFactory_() throw (){}

/*
* Inner API
*/
void RIBFactory_::createRIB(uint64_t version){
	//Serialize
	lock();

	//Check if it exists
	if( rib_inst.find(version) != rib_inst.end() ){
		unlock();
		throw eDuplicatedRIB("An instance of the RIB with this version already exists");
	}

	//Create object
	switch(version)
	{
	case 1:
		rib_inst[version] = new RIBDaemonv1(0);
		break;
	default:
		break;
	}

	//Unlock
	unlock();
}

rina::IRIBDaemon& RIBFactory_::getRIB(uint64_t version){

	rina::IRIBDaemon* rib;

	//Serialize
	lock();

	//Note: it is safe to recover the RIB reference without a RD lock
	//because removal of RIBs is NOT implemented. However this
	//implementation already protects it
	//Check if it exists
	if( rib_inst.find(version) == rib_inst.end() ){
		throw eRIBNotFound("RIB instance not found");
	}

	//TODO: reference count to avoid deletion while being used?

	rib = rib_inst[version];

	//Unlock
	unlock();

	return *rib;
}


}; //namespace mad
}; //namespace rinad



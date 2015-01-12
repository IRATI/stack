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

//Constructors destructors
RIBFactory_::RIBFactory_(){
	//TODO: register to flow events in librina and spawn workers
	pthread_rwlock_init(&rwlock, NULL);
}

RIBFactory_::~RIBFactory_(){
	pthread_rwlock_destroy(&rwlock);
}

/*
* Inner API
*/
rina::IRIBDaemon& RIBFactory_::createRIB(uint64_t version){

	rina::IRIBDaemon* rib;

	//Serialize
	pthread_rwlock_wrlock(&rwlock);

	//Check if it exists
	if( rib_inst.find(version) != rib_inst.end() ){
		pthread_rwlock_unlock(&rwlock);
		throw eDuplicatedRIB("An instance of the RIB with this version already exists");
	}

	//Create object
	rib = new RIBDaemonv1();

	//TODO: initialize further?
	rib_inst[version] = rib;

	//Unlock
	pthread_rwlock_unlock(&rwlock);

	return *rib;
}

rina::IRIBDaemon& RIBFactory_::getRIB(uint64_t version){

	rina::IRIBDaemon* rib;

	//Serialize
	pthread_rwlock_rdlock(&rwlock);

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
	pthread_rwlock_unlock(&rwlock);

	return *rib;
}


}; //namespace mad
}; //namespace rinad



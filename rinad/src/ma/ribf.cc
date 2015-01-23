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
void RIBFactory_::init(void){
	try
	{
		rib_daemon_->addRIBObject(new SimplestRIBObj(rib_daemon_, "ROOT", "root"));
		rib_daemon_->addRIBObject(new SimpleRIBObj(rib_daemon_, "DAF", "root, dafID", 1));
		rib_daemon_->addRIBObject(new SimpleRIBObj(rib_daemon_, "ComputingSystem", "root, computingSystemID", 1));
		rib_daemon_->addRIBObject(new SimpleRIBObj(rib_daemon_, "ProcessingSystem", "root, computingSystemID = 1, processingSystemID", 1));
		rib_daemon_->addRIBObject(new SimplestRIBObj(rib_daemon_, "Software", "root, computingSystemID = 1, processingSystemID=1, software"));
		rib_daemon_->addRIBObject(new SimplestRIBObj(rib_daemon_, "Hardware", "root, computingSystemID = 1, processingSystemID=1, hardware"));
		rib_daemon_->addRIBObject(new SimplestRIBObj(rib_daemon_, "KernelApplicationProcess", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess"));
		rib_daemon_->addRIBObject(new SimplestRIBObj(rib_daemon_, "OSApplicationProcess", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess"));
		rib_daemon_->addRIBObject(new SimpleRIBObj(rib_daemon_, "ManagementAgent", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID", 1));
		// IPCManagement branch
		rib_daemon_->addRIBObject(new SimplestRIBObj(rib_daemon_, "IPCManagement", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement"));
		rib_daemon_->addRIBObject(new SimplestRIBObj(rib_daemon_, "IPCResourceManager", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement, "
				"ipcResourceManager"));
		rib_daemon_->addRIBObject(new SimplestRIBObj(rib_daemon_, "UnderlayingFlows", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement, "
				"ipcResourceManager, underlayingFlows"));
		rib_daemon_->addRIBObject(new SimplestRIBObj(rib_daemon_, "UnderlayingDIFs", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement, "
				"ipcResourceManager, underlayingDIFs"));
		rib_daemon_->addRIBObject(new SimplestRIBObj(rib_daemon_, "QueryDIFAllocator", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement, "
				"ipcResourceManager, queryDIFAllocator"));
		rib_daemon_->addRIBObject(new SimplestRIBObj(rib_daemon_, "UnderlayingRegistrations", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement, "
				"ipcResourceManager, underlayingRegistrations"));
		rib_daemon_->addRIBObject(new SimplestRIBObj(rib_daemon_, "SDUPRotection", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement, "
				"sduProtection"));
		// RIBDaemon branch
		rib_daemon_->addRIBObject(new SimplestRIBObj(rib_daemon_, "RIBDaemon", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ribDaemon"));
		rib_daemon_->addRIBObject(new SimplestRIBObj(rib_daemon_, "Discriminators", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ribDaemon"
				", discriminators"));
		// DIFManagement
		rib_daemon_->addRIBObject(new SimplestRIBObj(rib_daemon_, "DIFManagement", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, difManagement"));
	}
	catch(Exception &e1)
	{
		LOG_ERR("RIB basic objects were not created because %s", e1.what());
		throw Exception("Finish application");
	}
	LOG_DBG("Initialized");
}

void RIBFactory_::destroy(void){

}



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



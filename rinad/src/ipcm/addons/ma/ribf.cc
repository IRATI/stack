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
#define RINA_PREFIX "ipcm.mad.ribf"
#include <librina/logs.h>

#include "ribs/ribd_v1.h"

namespace rinad{
namespace mad{

/*
* RIBFactory
*/


//Constructors destructors
RIBFactory::RIBFactory(std::list<uint64_t> supported_versions){
	for (std::list<uint64_t>::iterator it = supported_versions.begin();
			it != supported_versions.end(); it++)
	{
		createRIB(*it);
	}

	LOG_DBG("Initialized");

}

RIBFactory::~RIBFactory() throw (){}

/*
* Inner API
*/
void RIBFactory::createRIB(uint64_t version){
  char separator = ',';
  rina::cdap_rib::cdap_params_t *params = new rina::cdap_rib::cdap_params_t;
  params->is_IPCP_ = false;
  params->timeout_ = 2000;

  rina::cdap_rib::vers_info_t *vers = new rina::cdap_rib::vers_info_t;
  vers->version_ = (long) version;

	//Serialize
	lock();

	//Check if it exists
	if( rib_inst_.find(version) != rib_inst_.end() ){
		unlock();
		throw eDuplicatedRIB("An instance of the RIB with this version already exists");
	}

	//Create object
	switch(version)
	{
	case 1:
		rib_inst_[version] = factory_.create(RIBDaemonv1->getConnHandler(), RIBDaemonv1->getRespHandler(), params, vers, separator);
		break;
	default:
		break;
	}

	//Unlock
	unlock();
}

rina::rib::RIBDNorthInterface& RIBFactory::getRIB(uint64_t version){

  rina::rib::RIBDNorthInterface* rib;

	//Serialize
	lock();

	//Note: it is safe to recover the RIB reference without a RD lock
	//because removal of RIBs is NOT implemented. However this
	//implementation already protects it
	//Check if it exists
	if( rib_inst_.find(version) == rib_inst_.end() ){
		throw eRIBNotFound("RIB instance not found");
	}

	//TODO: reference count to avoid deletion while being used?

	rib = rib_inst_[version];

	//Unlock
	unlock();

	return *rib;
}


}; //namespace mad
}; //namespace rinad



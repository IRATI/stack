/*
 * Flow Manager
 *
 *    Bernat Gaston         <bernat.gaston@i2cat.net>
 *    Marc Sune             <marc.sune (at) bisdn.de>
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

using namespace rina;
using namespace rina::rib;


namespace rinad {
namespace mad {

//static (unique) ribd proxy ref
rina::rib::RIBDaemonProxy* RIBFactory::ribd = NULL;

/*
 * RIBFactory
 */

//Constructors destructors
RIBFactory::RIBFactory(RIBAEassoc ver_assoc){

	rib_handle_t rib_handle;
	RIBAEassoc::const_iterator it;

	for (it = ver_assoc.begin();
			it != ver_assoc.end(); ++it) {
		uint64_t ver = it->first;
		const std::list<std::string>& aes = it->second;

		//If schema for this version does not exist
		//create it
		if()
			createSchema(ver);

		//Create RIB
		rib_handle = createRIB(ver);

		//Register to all AEs
		std::list<std::string>::const_iterator itt;
		for(itt = aes.begin(); itt != aes.end(); ++itt)
			ribd->associateRIBtoAE(rib_handle, *itt);
	}

	LOG_DBG("Initialized");

}

RIBFactory::~RIBFactory() throw (){
	// FIXME destroy con handlers and resp handlers
}

/*
 * Internal API
 */
void RIBFactory::createSchema(uint64_t version){
	cdap_rib::vers_info_t ver;
	ver.ver_ = version;

	//TODO: we are lazy here... there should be an "exists" call
	//for the schema
	try{
		ribd->createSchema(ver);
	}catch(){

	}	
}

rib_handle_t RIBFactory::createRIB(uint64_t version){

	rina::cdap_rib::cdap_params_t params;
	rina::cdap_rib::vers_info_t vers;
	vers->version_ = (long) version;

	//Scoped lock
	rina::ScopedLock slock(mutex);

	//Create the RIB
	rib_handle_t handle = ribd->createRIB(vers);

	//Create object
	switch(version) {
		case 0x1ULL:
			//Initialize
			rib_v1::initRIB(handle);
			break;
		default:
			assert(0); //We cannot reach this point
			break;
	}

	return handle;
}

static rina::rib::rib_handle_t RIBFactory::getRIBHandle(
						const uint64_t& version,
						const std::string& ae_name){
	rina::cdap_rib::vers_info_t vers;
	vers->version_ = (long) version;

	return ribd->get(vers, ae_name);
}

//Process IPCP create event to all RIB versions
void RIBFactory::createIPCPevent(int ipcp_id){

}

//Process IPCP create event to all RIB versions
void RIBFactory::destroyIPCPevent(int ipcp_id){

}


};//namespace mad
};//namespace rinad

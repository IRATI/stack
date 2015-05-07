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

namespace rinad {
namespace mad {

/*
 * RIBFactory
 */

//Constructors destructors
RIBFactory::RIBFactory(std::list<uint64_t> supported_versions){

	for (std::list<uint64_t>::iterator it = supported_versions.begin();
			it != supported_versions.end(); it++) {
		createRIB(*it);
	}

	LOG_DBG("Initialized");

}

RIBFactory::~RIBFactory() throw (){

	// FIXME destroy con handlers and resp handlers
}

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

	//Scoped lock
	rina::ScopedLock slock(mutex);

	//Check if it exists
	if (rib_inst_.find(version) != rib_inst_.end())
		throw eDuplicatedRIB(
				"An instance of the RIB with this version already exists");

	//Create object
	switch (version) {
		case 1:
			rib_inst_[version] = factory_.create(new rib_v1::RIBConHandler_v1(),
					new rib_v1::RIBRespHandler_v1(),
					params, vers, separator);
			rib_v1::initiateRIB(rib_inst_[version]);
			break;
		default:
			break;
	}
}

rina::rib::RIBDNorthInterface& RIBFactory::getRIB(uint64_t version){


	rina::rib::RIBDNorthInterface* rib;

	//Scoped lock
	rina::ScopedLock slock(mutex);

	//Note: it is safe to recover the RIB reference without a RD lock
	//because removal of RIBs is NOT implemented. However this
	//implementation already protects it
	//Check if it exists
	if (rib_inst_.find(version) == rib_inst_.end())
		throw eRIBNotFound("RIB instance not found");

	//TODO: reference count to avoid deletion while being used?

	rib = rib_inst_[version];

	return *rib;
}

};//namespace mad
};//namespace rinad

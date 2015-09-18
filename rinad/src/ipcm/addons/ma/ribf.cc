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
RIBConHandler empty_conn_callbacks;
RIBRespHandler empty_rib_callbacks;

/*
 * RIBFactory
 */

//Constructors destructors
RIBFactory::RIBFactory(RIBAEassoc ver_assoc){

	cdap_rib::cdap_params params;
	rib_handle_t rib_handle;
	RIBAEassoc::const_iterator it;
	std::list<std::string>::const_iterator itt;


	//First initialize the RIB library
	rina::rib::init(&empty_conn_callbacks, &empty_rib_callbacks, params);

	for (it = ver_assoc.begin();
			it != ver_assoc.end(); ++it) {
		uint64_t ver = it->first;
		const std::list<std::string>& aes = it->second;

		//If schema for this version does not exist
		switch(ver) {
			case 0x1ULL:
				{
				//Create the schema FIXME there should be an
				//exists
				try{
					rib_v1::createSchema();
				}catch(eSchemaExists& s){}

				//Create the RIB
				rib_handle = rib_v1::createRIB();

				//Register to all AEs
				for(itt = aes.begin(); itt != aes.end(); ++itt)
					rib_v1::associateRIBtoAE(rib_handle,
									*itt);

				//Store the version
				ribs[rib_handle] = ver;

				}
				break;
			default:
				assert(0);
				throw rina::Exception("Invalid version");
		}
	}

	LOG_DBG("Initialized");

}

RIBFactory::~RIBFactory() throw (){
	// FIXME destroy con handlers and resp handlers
}

/*
 * Internal API
 */

rina::rib::rib_handle_t RIBFactory::getRIBHandle(
						const uint64_t& version,
						const std::string& ae_name){
	rina::cdap_rib::vers_info_t vers;
	vers.version_ = (long) version;

	return ribd->get(vers, ae_name);
}

//Process IPCP create event to all RIB versions
void RIBFactory::createIPCPevent(int ipcp_id){

	std::map<rib_handle_t, uint64_t>::const_iterator it;

	for(it = ribs.begin(); it != ribs.end(); ++it){
		switch(it->second){
			case 0x1ULL:
				rib_v1::createIPCPObj(it->first, ipcp_id);
				break;
			default:
				assert(0);
				break;
		}
	}
}

//Process IPCP create event to all RIB versions
void RIBFactory::destroyIPCPevent(int ipcp_id){

	std::map<rib_handle_t, uint64_t>::const_iterator it;

	for(it = ribs.begin(); it != ribs.end(); ++it){
		switch(it->second){
			case 0x1ULL:
				rib_v1::destroyIPCPObj(it->first, ipcp_id);
				break;
			default:
				assert(0);
				break;
		}
	}
}

void RIBConHandler::connect(int message_id,
				const rina::cdap_rib::con_handle_t &con) {
	(void) message_id;
	(void) con;
}

void RIBConHandler::connectResult(
		const rina::cdap_rib::res_info_t &res,
		const rina::cdap_rib::con_handle_t &con) {
	(void) res;
	(void) con;
}
void RIBConHandler::release(int message_id,
				const rina::cdap_rib::con_handle_t &con) {
	(void) message_id;
	(void) con;
}
void RIBConHandler::releaseResult(
		const rina::cdap_rib::res_info_t &res,
		const rina::cdap_rib::con_handle_t &con) {
	(void) res;
	(void) con;
}

//RIB ops response
void RIBRespHandler::remoteCreateResult(
			const rina::cdap_rib::con_handle_t &con,
			const rina::cdap_rib::obj_info_t &obj,
			const rina::cdap_rib::res_info_t &res){
	(void) res;
	(void) con;
	(void) obj;
}
void RIBRespHandler::remoteDeleteResult(
			const rina::cdap_rib::con_handle_t &con,
			const rina::cdap_rib::res_info_t &res){
	(void) res;
	(void) con;
}

void RIBRespHandler::remoteReadResult(
			const rina::cdap_rib::con_handle_t &con,
			const rina::cdap_rib::obj_info_t &obj,
			const rina::cdap_rib::res_info_t &res){
	(void) res;
	(void) con;
	(void) obj;
}
void RIBRespHandler::remoteCancelReadResult(
			const rina::cdap_rib::con_handle_t &con,
			const rina::cdap_rib::res_info_t &res){
	(void) res;
	(void) con;
}
void RIBRespHandler::remoteWriteResult(
			const rina::cdap_rib::con_handle_t &con,
			const rina::cdap_rib::obj_info_t &obj,
			const rina::cdap_rib::res_info_t &res){
	(void) res;
	(void) con;
	(void) obj;
}
void RIBRespHandler::remoteStartResult(
			const rina::cdap_rib::con_handle_t &con,
			const rina::cdap_rib::obj_info_t &obj,
			const rina::cdap_rib::res_info_t &res){
	(void) res;
	(void) con;
	(void) obj;
}
void RIBRespHandler::remoteStopResult(
			const rina::cdap_rib::con_handle_t &con,
			const rina::cdap_rib::obj_info_t &obj,
			const rina::cdap_rib::res_info_t &res){
	(void) res;
	(void) con;
	(void) obj;
}


};//namespace mad
};//namespace rinad

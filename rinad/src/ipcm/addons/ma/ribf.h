/*
 * RIB factory
 *
 *    Bernat Gaston         <bernat.gaston@i2cat.net>
 *    Marc Sune             <marc.sune (at) bisdn.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

#ifndef __RINAD_RIBM_H__
#define __RINAD_RIBM_H__

#include <list>
#include <librina/concurrency.h>
#include <inttypes.h>
#include <librina/rib_v2.h>

namespace rinad{
namespace mad{

/**
 * @file ribf.h
 * @author Marc Sune<marc.sune (at) bisdn.de>
 *
 * @brief RIB Factory
 */

//
// RIB factory exceptions
//

/**
 * Duplicated RIB exception
 */
DECLARE_EXCEPTION_SUBCLASS(eDuplicatedRIB);

/**
 * RIB not found exception
 */
DECLARE_EXCEPTION_SUBCLASS(eRIBNotFound);

// Helper type
typedef std::map<uint64_t, std::list<std::string> > RIBAEassoc;

/**
 * @brief RIB manager
 */
class RIBFactory{

public:
	//Constructors
	RIBFactory(RIBAEassoc ver_assoc);
	virtual ~RIBFactory(void) throw();


	/**
	* Get the RIB provider proxy
	*/
	static inline rina::rib::RIBDaemonProxy* getProxy(){
		if(!ribd)
			ribd = rina::rib::RIBDaemonProxyFactory();
		return ribd;
	}

	/**
	* Get the handle of the specific RIB and AE name associated
	*/
	static rina::rib::rib_handle_t getRIBHandle(
						const uint64_t& version,
						const std::string& AEname);

	//Process IPCP create event to all RIB versions
	void createIPCPevent(int ipcp_id);

	//Process IPCP create event to all RIB versions
	void destroyIPCPevent(int ipcp_id);


protected:
	//Mutex
	rina::Lockable mutex;

	//Map handle <-> version
	std::map<rina::rib::rib_handle_t, uint64_t> ribs;

	//RIBProxy instance
	static rina::rib::RIBDaemonProxy* ribd;
};

class RIBConHandler : public rina::cacep::AppConHandlerInterface {

public:
	void connect(const rina::cdap::CDAPMessage& message,
		     const rina::cdap_rib::con_handle_t &con);
	void connectResult(const rina::cdap_rib::res_info_t &res,
			   const rina::cdap_rib::con_handle_t &con);
	void release(int message_id, const rina::cdap_rib::con_handle_t &con);
	void releaseResult(const rina::cdap_rib::res_info_t &res,
			  const rina::cdap_rib::con_handle_t &con);
	void process_authentication_message(const rina::cdap::CDAPMessage& message,
					    const rina::cdap_rib::con_handle_t &con);
};

}; //namespace mad
}; //namespace rinad

#endif  /* __RINAD_RIBM_H__ */

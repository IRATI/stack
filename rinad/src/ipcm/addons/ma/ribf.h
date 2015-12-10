/*
 * RIB factory
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

//
// Empty callbacks
//

class RIBRespHandler : public rina::rib::RIBOpsRespHandlers {
public:
	~RIBRespHandler(){};

	void remoteCreateResult(const rina::cdap_rib::con_handle_t &con,
			const rina::cdap_rib::obj_info_t &obj,
			const rina::cdap_rib::res_info_t &res);
	void remoteDeleteResult(const rina::cdap_rib::con_handle_t &con,
			const rina::cdap_rib::res_info_t &res);
	void remoteReadResult(const rina::cdap_rib::con_handle_t &con,
			const rina::cdap_rib::obj_info_t &obj,
			const rina::cdap_rib::res_info_t &res);
	void remoteCancelReadResult(const rina::cdap_rib::con_handle_t &con,
			const rina::cdap_rib::res_info_t &res);
	void remoteWriteResult(const rina::cdap_rib::con_handle_t &con,
			const rina::cdap_rib::obj_info_t &obj,
			const rina::cdap_rib::res_info_t &res);
	void remoteStartResult(const rina::cdap_rib::con_handle_t &con,
			const rina::cdap_rib::obj_info_t &obj,
			const rina::cdap_rib::res_info_t &res);
	void remoteStopResult(const rina::cdap_rib::con_handle_t &con,
			const rina::cdap_rib::obj_info_t &obj,
			const rina::cdap_rib::res_info_t &res);
};

class RIBConHandler : public rina::cacep::AppConHandlerInterface {

public:
	void connect(int message_id, const rina::cdap_rib::con_handle_t &con);
	void connectResult(const rina::cdap_rib::res_info_t &res,
				const rina::cdap_rib::con_handle_t &con);
	void release(int message_id, const rina::cdap_rib::con_handle_t &con);
	void releaseResult(const rina::cdap_rib::res_info_t &res,
				const rina::cdap_rib::con_handle_t &con);
};

}; //namespace mad
}; //namespace rinad

#endif  /* __RINAD_RIBM_H__ */

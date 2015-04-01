//TODO

#ifndef __RINAD_RIBM_H__
#define __RINAD_RIBM_H__
/*
#include <pthread.h>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>
#include <utility>
#include <inttypes.h>

#include <librina/common.h>
#include <librina/exceptions.h>
#include <librina/ipc-manager.h>
#include <librina/rib.h>

#include "event-loop.h"
*/

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

/**
* @brief RIB manager
*/
class RIBFactory :public rina::Lockable {

public:
	//Constructors
	RIBFactory(std::list<uint64_t> supported_versions);
	virtual ~RIBFactory(void) throw();

	/**
	* Get a reference to a RIB
	* @throws eRIBNotFound
	*/
	rina::rib::RIBDNorthInterface& getRIB(uint64_t version);

#if 0
	//TODO: if ever necessary

	/**
	* @brief Destroy a RIB instance
	*
	* This call will stop all communications that are using this RIB
	* instance
	*/
	void destroyRIB(uint64_t version);
#endif

protected:
	/**
	* Create a RIB instance
	* @throwseDuplicatedRIB
	*/
	void createRIB(uint64_t version);

private:

	//Map with the current RIB instances
	std::map<uint64_t, rina::rib::RIBDNorthInterface*> rib_inst_;
	rina::rib::RIBDFactory factory_;

	/*
	* Internal methods
	*/
	//TODO
};

}; //namespace mad
}; //namespace rinad

#endif  /* __RINAD_RIBM_H__ */

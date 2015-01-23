//TODO

#ifndef __RINAD_RIBM_H__
#define __RINAD_RIBM_H__

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
class RIBFactory_ {

public:
	/**
	* Initialize running state
	*/
	void init(void);

	/**
	* Destroy the running state
	*/
	void destroy(void);

	/**
	* Create a RIB instance
	* @throwseDuplicatedRIB
	*/
	rina::IRIBDaemon& createRIB(uint64_t version);

	/**
	* Get a reference to a RIB
	* @throws eRIBNotFound
	*/
	rina::IRIBDaemon& getRIB(uint64_t version);

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

private:
	//RWLock
	pthread_rwlock_t rwlock;

	//Map with the current RIB instances
	std::map<uint64_t, rina::IRIBDaemon*> rib_inst;

	//Constructors
	RIBFactory_(void);
	~RIBFactory_(void);

	/*
	* Internal methods
	*/
	//TODO

	friend class Singleton<RIBFactory_>;
};

//Singleton instance
extern Singleton<RIBFactory_> RIBFactory;

}; //namespace mad
}; //namespace rinad

#endif  /* __RINAD_RIBM_H__ */

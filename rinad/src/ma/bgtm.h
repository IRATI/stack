//TODO

#ifndef __RINAD_BG_H__
#define __RINAD_BG_H__

#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>
#include <utility>

#include <librina/common.h>
#include <librina/ipc-manager.h>

#include "event-loop.h"

namespace rinad{
namespace mad{

/**
* @file bgtm.h
* @author Marc Sune<marc.sune (at) bisdn.de>
*
* @brief Background task manager
*/


/**
* @brief A background task manager. This runs on the main loop
*/
class BGTaskManager_ {

public:
	/**
	* Initialize running state
	*/
	void init(void);

	/**
	* Destroy the running state
	*/
	void destroy(void);

	//Methods
	void* run(void* unused);

	//Run flag
	volatile bool keep_running;
private:
	//Constructors
	BGTaskManager_(void);
	~BGTaskManager_(void);

	friend class Singleton<BGTaskManager_>;
};

//Singleton instance
extern Singleton<BGTaskManager_> BGTaskManager;


}; //namespace mad
}; //namespace rinad

#endif  /* __RINAD_BG_H__ */

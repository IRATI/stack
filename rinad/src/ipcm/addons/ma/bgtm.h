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
class BGTaskManager {

public:
	//Constructors
	BGTaskManager(void);
	~BGTaskManager(void);

	//Methods
	void* run(void* unused);

	//Run flag
	volatile bool keep_running;
private:

};

}; //namespace mad
}; //namespace rinad

#endif  /* __RINAD_BG_H__ */

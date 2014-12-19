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
class BGTaskManager /*: public rina::Application*/{

public:
	static void init(void);
	static inline BGTaskManager* get(){
		return inst;
	};
	static void destroy(void);
	
	//Methods	
	void* run(void* unused);

	//Run flag
	bool keep_running;
protected:
	static BGTaskManager* inst;

	//Constructors
	BGTaskManager(void);
	~BGTaskManager(void);

private:

};

}; //namespace mad 
}; //namespace rinad 

#endif  /* __RINAD_BG_H__ */

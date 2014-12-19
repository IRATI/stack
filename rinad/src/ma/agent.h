//TODO

#ifndef __RINAD_MA_H__
#define __RINAD_MA_H__

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
* @file configuration.h
* @author Marc Sune<marc.sune (at) bisdn.de>
*
* @brief Management Agent daemon class
*/


/**
* @brief Management Agent singleton class
*/
class ManagementAgent /*: public rina::Application*/{

public:
	static void init(const std::string& logfile, 
						const std::string& loglevel);
	static void destroy(void);
	
	//Methods	
	

protected:
	static ManagementAgent* inst;

	ManagementAgent(const std::string& logfile, 
						const std::string& loglevel);
	~ManagementAgent(void);
	
private:

};

}; //namespace mad 
}; //namespace rinad 

#endif  /* __RINAD_MA_H__ */

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
#include "rib-daemon.h"

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
	static void init(const std::string& conf,
					const std::string& cl_logfile,
					const std::string& cl_loglevel);
	static void destroy(void);
	
	//Methods	
	

protected:
	static ManagementAgent* inst;

	ManagementAgent(const std::string& conf,
					const std::string& cl_logfile,
					const std::string& cl_loglevel);
	~ManagementAgent(void);

private:
	void populateBasicRIB();
	MARIBDaemon *rib_daemon_;
};

}; //namespace mad 
}; //namespace rinad 

#endif  /* __RINAD_MA_H__ */

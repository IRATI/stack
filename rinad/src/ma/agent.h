//TODO

#ifndef __RINAD_MA_H__
#define __RINAD_MA_H__

#include <assert.h>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>
#include <utility>

#include <librina/application.h>
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
class ManagementAgent{

public:

	/**
	* Initialize the management agent components
	*/
	static void init(const std::string& conf,
					const std::string& cl_logfile,
					const std::string& cl_loglevel);

	/**
	* Destroy the entire Management Agent state
	*/
	static void destroy(void);

	/**
	* Set RINA AP information
	*/
	static void setAPInfo(rina::ApplicationProcessNamingInformation& _info){
		assert(inst != NULL);
		inst->info = _info;
	}

protected:
	static ManagementAgent* inst;
	rina::ApplicationProcessNamingInformation info;

	ManagementAgent(const std::string& conf,
					const std::string& cl_logfile,
					const std::string& cl_loglevel);
	~ManagementAgent(void);
};

}; //namespace mad
}; //namespace rinad

#endif  /* __RINAD_MA_H__ */

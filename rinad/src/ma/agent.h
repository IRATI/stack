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
#include <librina/patterns.h>

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
class ManagementAgent_{

public:

	/**
	* Initialize the management agent components
	*/
	void init(const std::string& conf,
					const std::string& cl_logfile,
					const std::string& cl_loglevel);

	/**
	* Destroy management agent and components state
	*/
	void destroy(void);

	/**
	* Set RINA AP information
	*/
	void setAPInfo(rina::ApplicationProcessNamingInformation& _info){
		//assert(inst != NULL);
		//inst->info = _info;
		(void)_info;
	}

private:
	rina::ApplicationProcessNamingInformation info;

	ManagementAgent_();
	~ManagementAgent_(void);

	friend class Singleton<ManagementAgent_>;
};

//Singleton instance
extern Singleton<ManagementAgent_> ManagementAgent;

}; //namespace mad
}; //namespace rinad

#endif  /* __RINAD_MA_H__ */

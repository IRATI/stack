//TODO

#ifndef __MA_CONFIGURATION_H__
#define __MA_CONFIGURATION_H__

#include <assert.h>
#include <string>
#include <librina/application.h>
#include <librina/common.h>

/**
* @file configuration.h
* @author Marc Sune<marc.sune (at) bisdn.de>
*
* @brief Management agent Configuration Manager module
*
* TODO: this module might be dropped in the future.
*/

namespace rinad{
namespace mad{

//fwd decl
class ManagementAgent;

/**
* @brief MAD configuration manager component
*/
class ConfManager{

public:
	/**
	* Initialize running state
	*/
	ConfManager(const std::string& params);

	/**
	* Destroy the running state
	*/
	~ConfManager(void);

	/**
	* Reads the configuration source (e.g. a config file) and configures
	* the rest of the modules
	*/
	void configure(ManagementAgent& agent);

private:

};


}; //namespace mad
}; //namespace rinad

#endif  /* __MA_CONFIGURATION_H__ */

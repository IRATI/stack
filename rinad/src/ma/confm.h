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

/**
* @brief MAD configuration manager component
*/
class ConfManager_{

public:
	/**
	* Initialize running state
	*/
	void init(const std::string& conf,
					const std::string& cl_logfile,
					const std::string& cl_loglevel);
	/**
	* Destroy the running state
	*/
	void destroy(void);

	/**
	* Reads the configuration source (e.g. a config file) and configures
	* the rest of the modules
	*/
	void configure(void);

private:
	ConfManager_(void);
	~ConfManager_(void);

	friend class Singleton<ConfManager_>;

};

//Singleton instance
extern Singleton<ConfManager_> ConfManager;


}; //namespace mad
}; //namespace rinad

#endif  /* __MA_CONFIGURATION_H__ */

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
* @brief Management agent ConfManager module
*
* TODO: this module might be dropped in the future.
*/

namespace rinad{
namespace mad{

/**
* @brief MAD configuration
*/
class ConfManager{

public:
	static void init(const std::string& conf,
					const std::string& cl_logfile,
					const std::string& cl_loglevel);

	/**
	* Reads the configuration source (e.g. a config file) and configures
	* the rest of the modules
	*/
	static void configure(void){
		assert(inst != NULL);
		inst->_configure();
	}

	static void destroy(void);

protected:
	static ConfManager* inst;

	ConfManager(const std::string& conf,
					const std::string& cl_logfile,
					const std::string& cl_loglevel);
	~ConfManager(void);

	/*
	* Configure routine
	*/
	void _configure(void);

private:

};


}; //namespace mad
}; //namespace rinad

#endif  /* __MA_CONFIGURATION_H__ */

//TODO

#ifndef __MA_CONFIGURATION_H__
#define __MA_CONFIGURATION_H__

#include <string>

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
	static inline ConfManager* get(){
		return inst;
	};
	
	void configure(void);
	
	static void destroy(void);

protected:
	static ConfManager* inst;

	ConfManager(const std::string& conf,
					const std::string& cl_logfile,
					const std::string& cl_loglevel);
	~ConfManager(void);


private:

};


}; //namespace mad
}; //namespace rinad

#endif  /* __MA_CONFIGURATION_H__ */

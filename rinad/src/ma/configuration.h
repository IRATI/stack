//TODO

#ifndef __MA_CONFIGURATION_H__
#define __MA_CONFIGURATION_H__

#include <string>

/**
* @file configuration.h
* @author Marc Sune<marc.sune (at) bisdn.de>
*
* @brief Management agent Configuration module
*
* TODO: this module might be dropped in the future. 
*/

namespace rinad{
namespace mad{

//fwd decl
class ManagementAgent;

/**
* @brief MAD configuration
*/
class Configuration{

public:
	void load(ManagementAgent ma, std::string& file_path);
private:
	void validate(std::string& file_path);
};


}; //namespace mad 
}; //namespace rinad 

#endif  /* __MA_CONFIGURATION_H__ */

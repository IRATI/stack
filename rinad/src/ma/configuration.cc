#include "configuration.h"
#include "agent.h"

namespace rinad{
namespace mad{


void Configuration::load(ManagementAgent ma, std::string& file_path){
	(void)ma;
	(void)file_path;
}

void Configuration::validate(std::string& file_path){
	(void)file_path;
}

}; //namespace mad 
}; //namespace rinad 



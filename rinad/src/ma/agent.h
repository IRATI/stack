//TODO

#ifndef __RINAD_MA_H__
#define __RINAD_MA_H__

#include <assert.h>
#include <cstdlib>
#include <iostream>
#include <map>
#include <list>
#include <vector>
#include <utility>

#include <librina/application.h>
#include <librina/common.h>
#include <librina/exceptions.h>
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
* There are no available NMS DIFs (invalid configuration)
*/
DECLARE_EXCEPTION_SUBCLASS(eNoNMDIF);


/**
* @brief Encapsulates the Connection state
*/
class Connection{

public:
	rina::ApplicationProcessNamingInformation rinfo;
	/* TODO: add CDAP auth details*/
};

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
		info = _info;
	}

	/**
	* Set RINA AP information
	*/
	void addNMSDIF(std::string& difName);


	/**
	* Set the Managers connection details
	*
	* TODO: improve this so that this method returns a handler, such that
	* in the future connections to the Manager(s) can be removed and
	* modified at runtime.
	*/
	void addManagerConnection(std::string& APName,
				std::string APInstId = std::string(""),
				std::string AEname = std::string(""),
				std::string AEinst = std::string("")
				/* TODO: add CDAP auth details*/);

private:

	/**
	* RINA AP information
	*/
	rina::ApplicationProcessNamingInformation info;

	/**
	* List of NMSDIFs
	*/
	std::list<std::string> nmsDIFs;

	/**
	* List of Manager connections
	*/
	std::list<Connection> connections;

	ManagementAgent_();
	~ManagementAgent_(void);

	//
	// Internal methods
	//

	/**
	* Bootstrap necessary NMS DIFs and shim-DIFs
	*/
	void bootstrapNMSDIFs(void);

	/**
	* Register agent AP into the IPCManager
	*/
	void reg(void);

	/**
	* Connect to the Manager/s
	*/
	void connect(void);

	friend class Singleton<ManagementAgent_>;
};

//Singleton instance
extern Singleton<ManagementAgent_> ManagementAgent;

}; //namespace mad
}; //namespace rinad

#endif  /* __RINAD_MA_H__ */

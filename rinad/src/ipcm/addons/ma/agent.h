/*
 * ManagementAgent class
 *
 *    Marc Sune <marc.sune (at) bisdn.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

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

#include "../../addon.h"
#include "flowm.h"

//Name of the MAD addon
#define MAD_NAME "mad"

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
*
* FIXME: move to librina?
*/
class AppConnection{

public:
	rina::FlowInformation flow_info;
	/* TODO: add CACEP auth details*/
};

//Submodule classes fwd decl
class ConfManager;
class FlowManager;
class RIBFactory;
class BGTaskManager;

/**
* @brief Management Agent singleton class
*/
class ManagementAgent : public AppAddon{
public:

	//Constructor and destructor
	ManagementAgent(const rinad::RINAConfiguration& config);
	~ManagementAgent(void);

	/**
	* Retrieve a **copy** of the AP naming information
	*/
	rina::ApplicationProcessNamingInformation getAPInfo(void){
		return info;
	}

	/**
	* Set RINA AP information
	*/
	void setAPInfo(rina::ApplicationProcessNamingInformation& _info){
		info = _info;
	}

	/**
	* Add an NMS DIF
	*/
	void addNMSDIF(std::string& difName);


	/**
	* Add a Manager connection
	*
	* TODO: improve this so that this method returns a handler, such that
	* in the future connections to the Manager(s) can be removed and
	* modified at runtime.
	*/
	void addManagerConnection(AppConnection& con);


	/**
	* Instantance pointer; there can only be an instance
	*/
	static ManagementAgent* inst;

	//Addon name
	static const std::string NAME;

protected:
	//Process flow event
	void process_librina_event(rina::IPCEvent** event);

	//Process ipcm event
	void process_ipcm_event(const IPCMEvent& event);

	//Get RIB factory pointer
	inline RIBFactory* get_ribf(void){
		return rib_factory;
	};

	friend class FlowManager;
private:

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

	//Submodules
	ConfManager* conf_manager;
	FlowManager* flow_manager;
	RIBFactory* rib_factory;
	BGTaskManager* bg_task_manager;

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
	std::list<AppConnection> connections;

};

}; //namespace mad
}; //namespace rinad

#endif  /* __RINAD_MA_H__ */

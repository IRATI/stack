/*
 * ManagementAgent class
 *
 *    Marc Sune <marc.sune (at) bisdn.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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
	ManagementAgent(const std::string& params);
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

protected:
	//Process event
	virtual void process_event(rina::IPCEvent** event);

private:

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

};

}; //namespace mad
}; //namespace rinad

#endif  /* __RINAD_MA_H__ */

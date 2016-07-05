/*
 * Base class for an addon
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

#ifndef __ADDON_H__
#define __ADDON_H__

#include <assert.h>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>
#include <utility>

#include <librina/common.h>
#include <librina/concurrency.h>
#include <librina/ipc-manager.h>
#include <librina/patterns.h>
#include "rina-configuration.h"

#include "ipcp.h"

namespace rinad {

//fwd decl
class Addon;

/**
* Events to be distributed to the addons
*/
enum event_type {

	//Addon related events
	IPCM_ADDON_LOADED,

	//General events
	IPCM_IPCP_CREATED,
	IPCM_IPCP_CRASHED, //oops
	IPCM_IPCP_TO_BE_DESTROYED,
	IPCM_IPCP_UPDATED,

};


class IPCMEvent{

public:
	/**
	* IPCP general constructor
	*/
	IPCMEvent(Addon* c, enum event_type t, int ipcp_id_) : type(t),
							callee(c),
							ipcp_id(ipcp_id_),
							addon(""){};

	/**
	* Addon loaded constructor
	*/
	IPCMEvent(Addon* c, std::string addon_name) : type(IPCM_ADDON_LOADED),
							callee(c),
							ipcp_id(-1),
							addon(addon_name){};

	/**
	* Type of the event
	*/
	enum event_type type;

	/**
	* Callee that originated the event or null
	*/
	Addon* callee;

	/**
	* IPCP id that the event is related to, or -1 otherwise.
	*/
	int ipcp_id;

	/**
	* Loaded addon or "" otherwise
	*/
	std::string addon;
};

/**
* Addon base class
*/
class Addon{

public:

	Addon(const std::string _name);
	virtual ~Addon(){};

	/**
	* Addon name
	*/
	const std::string name;

	/**
	* Distribute a librina event to the addons
	*
	* This is a necessary method because both the internals of the
	* IPCManager and addons consume librina APIs, and only one "listener"
	* can coexist (without loosing events).
	*
	* TODO: deprecate when librina-application is improved
	*/
	static void distribute_flow_event(rina::IPCEvent* event);

	/**
	* Distributes the events to all the addons except callee (if any)
	*
	* Note that IPCMEvent ipcp_id is only valid for IPCP events, and
	* "addon" is only valid for "IPCM_ADDON_LOADED"
	*
	* @warning: the callback shall NEVER block or it will interfere with
	* the IPCM operation.
	*/
	static void distribute_ipcm_event(const IPCMEvent& event);

	/**
	* Factory
	*/
	static void factory(rinad::RINAConfiguration& conf,
			    const std::string& name);

	/**
	* Destroy all
	*
	* This is not thread-safe and shall never be called if there can come
	* events to process
	*/
	static void destroy_all(void);

protected:

	/**
	* Process flow event callback
	*
	* @param event The event to process
	*/
	virtual void process_librina_event(rina::IPCEvent** event){(void)event;};

	/**
	* Process IPCM event callback
	*
	* @param event The event to process
	*/
	virtual void process_ipcm_event(const IPCMEvent& event){(void)event;};

	//Register to receive events
	static void subscribe(Addon* addon);

	//Unregister from receiving events
	static void unsubscribe(Addon* addon);

private:
	/**
	* Mutex
	*/
	static rina::ReadWriteLockable rwlock;

	/**
	* List of all the current running addons
	*/
	static std::map<std::string, Addon*> addons;

	/**
	* Event subscribers
	*/
	static std::list<Addon*> event_subscribers;
};

/**
* Application addon (consumes librina-application)
*
* TODO remove this once librina-application has been refactored
*/
class AppAddon : public Addon {

public:
	AppAddon(const std::string _name):Addon(_name){
		Addon::subscribe(this);
	};
	virtual ~AppAddon(){
		Addon::unsubscribe(this);
	};

protected:
	// Event processing routine
	virtual void process_librina_event(rina::IPCEvent** event)=0;

	//IPCM event processing routine
	virtual void process_ipcm_event(const IPCMEvent& event){(void)event;};
};

}//rinad namespace

#endif  /* __ADDON_H__ */

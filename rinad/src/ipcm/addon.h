/*
 * Base class for an addon
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

/**
* Addon base class
*/
class Addon{

public:

	Addon(const std::string _name):name(_name){};
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
	static void distribute_event(rina::IPCEvent* event);

	/**
	* Factory
	*/
	static Addon* factory(rinad::RINAConfiguration& conf,
			      const std::string& conf_name,
			      const std::string& name);

protected:

	/**
	* Process event callback
	*
	* @param event The event to process
	*
	* @ret NULL if consumed otherwise event
	*/
	virtual void process_event(rina::IPCEvent** event){(void)event;};

	//Register to receive events
	static void subscribe(Addon* addon);

	//Unregister from receiving events
	static void unsubscribe(Addon* addon);

private:
	/**
	* Mutex
	*/
	static rina::Lockable mutex;

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
	virtual void process_event(rina::IPCEvent** event)=0;

};

}//rinad namespace

#endif  /* __ADDON_H__ */

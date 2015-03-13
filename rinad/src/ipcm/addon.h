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
#include <librina/ipc-manager.h>
#include <librina/patterns.h>

namespace rinad {

/**
* Addon base class
*/
class Addon{

public:

	Addon(const std::string _name):name(_name){};
	virtual ~Addon(){};

	//
	// Callback section
	//

#if 0
	/**
	* On an async call to IPCManager::create_ipcp(), this callback will be
	* executed
	*
	* @param result 0 on success, -1 on failure
	*/
	virtual void callback_create_ipcp(const int result) const{
		/* Do nothing */
		(void)result;
	};

	/**
	* On an async call to IPCManager::destroy_ipcp(), this callback will be
	* executed
	*
	* @ret 0 on success, -1 on failure
	*/
	virtual void callback_destroy_ipcp(const int result) const{
		/* Do nothing */
		(void)result;
	};

#endif
	/**
	* Addon name
	*/
	std::string name;

};

}//rinad namespace

#endif  /* __ADDON_H__ */

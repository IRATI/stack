/*
 * Background Task Manager
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

#ifndef __RINAD_BG_H__
#define __RINAD_BG_H__

#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>
#include <utility>

#include <librina/common.h>
#include <librina/ipc-manager.h>

namespace rinad{
namespace mad{

/**
* @file bgtm.h
* @author Marc Sune<marc.sune (at) bisdn.de>
*
* @brief Background task manager
*/


/**
* @brief A background task manager. This runs on the main loop
*/
class BGTaskManager {

public:
	//Constructors
	BGTaskManager(void);
	~BGTaskManager(void);

	//Methods
	void* run(void* unused);

	//Run flag
	volatile bool keep_running;
private:

};

}; //namespace mad
}; //namespace rinad

#endif  /* __RINAD_BG_H__ */

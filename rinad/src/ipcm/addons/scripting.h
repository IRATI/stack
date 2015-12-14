/*
 * Scripting engine
 *
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
 *    Marc Sune             <marc.sune (at) bisdn.de>
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

#ifndef __IPCM_SCRIPTING_H__
#define __IPCM_SCRIPTING_H__

#include <cstdlib>
#include <map>
#include <vector>

#include "../addon.h"

namespace rinad {


class ScriptingEngine : public Addon{

public:
	ScriptingEngine(void);
	virtual ~ScriptingEngine(void);
	static const std::string NAME;
};

} //namespace rinad

#endif  /* __IPCM_SCRIPTING_H__ */

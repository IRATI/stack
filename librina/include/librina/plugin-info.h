/*
 * Plugin information retrieval from manifest manifest
 *
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
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
#ifndef __PLUGIN_INFO__
#define __PLUGIN_INFO__

#include <list>
#include <string>

#include <librina/common.h>
#include <librina/application.h>

namespace rina {

int plugin_get_info(const std::string& plugin_name,
		    const std::string& plugins_path,
		    std::list<rina::PsInfo>& result);

} // namespace rina

#endif  /* __PLUGIN_INFO__ */

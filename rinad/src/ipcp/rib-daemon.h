/*
 * RIB Daemon
 *
 *    Bernat Gaston <bernat.gaston@i2cat.net>
 *    Eduard Grasa <eduard.grasa@i2cat.net>
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

#ifndef IPCP_RIB_DAEMON_HH
#define IPCP_RIB_DAEMON_HH

#ifdef __cplusplus

#include <librina/concurrency.h>

#include "ipcp/components.h"

namespace rinad {

class RIB : public rina::Lockable {
public:
	RIB();
	~RIB() throw();

	/// Given an objectname of the form "substring\0substring\0...substring" locate
	/// the RIBObject that corresponds to it
	/// @param objectName
	/// @return
	BaseRIBObject* getRIBObject(const std::string& objectName) throw (Exception);
	void addRIBObject(BaseRIBObject* ribObject) throw (Exception);
	BaseRIBObject * removeRIBObject(const std::string& objectName) throw (Exception);
	std::list<BaseRIBObject*> getRIBObjects();

private:
	std::map<std::string, BaseRIBObject*> rib_;
};

}

#endif

#endif

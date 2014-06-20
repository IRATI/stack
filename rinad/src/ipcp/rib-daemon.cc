//
// RIB Daemon
//
//    Bernat Gaston <bernat.gaston@i2cat.net>
//    Eduard Grasa <eduard.grasa@i2cat.net>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#include "rib-daemon.h"

namespace rinad {

//Class RIB
RIB::RIB() {
}

RIB::~RIB() throw() {
}

BaseRIBObject* RIB::getRIBObject(const std::string& objectName) throw (Exception) {
	BaseRIBObject* ribObject;
	std::map<std::string, BaseRIBObject*>::iterator it;

	lock();
	it = rib_.find(objectName);
	unlock();

	if (it == rib_.end()) {
		throw Exception("Could not find object in the RIB");
	}

	ribObject = it->second;
	return ribObject;
}

void RIB::addRIBObject(BaseRIBObject* ribObject) throw (Exception) {
	lock();
	if (rib_.find(ribObject->get_name()) != rib_.end()) {
		throw Exception("Object already exists in the RIB");
	}
	rib_[ribObject->get_name()] = ribObject;
	unlock();
}

BaseRIBObject * RIB::removeRIBObject(const std::string& objectName) throw (Exception) {
	std::map<std::string, BaseRIBObject*>::iterator it;
	BaseRIBObject* ribObject;

	lock();
	it = rib_.find(objectName);
	if (it == rib_.end()) {
		throw Exception("Could not find object in the RIB");
	}

	ribObject = it->second;
	rib_.erase(it);
	unlock();

	return ribObject;
}

std::list<BaseRIBObject*> RIB::getRIBObjects() {
	std::list<BaseRIBObject*> result;

	for(std::map<std::string, BaseRIBObject*>::iterator it = rib_.begin();
			it != rib_.end(); ++it) {
		result.push_back(it->second);
	}

	return result;
}

}

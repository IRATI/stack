//
// Resource Allocator
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

#include <sstream>

#include "resource-allocator.h"

namespace rinad {

//Class NMinusOneFlowManager
NMinusOneFlowManager::NMinusOneFlowManager() {
	rib_daemon_ = 0;
	ipc_process_ = 0;
	cdap_session_manager_ = 0;
}

void NMinusOneFlowManager::set_ipc_process(IPCProcess * ipc_process) {
	ipc_process_ = ipc_process;
	rib_daemon_ = ipc_process->get_rib_daemon();
	cdap_session_manager_ = ipc_process->get_cdap_session_manager();
}

void NMinusOneFlowManager::populateRIB(){
	try {
		BaseRIBObject * object = new DIFRegistrationSetRIBObject(ipc_process_, this);
		rib_daemon_->addRIBObject(object);
	} catch (Exception &e) {
		//TODO log error
	}
}

//Class DIFRegistrationSetRIBObject
const std::string DIFRegistrationSetRIBObject::DIF_REGISTRATION_SET_RIB_OBJECT_CLASS
	= "DIF registration set";
const std::string DIFRegistrationSetRIBObject::DIF_REGISTRATION_RIB_OBJECT_CLASS
	= "DIF registration";
const std::string DIFRegistrationSetRIBObject::DIF_REGISTRATION_SET_RIB_OBJECT_NAME
	= RIBObjectNames::SEPARATOR + RIBObjectNames::DIF + RIBObjectNames::SEPARATOR +
	RIBObjectNames::RESOURCE_ALLOCATION + RIBObjectNames::SEPARATOR +
	RIBObjectNames::NMINUSONEFLOWMANAGER + RIBObjectNames::SEPARATOR + RIBObjectNames::DIF_REGISTRATIONS;

DIFRegistrationSetRIBObject::DIFRegistrationSetRIBObject(IPCProcess * ipcProcess,
			INMinusOneFlowManager * nMinus1FlowManager) :
					BaseRIBObject(ipcProcess, DIF_REGISTRATION_SET_RIB_OBJECT_CLASS,
							objectInstanceGenerator->getObjectInstance(),
							DIF_REGISTRATION_SET_RIB_OBJECT_NAME) {
	n_minus_one_flow_manager_ = nMinus1FlowManager;
}

void DIFRegistrationSetRIBObject::createObject(const std::string& objectClass,
		const std::string& objectName, void* objectValue) throw (Exception) {
	SimpleSetMemberRIBObject * ribObject = new SimpleSetMemberRIBObject(get_ipc_process(), objectClass,
			objectName, objectValue);
	add_child(ribObject);
	get_rib_daemon()->addRIBObject(ribObject);
}

void* DIFRegistrationSetRIBObject::get_value() {
	int i=0;
	std::list<BaseRIBObject*> children = get_children();
	void ** result = new void*[children.size()];

	for (std::list<BaseRIBObject*>::iterator it = children.begin();
				it != children.end(); it++) {
		result[i] = (*it)->get_value();
		i++;
	}

	return result;
}

}

//
// Enrollment
//
//    Francesco Salvestrini <f.salvestrini@nextworks.it>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA  02110-1301  USA
//

#define RINA_PREFIX "librina.enrollment"

#include <librina/logs.h>
#include "librina/enrollment.h"
#include "librina/irm.h"
#include "librina/ipc-process.h"

namespace rina {

// Class Neighbor RIB object
NeighborRIBObject::NeighborRIBObject(IRIBDaemon * rib_daemon,
		const std::string& object_class, const std::string& object_name,
		const rina::Neighbor* neighbor) :
				SimpleSetMemberRIBObject(rib_daemon, object_class,
						object_name, neighbor)
{
};

std::string NeighborRIBObject::get_displayable_value() {
    const rina::Neighbor * nei = (const rina::Neighbor *) get_value();
    std::stringstream ss;
    ss << "Name: " << nei->name_.getEncodedString();
    ss << "; Address: " << nei->address_;
    ss << "; Enrolled: " << nei->enrolled_ << std::endl;
    ss << "; Supporting DIF Name: " << nei->supporting_dif_name_.processName;
    ss << "; Underlying port-id: " << nei->underlying_port_id_;
    ss << "; Number of enroll. attempts: " << nei->number_of_enrollment_attempts_;

    return ss.str();
}

// Class Neighbor Set RIB Object
const std::string NeighborSetRIBObject::NEIGHBOR_SET_RIB_OBJECT_CLASS = "neighbor set";
const std::string NeighborSetRIBObject::NEIGHBOR_RIB_OBJECT_CLASS = "neighbor";
const std::string NeighborSetRIBObject::NEIGHBOR_SET_RIB_OBJECT_NAME =
		RIBNamingConstants::SEPARATOR + RIBNamingConstants::DAF + RIBNamingConstants::SEPARATOR +
		RIBNamingConstants::MANAGEMENT + RIBNamingConstants::SEPARATOR + RIBNamingConstants::NEIGHBORS;

NeighborSetRIBObject::NeighborSetRIBObject(ApplicationProcess * app, IRIBDaemon * rib_daemon) :
	BaseRIBObject(rib_daemon, NEIGHBOR_SET_RIB_OBJECT_CLASS,
			objectInstanceGenerator->getObjectInstance(),
			NEIGHBOR_SET_RIB_OBJECT_NAME){
	app_ = app;
}

const void* NeighborSetRIBObject::get_value() const {
	return 0;
}

void NeighborSetRIBObject::remoteCreateObject(void * object_value, const std::string& object_name,
		int invoke_id, rina::CDAPSessionDescriptor * session_descriptor) {
	rina::ScopedLock g(lock_);
	std::list<rina::Neighbor *> neighborsToCreate;


	try {
		if (object_name.compare(NEIGHBOR_SET_RIB_OBJECT_NAME) == 0) {
			std::list<rina::Neighbor *> * neighbors =
					(std::list<rina::Neighbor *> *) object_value;
			std::list<rina::Neighbor *>::const_iterator iterator;
			for(iterator = neighbors->begin(); iterator != neighbors->end(); ++iterator) {
				populateNeighborsToCreateList(*iterator, &neighborsToCreate);
			}

			delete neighbors;
		} else {
			rina::Neighbor * neighbor = (rina::Neighbor *) object_value;
			populateNeighborsToCreateList(neighbor, &neighborsToCreate);
		}
	} catch (rina::Exception &e) {
		LOG_ERR("Error decoding CDAP object value: %s", e.what());
	}

	if (neighborsToCreate.size() == 0) {
		LOG_DBG("No neighbors entries to create or update");
		return;
	}

	try {
		base_rib_daemon_->createObject(NEIGHBOR_SET_RIB_OBJECT_CLASS,
					       NEIGHBOR_SET_RIB_OBJECT_NAME,
					       &neighborsToCreate,
					       0);
	} catch (rina::Exception &e) {
		LOG_ERR("Problems creating RIB object: %s", e.what());
	}
}

void NeighborSetRIBObject::populateNeighborsToCreateList(rina::Neighbor* neighbor,
		std::list<rina::Neighbor *> * list) {
	const rina::Neighbor * candidate;
	std::list<BaseRIBObject*>::const_iterator it;
	bool found = false;

	for(it = get_children().begin(); it != get_children().end(); ++it) {
		candidate = (const rina::Neighbor *) (*it)->get_value();
		if (candidate->get_name().processName.compare(neighbor->name_.processName) == 0)
			found = true;
	}
	if (!found)
		list->push_back(neighbor);
	else
		delete neighbor;
}

void NeighborSetRIBObject::createObject(const std::string& objectClass,
			const std::string& objectName,
			const void* objectValue) {

	if (objectName.compare(NEIGHBOR_SET_RIB_OBJECT_NAME) == 0) {
		std::list<rina::Neighbor *>::const_iterator iterator;
		std::list<rina::Neighbor *> * neighbors =
				(std::list<rina::Neighbor *> *) objectValue;

		for (iterator = neighbors->begin(); iterator != neighbors->end(); ++iterator) {
			createNeighbor((*iterator));
		}
	} else {
		rina::Neighbor * currentNeighbor = (rina::Neighbor *) objectValue;
		createNeighbor(currentNeighbor);
	}
}

void NeighborSetRIBObject::createNeighbor(rina::Neighbor * neighbor) {
	//Avoid creating myself as a neighbor
	if (neighbor->name_.processName.compare(app_->get_name()) == 0) {
		return;
	}

	//Only create neighbours with whom I have an N-1 DIF in common
	std::list<rina::ApplicationProcessNamingInformation>::const_iterator it;
	IPCResourceManager * irm =
			dynamic_cast<IPCResourceManager*>(app_->get_ipc_resource_manager());
	bool supportingDifInCommon = false;
	for(it = neighbor->supporting_difs_.begin(); it != neighbor->supporting_difs_.end(); ++it) {
		if (irm->isSupportingDIF((*it))) {
			neighbor->supporting_dif_name_ = (*it);
			supportingDifInCommon = true;
			break;
		}
	}

	if (!supportingDifInCommon) {
		LOG_INFO("Ignoring neighbor %s because we don't have an N-1 DIF in common",
				neighbor->name_.processName.c_str());
		return;
	}

	std::stringstream ss;
	ss<<NEIGHBOR_SET_RIB_OBJECT_NAME<<RIBNamingConstants::SEPARATOR;
	ss<<neighbor->name_.processName;
	BaseRIBObject * ribObject = new NeighborRIBObject(base_rib_daemon_,
							  NEIGHBOR_RIB_OBJECT_CLASS,
							  ss.str(),
							  neighbor);
	add_child(ribObject);
	try {
		base_rib_daemon_->addRIBObject(ribObject);
	} catch(rina::Exception &e){
		LOG_ERR("Problems adding object to the RIB: %s", e.what());
	}
}

}


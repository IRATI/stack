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
const std::string NeighborRIBObj::class_name = "Neighbor";
const std::string NeighborRIBObj::object_name_prefix = "/difmanagement/enrollment/neighbors/processName=";
const std::string NeighborRIBObj::parent_class_name = "Neighbors";
const std::string NeighborRIBObj::parent_object_name = "/difmanagement/enrollment/neighbors";

NeighborRIBObj::NeighborRIBObj(ApplicationProcess * app,
			      rib::RIBDaemonProxy * rib_daemon,
			      rib::rib_handle_t rib_handle,
			      const Neighbor* neigh) : rib::RIBObj(class_name)
{
	neighbor = neigh;
	app_ = app;
	ribd = rib_daemon;
	rib = rib_handle;
}

const std::string NeighborRIBObj::get_displayable_value() const
{
    std::stringstream ss;
    ss << "Name: " << neighbor->name_.getEncodedString();
    ss << "; Address: " << neighbor->address_;
    ss << "; Enrolled: " << neighbor->enrolled_ << std::endl;
    ss << "; Supporting DIF Name: " << neighbor->supporting_dif_name_.processName;
    ss << "; Underlying port-id: " << neighbor->underlying_port_id_;
    ss << "; Number of enroll. attempts: " << neighbor->number_of_enrollment_attempts_;

    return ss.str();
}

void NeighborRIBObj::create_cb(const rib::rib_handle_t rib,
			       const cdap_rib::con_handle_t &con,
			       const std::string& fqn,
			       const std::string& class_,
			       const cdap_rib::filt_info_t &filt,
			       const int invoke_id,
			       const ser_obj_t &obj_req,
			       ser_obj_t &obj_reply,
			       cdap_rib::res_info_t& res)
{
	rina::ScopedLock g(lock_);
	rina::Neighbor * neighbor;
	res.code_ = cdap_rib::CDAP_SUCCESS;

	//TODO 1 decode neighbor from ser_obj_t

	//2 If neighbor is already in RIB, exit
	if (ribd->containsObj(rib, fqn))
		return;

	//3 Avoid creating myself as a neighbor
	if (neighbor->name_.processName.compare(app_->get_name()) == 0)
		return;

	//4 Only create neighbours with whom I have an N-1 DIF in common
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

	//5 Create object
	try {
		std::stringstream ss;
		ss << NeighborRIBObj::object_name_prefix << neighbor->name_.processName;

		NeighborRIBObj * nrobj = new NeighborRIBObj(app_, ribd, rib, neighbor);
		ribd->addObjRIB(rib, ss.str(), &nrobj);
	} catch (Exception &e) {
		res.code_ = cdap_rib::CDAP_ERROR;
		LOG_ERR("Problems creating RIB object: %s", e.what());
	}
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


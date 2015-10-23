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

NeighborRIBObj::NeighborRIBObj(Neighbor* neigh) :
		rib::RIBObj(class_name), neighbor(neigh)
{
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


// Class Neighbor RIB object
const std::string NeighborsRIBObj::class_name = "Neighbors";
const std::string NeighborsRIBObj::object_name = "/difmanagement/enrollment/neighbors";

NeighborsRIBObj::NeighborsRIBObj(ApplicationProcess * app,
			      	 rib::RIBDaemonProxy * rib_daemon,
			      	 rib::rib_handle_t rib_handle) :
					      rib::RIBObj(class_name)
{
	app_ = app;
	ribd = rib_daemon;
	rib = rib_handle;
}

void NeighborsRIBObj::create(const cdap_rib::con_handle_t &con,
			     const std::string& fqn,
			     const std::string& class_,
			     const cdap_rib::filt_info_t &filt,
			     const int invoke_id,
			     const ser_obj_t &obj_req,
			     ser_obj_t &obj_reply,
			     cdap_rib::res_info_t& res)
{
	Lockable lock;
	ScopedLock g(lock);
	std::list<rina::Neighbor *> * neighbors = 0;
	std::stringstream ss;
	res.code_ = cdap_rib::CDAP_SUCCESS;

	//TODO 1 decode neighbor list from ser_obj_t

	std::list<rina::Neighbor *>::const_iterator iterator;
	for(iterator = neighbors->begin(); iterator != neighbors->end(); ++iterator) {
		ss.flush();
		ss << NeighborRIBObj::object_name_prefix;
		ss << (*iterator)->name_.processName;

		//2 If neighbor is already in RIB, exit
		if (ribd->containsObj(rib, ss.str()))
			continue;

		//3 Avoid creating myself as a neighbor
		if ((*iterator)->name_.processName.compare(app_->get_name()) == 0)
			continue;

		//4 Only create neighbours with whom I have an N-1 DIF in common
		std::list<rina::ApplicationProcessNamingInformation>::const_iterator it;
		IPCResourceManager * irm =
				dynamic_cast<IPCResourceManager*>(app_->get_ipc_resource_manager());
		bool supportingDifInCommon = false;
		for(it =(*iterator)->supporting_difs_.begin();
				it != (*iterator)->supporting_difs_.end(); ++it) {
			if (irm->isSupportingDIF((*it))) {
				(*iterator)->supporting_dif_name_ = (*it);
				supportingDifInCommon = true;
				break;
			}
		}

		if (!supportingDifInCommon) {
			LOG_INFO("Ignoring neighbor %s because we don't have an N-1 DIF in common",
					(*iterator)->name_.processName.c_str());
			continue;
		}

		//5 Create object
		try {
			std::stringstream ss2;
			ss2 << NeighborRIBObj::object_name_prefix << (*iterator)->name_.processName;

			NeighborRIBObj * nrobj = new NeighborRIBObj(*iterator);
			ribd->addObjRIB(rib, ss2.str(), &nrobj);
		} catch (Exception &e) {
			res.code_ = cdap_rib::CDAP_ERROR;
			LOG_ERR("Problems creating RIB object: %s", e.what());
		}
	}

	delete neighbors;
}

}


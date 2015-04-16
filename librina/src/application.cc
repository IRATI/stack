//
// Application
//
//    Eduard Grasa          <eduard.grasa@i2cat.net>
//    Vincenzo Maffione     <v.maffione@nextworks.it>
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

#define RINA_PREFIX "application"

#include "librina/logs.h"
#include "core.h"
#include "rina-syscalls.h"
#include "librina/application.h"

namespace rina {

const std::string IPolicySet::DEFAULT_PS_SET_NAME = "default";

// Class ApplicationEntityInstance
const std::string& ApplicationEntityInstance::get_instance_id() const
{
	return instance_id_;
}

// Class ApplicationEntity
ApplicationEntity::~ApplicationEntity()
{
		instances.deleteValues();
}

const std::string& ApplicationEntity::get_name() const
{
		return name_;
}

void ApplicationEntity::add_instance(ApplicationEntityInstance * instance)
{
		if (!instance) {
				LOG_ERR("Bogus AE instance passed, returning");
				return;
		}

		instance->set_application_entity(this);
		instances.put(instance->get_instance_id(), instance);
}

ApplicationEntityInstance * ApplicationEntity::remove_instance(const std::string& instance_id)
{
		ApplicationEntityInstance * aei = instances.erase(instance_id);
		if (aei) {
				aei->set_application_entity(NULL);
		}

		return aei;
}

ApplicationEntityInstance * ApplicationEntity::get_instance(const std::string& instance_id)
{
		return instances.find(instance_id);
}

int ApplicationEntity::select_policy_set_common(const std::string& component,
                                           	   const std::string& path,
                                           	   const std::string& ps_name)
{
        IPolicySet *candidate = NULL;

        if (path != std::string()) {
                LOG_ERR("No subcomponents to address");
                return -1;
        }

        if (!app) {
                LOG_ERR("bug: NULL app reference");
                return -1;
        }

        if (ps_name == selected_ps_name) {
                LOG_INFO("policy set %s already selected", ps_name.c_str());
                return 0;
        }

        candidate = app->psCreate(component, ps_name, this);
        if (!candidate) {
                LOG_ERR("failed to allocate instance of policy set %s", ps_name.c_str());
                return -1;
        }

        if (ps) {
                // Remove the old one.
                app->psDestroy(component, selected_ps_name, ps);
        }

        // Install the new one.
        ps = candidate;
        selected_ps_name = ps_name;
        LOG_INFO("Policy-set %s selected for component %s",
                        ps_name.c_str(), component.c_str());

        return ps ? 0 : -1;
}

int ApplicationEntity::set_policy_set_param_common(const std::string& path,
                                              const std::string& param_name,
                                              const std::string& param_value)
{
        LOG_DBG("set_policy_set_param(%s, %s) called",
                param_name.c_str(), param_value.c_str());

        if (!app) {
                LOG_ERR("bug: NULL app reference");
                return -1;
        }

        if (path == selected_ps_name) {
                // This request is for the currently selected
                // policy set, forward to it
                return ps->set_policy_set_param(param_name, param_value);
        } else if (path != std::string()) {
                LOG_ERR("Invalid component address '%s'", path.c_str());
                return -1;
        }

        // This request is for the component itself
        LOG_ERR("No such parameter '%s' exists", param_name.c_str());

        return -1;
}

//Class Application Process
ApplicationProcess::~ApplicationProcess()
{
	entities.deleteValues();
}

const std::string& ApplicationProcess::get_name() const
{
		return name_;
}

const std::string& ApplicationProcess::get_instance() const
{
		return instance_;
}

void ApplicationProcess::add_entity(ApplicationEntity * entity)
{
		if (!entity) {
				LOG_ERR("Bogus entity passed, returning");
				return;
		}

		entity->set_application_process(this);
		entities.put(entity->get_name(), entity);
}

ApplicationEntity * ApplicationProcess::remove_entity(const std::string& name)
{
		ApplicationEntity * ae = entities.erase(name);
		if (ae) {
				ae->set_application_process(NULL);
		}

		return ae;
}

ApplicationEntity * ApplicationProcess::get_entity(const std::string& name)
{
		return entities.find(name);
}

}

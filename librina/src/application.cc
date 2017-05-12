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

#include <dlfcn.h>
#include <list>

#define RINA_PREFIX "librina.application"

#include "librina/logs.h"
#include "core.h"
#include "librina/application.h"
#include "librina/plugin-info.h"

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

ApplicationProcess * ApplicationEntity::get_application_process()
{
        return app;
}

void ApplicationEntity::add_instance(ApplicationEntityInstance * instance)
{
        if (!instance)
        {
                LOG_ERR("Bogus AE instance passed, returning");
                return;
        }

        instance->set_application_entity(this);
        instances.put(instance->get_instance_id(), instance);
}

ApplicationEntityInstance * ApplicationEntity::remove_instance(
                const std::string& instance_id)
{
        ApplicationEntityInstance * aei = instances.erase(instance_id);
        if (aei)
        {
                aei->set_application_entity(NULL);
        }

        return aei;
}

ApplicationEntityInstance * ApplicationEntity::get_instance(
                const std::string& instance_id)
{
        return instances.find(instance_id);
}

std::list<ApplicationEntityInstance*> ApplicationEntity::get_all_instances()
{
        return instances.getEntries();
}

int ApplicationEntity::select_policy_set(const std::string& path,
                                         const std::string& name)
{
        return select_policy_set_common(get_name(), path, name);
}

int ApplicationEntity::set_policy_set_param(const std::string& path,
                                            const std::string& name,
                                            const std::string& value)
{    
        return set_policy_set_param_common(path, name, value);
}

int ApplicationEntity::select_policy_set_common(const std::string& component,
                                                const std::string& path,
                                                const std::string& ps_name)
{
        IPolicySet *candidate = NULL;
        
        if (path != std::string())
        {
                LOG_ERR("No subcomponents to address");
                return -1;
        }

        if (!app)
        {
                LOG_ERR("bug: NULL app reference");
                return -1;
        }

        if (ps_name == selected_ps_name)
        {
                LOG_INFO("policy set %s already selected for component %s",
                         ps_name.c_str(), component.c_str());
                return 0;
        }
        
        candidate = app->psCreate(component, ps_name, this);
        if (!candidate)
        {
                LOG_ERR("failed to allocate instance of policy set %s",
                        ps_name.c_str());
                return -1;
        }

        if (ps)
        {
                // Remove the old one.
                app->psDestroy(component, selected_ps_name, ps);
        }

        // Install the new one.
        ps = candidate;
        selected_ps_name = ps_name;
        LOG_INFO("Policy-set %s selected for component %s", ps_name.c_str(),
                 component.c_str());

        return ps ? 0 : -1;
}

int ApplicationEntity::set_policy_set_param_common(
                const std::string& path, const std::string& param_name,
                const std::string& param_value)
{
        LOG_DBG("set_policy_set_param(%s, %s) called", param_name.c_str(),
                param_value.c_str());

        if (!app)
        {
                LOG_ERR("bug: NULL app reference");
                return -1;
        }

        if (path == selected_ps_name)
        {
                // This request is for the currently selected
                // policy set, forward to it
                return ps->set_policy_set_param(param_name, param_value);
        } else if (path != std::string())
        {
                LOG_ERR("Invalid component address '%s'", path.c_str());
                return -1;
        }

        // This request is for the component itself
        LOG_ERR("No such parameter '%s' exists", param_name.c_str());

        return -1;
}

const std::string ApplicationEntity::IRM_AE_NAME = "ipc-resource-manager";
const std::string ApplicationEntity::RIB_DAEMON_AE_NAME = "rib-daemon";
const std::string ApplicationEntity::ENROLLMENT_TASK_AE_NAME = "enrollment-task";
const std::string ApplicationEntity::INTERNAL_EVENT_MANAGER_AE_NAME =
                "event-manager";
const std::string ApplicationEntity::SECURITY_MANAGER_AE_NAME =
                "security-manager";

//Class App Policy Manager

AppPolicyManager::~AppPolicyManager()
{
}

void AppPolicyManager::clear_policies()
{
        for(std::vector<rina::PsFactory>::iterator it =
                        ae_policy_factories.begin();
                        it != ae_policy_factories.end(); ++it)
        {
                it->refcnt = 0;
        }
        for (std::map<std::string, void *>::iterator it =
                        plugins_handles.begin(); it != plugins_handles.end();
                        it++)
        {
                plugin_unload(it->first);
        }
        plugins_handles.clear();
}

std::vector<rina::PsFactory>::iterator AppPolicyManager::psFactoryLookup(
                const PsInfo& ps_info)
{
        ReadScopedLock g(rwlock);

        for (std::vector<PsFactory>::iterator it = ae_policy_factories.begin();
                        it != ae_policy_factories.end(); it++)
        {
                if (it->info.app_entity == ps_info.app_entity
                                && it->info.name == ps_info.name)
                {
                        return it;
                }
        }

        return ae_policy_factories.end();
}

int AppPolicyManager::psFactoryPublish(const PsFactory& factory,
                                       const std::list<PsInfo>& manifest_psets)
{
        bool in_manifest = false;

        for (std::list<rina::PsInfo>::const_iterator mi =
                        manifest_psets.begin(); mi != manifest_psets.end();
                        mi++)
        {
                if (*mi == factory.info)
                {
                        in_manifest = true;
                        break;
                }
        }

        if (!in_manifest)
        {
                LOG_WARN("Plugin %s contains policy set factory %s " "but such factory was not declared in the " "manifest. Discarding it.",
                         factory.plugin_name.c_str(),
                         factory.info.toString().c_str());
                return -1;
        }

        if (psFactoryLookup(factory.info) != ae_policy_factories.end())
        {
                LOG_ERR("Factory %s in plugin %s already " "published. Discarding it.",
                        factory.info.toString().c_str(),
                        factory.plugin_name.c_str());
                return -1;
        }

        if (!factory.create || !factory.destroy)
        {
                LOG_ERR("Factory %s in plugin %s has invalid factory " "methods: constructor=%p, destructor=%p. " "Discarding it.",
                        factory.info.toString().c_str(),
                        factory.plugin_name.c_str(), factory.create,
                        factory.destroy);
                return -1;
        }

        /* Add the new factory to the internal factory list. */
        WriteScopedLock g(rwlock);

        ae_policy_factories.push_back(factory);
        ae_policy_factories.back().refcnt = 0;

        LOG_INFO("Pluggable component '%s' [%s] published",
                 factory.info.toString().c_str(), factory.plugin_name.c_str());

        return 0;
}

int AppPolicyManager::psFactoryUnpublish(const PsInfo& ps_info)
{
        std::vector<PsFactory>::iterator fi;

        fi = psFactoryLookup(ps_info);
        if (fi == ae_policy_factories.end())
        {
                LOG_ERR("Factory %s for component %s not " "published",
                        ps_info.name.c_str(), ps_info.app_entity.c_str());
                return -1;
        }

        WriteScopedLock g(rwlock);
        LOG_INFO("Pluggable component '%s'/'%s' [%s] unpublished",
                 fi->info.app_entity.c_str(), fi->info.name.c_str(),
                 fi->plugin_name.c_str());

        ae_policy_factories.erase(fi);

        return 0;
}

IPolicySet * AppPolicyManager::psCreate(const std::string& component,
                                        const std::string& name,
                                        ApplicationEntity * ae)
{
        std::vector<PsFactory>::iterator it;
        IPolicySet *ps = NULL;

        it = psFactoryLookup(PsInfo(name, component, std::string()));
        if (it == ae_policy_factories.end())
        {
                LOG_ERR("Pluggable component %s/%s not found",
                        component.c_str(), name.c_str());
                return NULL;
        }

        ps = it->create(ae);
        if (ps)
        {
                it->refcnt++;
        }

        return ps;
}

int AppPolicyManager::psDestroy(const std::string& component,
                                const std::string& name, IPolicySet * instance)
{
        std::vector<PsFactory>::iterator it;

        it = psFactoryLookup(PsInfo(name, component, std::string()));
        if (it == ae_policy_factories.end())
        {
                LOG_ERR("Pluggable component %s/%s not found",
                        component.c_str(), name.c_str());
                return -1;
        }

        it->destroy(instance);
        it->refcnt--;

        return 0;
}

int AppPolicyManager::plugin_load(const std::string& plugin_dir,
                                  const std::string& plugin_name)
{
        std::vector<struct rina::PsFactory> factories;
        std::list<PsInfo> manifest_psets;
        std::string plugin_path = plugin_dir;
        void *handle = NULL;
        rina::plugin_get_factories_t get_factories;
        char *errstr;
        int ret;

        if (plugins_handles.count(plugin_name))
        {
                LOG_INFO("Plugin '%s' already loaded", plugin_name.c_str());
                return 0;
        }

        // Load the manifest
        ret = plugin_get_info(plugin_name, plugin_dir, manifest_psets);
        if (ret)
        {
                LOG_ERR("Failed to load manifest for plugin %s",
                        plugin_name.c_str());
                return ret;
        }

        plugin_path += "/";
        plugin_path += plugin_name + ".so";

        handle = dlopen(plugin_path.c_str(), RTLD_NOW);
        if (!handle)
        {
                LOG_ERR("Cannot load plugin %s: %s", plugin_name.c_str(),
                        dlerror());
                return -1;
        }

        /* Clear any pending error conditions. */
        dlerror();

        /* Try to load the get_factories() function. */
        get_factories = (plugin_get_factories_t) dlsym(handle, "get_factories");

        /* Check if an error occurred in dlsym(). */
        errstr = dlerror();
        if (errstr)
        {
                dlclose(handle);
                LOG_ERR("Failed to link the get_factories() function for plugin %s: %s",
                        plugin_name.c_str(), errstr);
                return -1;
        }

        /* Invoke the plugin initialization function, that will publish
         * pluggable components. */
        ret = get_factories(factories);
        if (ret)
        {
                dlclose(handle);
                LOG_ERR("Failed to initialize plugin %s", plugin_name.c_str());
                return -1;
        }

        plugins_handles[plugin_name] = handle;

        LOG_INFO("Plugin %s loaded successfully", plugin_name.c_str());

        for (unsigned int i = 0; i < factories.size(); i++)
        {
                factories[i].plugin_name = plugin_name;
                psFactoryPublish(factories[i], manifest_psets);
        }

        return 0;
}

int AppPolicyManager::plugin_unload(const std::string& plugin_name)
{
        std::map<std::string, void *>::iterator mit;
        std::vector<PsFactory> unpublish_list;

        mit = plugins_handles.find(plugin_name);
        if (mit == plugins_handles.end())
        {
                LOG_ERR("plugin %s not found", plugin_name.c_str());
                return -1;
        }

        // Look for all the policy-sets published by the plugin
        // Note: Here we assume the plugin name is used as the "name"
        // argument in the psFactoryPublish() calls.
        for (std::vector<PsFactory>::iterator it = ae_policy_factories.begin();
                        it != ae_policy_factories.end(); it++)
        {
                if (it->plugin_name == plugin_name)
                {
                        if (it->refcnt > 0)
                        {
                                LOG_ERR("Cannot unload plugin %s: it is " "in use by %d AE",
                                        plugin_name.c_str(), it->refcnt);
                                return -1;
                        }
                        unpublish_list.push_back(*it);
                }
        }

        // Unpublish all the policy sets published by this plugin
        for (unsigned int i = 0; i < unpublish_list.size(); i++)
        {
                psFactoryUnpublish(unpublish_list[i].info);
        }

        /* Unload the plugin only if */
        dlclose(mit->second);
        plugins_handles.erase(mit);

        LOG_INFO("Plugin %s unloaded successfully", plugin_name.c_str());

        return 0;
}

//Class Application Process
ApplicationProcess::~ApplicationProcess()
{
        clear_policies();
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
        if (!entity)
        {
                LOG_ERR("Bogus entity passed, returning");
                return;
        }

        entity->set_application_process(this);
        entities.put(entity->get_name(), entity);
}

ApplicationEntity * ApplicationProcess::remove_entity(const std::string& name)
{
        ApplicationEntity * ae = entities.erase(name);
        if (ae)
        {
                ae->set_application_process(NULL);
        }

        return ae;
}

ApplicationEntity * ApplicationProcess::get_entity(const std::string& name)
{
        return entities.find(name);
}

std::list<ApplicationEntity*> ApplicationProcess::get_all_entities()
{
        return entities.getEntries();
}

ApplicationEntity * ApplicationProcess::get_ipc_resource_manager()
{
        return get_entity(ApplicationEntity::IRM_AE_NAME);
}

ApplicationEntity * ApplicationProcess::get_rib_daemon()
{
        return get_entity(ApplicationEntity::RIB_DAEMON_AE_NAME);
}

ApplicationEntity * ApplicationProcess::get_enrollment_task()
{
        return get_entity(ApplicationEntity::ENROLLMENT_TASK_AE_NAME);
}

ApplicationEntity * ApplicationProcess::get_internal_event_manager()
{
        return get_entity(ApplicationEntity::INTERNAL_EVENT_MANAGER_AE_NAME);
}

ApplicationEntity * ApplicationProcess::get_security_manager()
{
        return get_entity(ApplicationEntity::SECURITY_MANAGER_AE_NAME);
}

}

/*
 * Application
 *
 *    Eduard Grasa          <eduard.grasa@i2cat.net>
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
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

#ifndef LIBRINA_APPLICATION_H
#define LIBRINA_APPLICATION_H

#ifdef __cplusplus

#include <map>
#include <string>
#include <list>

#include "librina/patterns.h"
#include "librina/concurrency.h"
#include "librina/ipc-api.h"

/**
 * The librina-application library provides a set of classes to
 * facilitate the creation of distributed applications based on the
 * DAF model.
 *
 * The entities of that applications are programmable via policies.
 * librina-application provides the base classes of the user-space
 * RINA plugin infrastructure (uRPI)
 */
namespace rina {

// A set of policies for a specific application entity
class IPolicySet {
public:
        virtual int set_policy_set_param(const std::string& name,
                                         const std::string& value) = 0;
        virtual ~IPolicySet() {}

        static const std::string DEFAULT_PS_SET_NAME;
};

class ApplicationEntity;

// An instance of an application entity
class ApplicationEntityInstance {
public:
		ApplicationEntityInstance(const std::string& instance_id)
			: instance_id_(instance_id), ae(NULL) { };
		virtual ~ApplicationEntityInstance(){};
		const std::string& get_instance_id() const;
		virtual void set_application_entity(ApplicationEntity * ae) = 0;

protected:
		//The AE Instance, immutable during the AE's lifetime
		std::string instance_id_;

		//The Application Entity this instance is part of
		ApplicationEntity * ae;
};

class AppPolicyManager;

class ApplicationProcess;

// Contains all the data and functions required to access
// and configure the AE Policy Set
class AEPolicySet {
public:
		AEPolicySet() : ps(NULL) { };
		virtual ~AEPolicySet() { };
        virtual int select_policy_set(const std::string& path,
                                      const std::string& name) {
                // TODO it will be pure virtual as soon as overridden
                // by all existing components
                (void) (path+name);
                return -1;
        }
        virtual int set_policy_set_param(const std::string& path,
                                         const std::string& name,
                                         const std::string& value) {
                // TODO it will be pure virtual as soon as overridden
                // by all existing components
                (void) (path+name+value);
                return -1;
        }

        virtual int select_policy_set_common(const std::string& component,
                                     	 	 const std::string& path,
                                     	 	 const std::string& ps_name) = 0;
        virtual int set_policy_set_param_common(const std::string& path,
                                        		const std::string& param_name,
                                        		const std::string& param_value) = 0;

		//The policy set of this AE
		IPolicySet * ps;

		//The name of the selected policy set
		std::string selected_ps_name;
};

// A type of component of an application process, manages all the instances
// of this type
class ApplicationEntity : public AEPolicySet {
public:
		ApplicationEntity(const std::string& name)
						: name_(name), app(NULL) { };
		virtual ~ApplicationEntity();
		const std::string& get_name() const;
		virtual void set_application_process(ApplicationProcess * ap) = 0;
		void add_instance(ApplicationEntityInstance * instance);
		ApplicationEntityInstance * remove_instance(const std::string& instance_id);
		ApplicationEntityInstance * get_instance(const std::string& instance_id);
		std::list<ApplicationEntityInstance*> get_all_instances();

        int select_policy_set_common(const std::string& component,
                                     const std::string& path,
                                     const std::string& ps_name);
        int set_policy_set_param_common(const std::string& path,
                                        const std::string& param_name,
                                        const std::string& param_value);

protected:
		//The Application Entity name, immutable during the AE's lifetime
		std::string name_;

		//A reference to the application process that hosts the AE
		ApplicationProcess * app;

private:
		// The Application entity instances in this application entity
		ThreadSafeMapOfPointers<std::string, ApplicationEntityInstance> instances;
};

extern "C" {
        typedef IPolicySet *(*app_entity_factory_create_t)(
                                                ApplicationEntity * ctx);
        typedef void (*app_entity_factory_destroy_t)(IPolicySet * ps);
        typedef int (*plugin_init_function_t)(AppPolicyManager * app_process,
                                              const std::string& plugin_name);
}

struct PsFactory {
        // Name of this pluggable policy set.
        std::string name;

        // Name of the AE where this plugin applies.
        std::string app_entity;

        // Name of the plugin that published this policy set
        std::string plugin_name;

        // Constructor method for instances of this pluggable policy set.
        app_entity_factory_create_t create;

        // Destructor method for instances of this pluggable policy set.
        app_entity_factory_destroy_t destroy;

        // Reference counter for the number of policy sets created
        // by this factory
        unsigned int refcnt;
};

// A class that can manage the policies of an application process
class AppPolicyManager {
public:
		AppPolicyManager() { };
		virtual ~AppPolicyManager();
		virtual std::vector<PsFactory>::iterator
                    psFactoryLookup(const std::string& ae_name,
                                    const std::string& name);
		virtual int psFactoryPublish(const PsFactory& factory);
		virtual int psFactoryUnpublish(const std::string& ae_name,
                                   	   const std::string& name);
		virtual IPolicySet * psCreate(const std::string& ae_name,
                                  const std::string& name,
                                  ApplicationEntity * context);
		virtual int psDestroy(const std::string& ae_name,
                          	  const std::string& name,
                          	  IPolicySet * instance);

protected:
		int plugin_load(const std::string& plugin_dir,
						const std::string& name);
		int plugin_unload(const std::string& name);

private:
		std::vector<rina::PsFactory> ae_policy_factories;
		std::map< std::string, void * > plugins_handles;
};

// The base class for an Application Process that is member of a
// distributed application and is configurable via policies
class ApplicationProcess : public AppPolicyManager {
public:
		ApplicationProcess(const std::string& name, const std::string& instance)
						: name_(name), instance_(instance) { };
		virtual ~ApplicationProcess();
		const std::string& get_name() const;
		const std::string& get_instance() const;
		void add_entity(ApplicationEntity * entity);
		ApplicationEntity * remove_entity(const std::string& name);
		ApplicationEntity * get_entity(const std::string& name);
		std::list<ApplicationEntity*> get_all_entities();

protected:
		// The ApplicationProcess name, immutable during the AP's lifetime
		std::string name_;

		// The ApplicationProcess instance, immutable during the AP's lifetime
		std::string instance_;

private:
		// The Application entities in this application process
		ThreadSafeMapOfPointers<std::string, ApplicationEntity> entities;
};

}

#endif

#endif

/*
 * DIF Allocator (resolves application names to DIFs)
 *
 *    Eduard Grasa <eduard.grasa@i2cat.net>
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

#ifndef __DIF_ALLOCATOR_H__
#define __DIF_ALLOCATOR_H__

#include <map>

#include <librina/concurrency.h>

#include "rina-configuration.h"

namespace rinad {

/// The DIF Allocator configuration
class DIFAllocatorConfig {
public:
	/// The type of DIF Allocator
	std::string type;

	/// The name of the DIF Allocator DAP instance
	rina::ApplicationProcessNamingInformation dap_name;

	/// The name of the DIF Allocator DAF
	rina::ApplicationProcessNamingInformation daf_name;

	/// The security manager configuration (Authentication & SDU Protection)
	rina::SecurityManagerConfiguration sec_config;

	/// Other configuration parameters as name/value pairs
	std::list<rina::Parameter> parameters;
};

typedef enum da_res{
	//Success
	DA_SUCCESS = 0,

	//Return value will be deferred
	DA_PENDING = 1,

	//Generic failure
	DA_FAILURE = -1,
} da_res_t;

/// Abstract class that must be extended by DIF Allocator implementations
class DIFAllocator {
public:
	DIFAllocator(){};
	virtual ~DIFAllocator(void){};

	/// Create a new instance and configure it, returning the DIF Allocator
	/// name to register to all normal DIFs if any
	static DIFAllocator * create_instance(const rinad::RINAConfiguration& config,
					      rina::ApplicationProcessNamingInformation& da_name);

	/// Parse the DIF Allocator configuration information from the main config file
	static void parse_config(DIFAllocatorConfig& da_config,
				 const rinad::RINAConfiguration& config);

	/// Returns 0 is configuration is correclty applied, -1 otherwise
	virtual int set_config(const DIFAllocatorConfig& da_config,
			       rina::ApplicationProcessNamingInformation& da_name) = 0;

	/// A local app (or IPCP) has registered to an N-1 DIF
	virtual void local_app_registered(const rina::ApplicationProcessNamingInformation& local_app_name,
					  const rina::ApplicationProcessNamingInformation& dif_name) = 0;

	/// A local app (or IPCP) has unregistered from an N-1 DIF
	virtual void local_app_unregistered(const rina::ApplicationProcessNamingInformation& local_app_name,
					    const rina::ApplicationProcessNamingInformation& dif_name) = 0;

	/// Returns IPCM_SUCCESS on success, IPCM_ERROR on error and IPCM_ONGOING
	/// if the operation is still in progress
	virtual da_res_t lookup_dif_by_application(const rina::ApplicationProcessNamingInformation& app_name,
        			       	     	   rina::ApplicationProcessNamingInformation& result,
						   const std::list<std::string>& supported_difs) = 0;
        virtual void update_directory_contents() = 0;

private:
        static void populate_with_default_conf(DIFAllocatorConfig& da_config,
        				       const std::string& config_file);
};

/// Static DIF Allocator, reads Application to DIF mappings from a config file
class StaticDIFAllocator : public DIFAllocator {
public:
	static const std::string TYPE;
	static const std::string DIF_DIRECTORY_FILE_NAME;

	StaticDIFAllocator();
	virtual ~StaticDIFAllocator(void);
	int set_config(const DIFAllocatorConfig& da_config,
		       rina::ApplicationProcessNamingInformation& da_name);
	void local_app_registered(const rina::ApplicationProcessNamingInformation& local_app_name,
			          const rina::ApplicationProcessNamingInformation& dif_name);
	void local_app_unregistered(const rina::ApplicationProcessNamingInformation& local_app_name,
			            const rina::ApplicationProcessNamingInformation& dif_name);
	da_res_t lookup_dif_by_application(const rina::ApplicationProcessNamingInformation& app_name,
        			       	   rina::ApplicationProcessNamingInformation& result,
					   const std::list<std::string>& supported_difs);
        void update_directory_contents();

private:
        void print_directory_contents();

        std::string folder_name;
        std::string fq_file_name;

	//The current DIF Directory
	std::list< std::pair<std::string, std::string> > dif_directory;

	rina::ReadWriteLockable directory_lock;
};

class DynamicDIFAllocator : public DIFAllocator {
public:
	static const std::string TYPE;

	DynamicDIFAllocator();
	virtual ~DynamicDIFAllocator(void);
	int set_config(const DIFAllocatorConfig& da_config,
		       rina::ApplicationProcessNamingInformation& da_name);
	void local_app_registered(const rina::ApplicationProcessNamingInformation& local_app_name,
			          const rina::ApplicationProcessNamingInformation& dif_name);
	void local_app_unregistered(const rina::ApplicationProcessNamingInformation& local_app_name,
			            const rina::ApplicationProcessNamingInformation& dif_name);
	da_res_t lookup_dif_by_application(const rina::ApplicationProcessNamingInformation& app_name,
        			       	   rina::ApplicationProcessNamingInformation& result,
					   const std::list<std::string>& supported_difs);
        void update_directory_contents();

private:

	/// The name of the DIF Allocator DAP instance
	rina::ApplicationProcessNamingInformation dap_name;

	/// The name of the DIF Allocator DAF
	rina::ApplicationProcessNamingInformation daf_name;
};

} //namespace rinad

#endif  /* __DIF_ALLOCATOR_H__ */

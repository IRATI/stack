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
#include "ipcm.h"

namespace rinad {

/// Static DIF Allocator, reads Application to DIF mappings from a config file
class StaticDIFAllocator : public DIFAllocator {
public:
	static const std::string TYPE;

	StaticDIFAllocator();
	virtual ~StaticDIFAllocator(void);
	int set_config(const DIFAllocatorConfig& da_config);
	da_res_t lookup_dif_by_application(const rina::ApplicationProcessNamingInformation& app_name,
        			       	   rina::ApplicationProcessNamingInformation& result,
					   const std::list<std::string>& supported_difs);
	void app_registered(const rina::ApplicationProcessNamingInformation & app_name,
				    const std::string& dif_name);
	void app_unregistered(const rina::ApplicationProcessNamingInformation & app_name,
				      const std::string& dif_name);
        void update_directory_contents();
        void assigned_to_dif(const std::string& dif_name);
        void list_da_mappings(std::ostream& os);

private:
        void print_directory_contents();

        std::string folder_name;
        std::string fq_file_name;

	//The current DIF Directory
	std::list< std::pair<std::string, std::string> > dif_directory;

	rina::ReadWriteLockable directory_lock;
};

class DDARIBDaemon;
class DDAEnrollmentTask;
class DDAEnrollerWorker;
class DDAFlowAcceptor;
class SDUReader;

class DynamicDIFAllocator : public DIFAllocator, public rina::ApplicationProcess {
public:
	static const std::string TYPE;

	DynamicDIFAllocator(const rina::ApplicationProcessNamingInformation& ap_name,
			    IPCManager_ * ipcm);
	virtual ~DynamicDIFAllocator(void);
	int set_config(const DIFAllocatorConfig& da_config);
	da_res_t lookup_dif_by_application(const rina::ApplicationProcessNamingInformation& app_name,
        			       	   rina::ApplicationProcessNamingInformation& result,
					   const std::list<std::string>& supported_difs);
	void app_registered(const rina::ApplicationProcessNamingInformation & app_name,
			    const std::string& dif_name);
	void app_unregistered(const rina::ApplicationProcessNamingInformation & app_name,
			      const std::string& dif_name);
        void update_directory_contents();
        void assigned_to_dif(const std::string& dif_name);
        void n1_flow_allocated(const rina::Neighbor& neighbor, int fd);
        void n1_flow_accepted(const char * incomingapn, int fd);
        void disconnect_from_peer(int fd);
        void enrollment_completed(const rina::cdap_rib::con_handle_t &con);
        unsigned int get_address() const;

        void unregister_app(const rina::ApplicationProcessNamingInformation& app_name,
			    const std::string& dif_name,
			    bool notify_neighs,
			    std::list<int>& neighs_to_exclude);

        void register_app(const std::list<AppToDIFMapping> & mappings,
        		  bool notify_neighs,
			  std::list<int>& neights_to_exclude);

        void list_da_mappings(std::ostream& os);

	/// The name of the DIF Allocator DAP instance
	rina::ApplicationProcessNamingInformation dap_name;

	DDARIBDaemon * ribd;
	DDAEnrollmentTask * et;

private:
	IPCManager_ * ipcm;
	DDAEnrollerWorker * dda_enroller;

	/// The name of the DIF Allocator DAF
	rina::ApplicationProcessNamingInformation daf_name;

	/// Peer DA instances to enroll to
	std::list<rina::Neighbor> enrollments;

	/// Readers of N-1 flows
	rina::Lockable lock;
	std::map<int, SDUReader *> sdu_readers;

	std::map<int, DDAFlowAcceptor *> flow_acceptors;
	std::map<std::string, AppToDIFMapping *> app_dif_mappings;

	bool contains_entry(int candidate, const std::list<int>& elements);
};

} //namespace rinad

#endif  /* __DIF_ALLOCATOR_H__ */

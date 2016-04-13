/*
 * RIB Daemon
 *
 *    Bernat Gaston <bernat.gaston@i2cat.net>
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

#ifndef IPCP_RIB_DAEMON_HH
#define IPCP_RIB_DAEMON_HH

#include <librina/cdap_v2.h>
#include <librina/concurrency.h>

#include "common/concurrency.h"
#include "ipcp/components.h"

namespace rinad {

/// Reads sdus from the Kernel IPC Process, and
/// passes them to the RIB Daemon
class ManagementSDUReaderData {
public:
	ManagementSDUReaderData(unsigned int max_sdu_size);
	unsigned int max_sdu_size_;
};


/// RIBDaemon RIB Object
class RIBDaemonRO: public rina::rib::RIBObj{
public:
        RIBDaemonRO(rina::rib::rib_handle_t rib_);
        void read(const rina::cdap_rib::con_handle_t &con,
                  const std::string& fqn,
                  const std::string& class_,
                  const rina::cdap_rib::filt_info_t &filt,
                  const int invoke_id,
                  rina::cdap_rib::obj_info_t &obj_reply,
                  rina::cdap_rib::res_info_t& res);
private:
        //Handle to the RIB
        rina::rib::rib_handle_t rib;
};

/// The RIB Daemon will start a thread that continuously tries to retrieve management
/// SDUs directed to this IPC Process
void * doManagementSDUReaderWork(void* data);

///Full implementation of the RIB Daemon
class IPCPRIBDaemonImpl : public IPCPRIBDaemon, public rina::InternalEventListener {
public:
	IPCPRIBDaemonImpl(rina::cacep::AppConHandlerInterface *app_con_callback);
	rina::rib::RIBDaemonProxy * getProxy();
        void set_application_process(rina::ApplicationProcess * ap);
        void set_dif_configuration(const rina::DIFConfiguration& dif_configuration);
        void eventHappened(rina::InternalEvent * event);
        void processQueryRIBRequestEvent(const rina::QueryRIBRequestEvent& event);
        const rina::rib::rib_handle_t & get_rib_handle();
        int64_t addObjRIB(const std::string& fqn, rina::rib::RIBObj** obj);
        void removeObjRIB(const std::string& fqn);

private:
	void initialize_rib_daemon(rina::cacep::AppConHandlerInterface *app_con_callback);

	//Handle to the RIB
	rina::rib::rib_handle_t rib;

        INMinusOneFlowManager * n_minus_one_flow_manager_;
        rina::Thread * management_sdu_reader_;

        /// Lock to control that when sending a message requiring a reply the
        /// CDAP Session manager has been updated before receiving the response message
        rina::Lockable atomic_send_lock_;

        void subscribeToEvents();

        /// Invoked by the Resource allocator when it detects that a certain flow has been deallocated, and therefore a
        /// any CDAP sessions over it should be terminated.
        void nMinusOneFlowDeallocated(int portId);
        void nMinusOneFlowAllocated(rina::NMinusOneFlowAllocatedEvent * event);
};

} //namespace rinad

#endif //IPCP_RIB_DAEMON_HH

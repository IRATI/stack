//
// Core
//
//    Eduard Grasa          <eduard.grasa@i2cat.net>
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

#include <errno.h>
#include <sstream>
#include <unistd.h>

#define RINA_PREFIX "librina.core"

#include "librina/logs.h"
#include "librina/configuration.h"
#include "librina/ipc-process.h"
#include "librina/ipc-manager.h"
#include "core.h"
#include "ctrl.h"
#include "irati/serdes-utils.h"

namespace rina {

char * stringToCharArray(std::string s)
{
	char * result = new char[s.size() + 1];
	result[s.size()] = '\0';
	memcpy(result, s.c_str(), s.size());
	return result;
}

char * intToCharArray(int i)
{
	std::stringstream strs;
	strs << i;
	return stringToCharArray(strs.str());
}

IRATICtrlManager::IRATICtrlManager()
{
	ctrl_port = 0;
	cfd = 0;
	next_seq_number = 1;
}

void IRATICtrlManager::initialize()
{
	// Open a control device
	cfd = irati_open_ctrl_port(ctrl_port);
	if (ctrl_port == 0)
		ctrl_port = get_app_ctrl_port_from_cfd(cfd);

	if (cfd < 0) {
		LOG_ERR("Error initializing IRATI Ctrl manager");
		LOG_ERR("Program will exit now");
		exit(-1);
	}

	LOG_DBG("Initialized IRTI Ctrl Manager");
}

irati_msg_port_t IRATICtrlManager::get_irati_ctrl_port(void)
{
	ScopedLock g(irati_ctr_lock);
	return ctrl_port;
}

void IRATICtrlManager::set_irati_ctrl_port(irati_msg_port_t irati_ctrl_port)
{
	ScopedLock g(irati_ctr_lock);
	ctrl_port = irati_ctrl_port;
}

int IRATICtrlManager::get_ctrl_fd(void)
{
	return cfd;
}

IRATICtrlManager::~IRATICtrlManager()
{
	if (close_port(cfd)) {
		LOG_ERR("Problems closing file descriptor %d in control device",
			cfd);
	}
}

unsigned int IRATICtrlManager::get_next_seq_number()
{
	unsigned int result = 0;

	if (next_seq_number == 0)
		next_seq_number = 1;

	result = next_seq_number;
	next_seq_number++;

	return result;
}

int IRATICtrlManager::send_msg(struct irati_msg_base *msg, bool fill_seq_num)
{
	if (fill_seq_num) {
		sendReceiveLock.lock();
		msg->event_id = get_next_seq_number();
		sendReceiveLock.unlock();
	}

	msg->src_port = ctrl_port;

	return irati_write_msg(cfd, msg);
}

IPCEvent * IRATICtrlManager::irati_ctrl_msg_to_ipc_event(struct irati_msg_base *msg)
{
	IPCEvent * event = 0;

	switch (msg->msg_type) {
	case RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST: {
		struct irati_kmsg_ipcm_assign_to_dif * sp_msg =
				(struct irati_kmsg_ipcm_assign_to_dif *) msg;

		DIFInformation dif_info(sp_msg->dif_config, sp_msg->dif_name,
					sp_msg->type);
		event = new AssignToDIFRequestEvent(dif_info, msg->event_id,
						    msg->src_port, msg->src_ipcp_id);
		break;
	}
	case RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE: {
		struct irati_msg_base_resp * sp_msg =
				(struct irati_msg_base_resp *) msg;
		event = new AssignToDIFResponseEvent(sp_msg->result, msg->event_id,
						     msg->src_port, msg->src_ipcp_id);
		break;
	}
	case RINA_C_IPCM_UPDATE_DIF_CONFIG_REQUEST: {
		struct irati_kmsg_ipcm_update_config * sp_msg =
				(struct irati_kmsg_ipcm_update_config *) msg;

		DIFConfiguration dif_config;
		DIFConfiguration::from_c_dif_config(dif_config, sp_msg->dif_config);
		event = new UpdateDIFConfigurationRequestEvent(dif_config, msg->event_id,
							       msg->src_port, msg->src_ipcp_id);
		break;
	}
	case RINA_C_IPCM_UPDATE_DIF_CONFIG_RESPONSE: {
		struct irati_msg_base_resp * sp_msg =
				(struct irati_msg_base_resp *) msg;
		event = new UpdateDIFConfigurationResponseEvent(sp_msg->result, msg->event_id,
							        msg->src_port, msg->src_ipcp_id);
		break;
	}
	case RINA_C_IPCM_IPC_PROCESS_DIF_UNREGISTRATION_NOTIFICATION:
	case RINA_C_IPCM_IPC_PROCESS_DIF_REGISTRATION_NOTIFICATION: {
		struct irati_kmsg_ipcp_dif_reg_not * sp_msg =
				(struct irati_kmsg_ipcp_dif_reg_not *) msg;
		 event = new IPCProcessDIFRegistrationEvent(ApplicationProcessNamingInformation(sp_msg->ipcp_name),
				 	 	 	    ApplicationProcessNamingInformation(sp_msg->dif_name),
							    sp_msg->is_registered, msg->event_id,
							    msg->src_port, msg->src_ipcp_id);
		break;
	}
	case RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_ARRIVED:{
		struct irati_kmsg_ipcm_allocate_flow * sp_msg =
				(struct irati_kmsg_ipcm_allocate_flow *) msg;

		event = new FlowRequestEvent(sp_msg->port_id, FlowSpecification(sp_msg->fspec), false,
			                     ApplicationProcessNamingInformation(sp_msg->local),
					     ApplicationProcessNamingInformation(sp_msg->remote),
					     ApplicationProcessNamingInformation(sp_msg->dif_name),
					     sp_msg->event_id, msg->src_port, msg->src_ipcp_id, 0);
		break;
	}
	case RINA_C_IPCM_ALLOCATE_FLOW_REQUEST: {
		struct irati_kmsg_ipcm_allocate_flow * sp_msg =
				(struct irati_kmsg_ipcm_allocate_flow *) msg;

		event = new FlowRequestEvent(-1, FlowSpecification(sp_msg->fspec), true,
			                     ApplicationProcessNamingInformation(sp_msg->local),
					     ApplicationProcessNamingInformation(sp_msg->remote),
					     ApplicationProcessNamingInformation(sp_msg->dif_name),
					     sp_msg->event_id, msg->src_port, msg->src_ipcp_id, 0);
		break;
	}
	case RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_RESULT: {
		struct irati_kmsg_multi_msg * sp_msg =
				(struct irati_kmsg_multi_msg *) msg;
		event = new IpcmAllocateFlowRequestResultEvent(sp_msg->result, sp_msg->port_id,
				       	       	       	       sp_msg->event_id, msg->src_port,
							       msg->src_ipcp_id);
		break;
	}
	case RINA_C_IPCM_ALLOCATE_FLOW_RESPONSE: {
		struct irati_kmsg_ipcm_allocate_flow_resp * sp_msg =
				(struct irati_kmsg_ipcm_allocate_flow_resp *) msg;
		event = new AllocateFlowResponseEvent(sp_msg->result, sp_msg->notify_src,
                                		      sp_msg->src_ipcp_id, sp_msg->event_id,
						      msg->src_port, msg->src_ipcp_id, 0);
		break;
	}
	case RINA_C_IPCM_DEALLOCATE_FLOW_REQUEST: {
		struct irati_kmsg_multi_msg * sp_msg =
				(struct irati_kmsg_multi_msg *) msg;
		event = new FlowDeallocateRequestEvent(sp_msg->port_id, sp_msg->event_id,
						       msg->src_port, msg->src_ipcp_id);
		break;
	}
	case RINA_C_IPCM_FLOW_DEALLOCATED_NOTIFICATION: {
		struct irati_kmsg_multi_msg * sp_msg =
				(struct irati_kmsg_multi_msg *) msg;
		event = new FlowDeallocatedEvent(sp_msg->port_id, sp_msg->result,
						 msg->src_port, msg->src_ipcp_id);
		break;
	}
	case RINA_C_IPCM_REGISTER_APPLICATION_REQUEST: {
		struct irati_kmsg_ipcm_reg_app * sp_msg =
				(struct irati_kmsg_ipcm_reg_app *) msg;
		ApplicationRegistrationInformation information =
				ApplicationRegistrationInformation(APPLICATION_REGISTRATION_SINGLE_DIF);
		information.difName = ApplicationProcessNamingInformation(sp_msg->dif_name);
		information.appName = ApplicationProcessNamingInformation(sp_msg->app_name);
		information.dafName = ApplicationProcessNamingInformation(sp_msg->daf_name);
		information.ipcProcessId = sp_msg->reg_ipcp_id;
		event = new ApplicationRegistrationRequestEvent(information, sp_msg->event_id,
							        msg->src_port, msg->src_ipcp_id);
		break;
	}
	case RINA_C_IPCM_REGISTER_APPLICATION_RESPONSE: {
		struct irati_msg_base_resp * sp_msg =
				(struct irati_msg_base_resp *) msg;
		event = new IpcmRegisterApplicationResponseEvent(sp_msg->result, sp_msg->event_id,
								 msg->src_port, msg->src_ipcp_id);
		break;
	}
	case RINA_C_IPCM_UNREGISTER_APPLICATION_REQUEST: {
		struct irati_kmsg_ipcm_unreg_app * sp_msg =
				(struct irati_kmsg_ipcm_unreg_app *) msg;
		event = new ApplicationUnregistrationRequestEvent(ApplicationProcessNamingInformation(sp_msg->app_name),
								  ApplicationProcessNamingInformation(sp_msg->dif_name),
								  sp_msg->event_id, msg->src_port, msg->src_ipcp_id);
		break;
	}
	case RINA_C_IPCM_UNREGISTER_APPLICATION_RESPONSE: {
		struct irati_msg_base_resp * sp_msg =
				(struct irati_msg_base_resp *) msg;
		event = new IpcmUnregisterApplicationResponseEvent(sp_msg->result, sp_msg->event_id,
								   msg->src_port, msg->src_ipcp_id);
		break;
	}
	case RINA_C_IPCM_QUERY_RIB_REQUEST: {
		struct irati_kmsg_ipcm_query_rib * sp_msg =
				(struct irati_kmsg_ipcm_query_rib *) msg;
		std::string oc, on, fil;
		if (sp_msg->object_class)
			oc = sp_msg->object_class;
		if (sp_msg->object_name)
			on = sp_msg->object_name;
		if (sp_msg->filter)
			fil = sp_msg->filter;
		event = new QueryRIBRequestEvent(oc, on, sp_msg->object_instance,
						 sp_msg->scope, fil, sp_msg->event_id,
						 msg->src_port, msg->src_ipcp_id);
		break;
	}
	case RINA_C_IPCM_QUERY_RIB_RESPONSE: {
		std::list<rib::RIBObjectData> objects;
		rib::RIBObjectData object;
		struct rib_object_data * pos;
		struct irati_kmsg_ipcm_query_rib_resp * sp_msg =
				(struct irati_kmsg_ipcm_query_rib_resp *) msg;
		if (sp_msg->rib_entries) {
		        list_for_each_entry(pos, &(sp_msg->rib_entries->rib_object_data_entries), next) {
		        	if (pos->clazz)
		        		object.class_ = pos->clazz;
		        	if (pos->disp_value)
		        		object.displayable_value_ = pos->disp_value;
		        	if (pos->name)
		        		object.name_ = pos->name;
				object.instance_ = pos->instance;
				objects.push_back(object);
		        }
		}
		event = new QueryRIBResponseEvent(objects, sp_msg->result, sp_msg->event_id,
						  msg->src_port, msg->src_ipcp_id);
		break;
	}
	case RINA_C_RMT_DUMP_FT_REPLY: {
		std::list<PDUForwardingTableEntry> entries;
		struct mod_pff_entry * pos;
		struct irati_kmsg_rmt_dump_ft * sp_msg =
				(struct irati_kmsg_rmt_dump_ft *) msg;
		if (sp_msg->pft_entries) {
		        list_for_each_entry(pos, &(sp_msg->pft_entries->pff_entries), next) {
				PDUForwardingTableEntry entry;
				PDUForwardingTableEntry::from_c_pff_entry(entry, pos);
				entries.push_back(entry);
		        }
		}
		event = new DumpFTResponseEvent(entries, sp_msg->result, sp_msg->event_id,
						msg->src_port, msg->src_ipcp_id);
		break;
	}
	case RINA_C_IPCP_CONN_CREATE_RESPONSE: {
		struct irati_kmsg_ipcp_conn_update * sp_msg =
				(struct irati_kmsg_ipcp_conn_update *) msg;
		event = new CreateConnectionResponseEvent(sp_msg->port_id, sp_msg->src_cep,
							  sp_msg->event_id, msg->src_port,
							  msg->src_ipcp_id);
		break;
	}
	case RINA_C_IPCP_CONN_CREATE_RESULT: {
		struct irati_kmsg_ipcp_conn_update * sp_msg =
				(struct irati_kmsg_ipcp_conn_update *) msg;
		event = new CreateConnectionResultEvent(sp_msg->port_id, sp_msg->src_cep,
	                        			sp_msg->dst_cep, sp_msg->event_id,
							msg->src_port, msg->src_ipcp_id);
		break;
	}
	case RINA_C_IPCP_CONN_UPDATE_RESULT: {
		struct irati_kmsg_multi_msg * sp_msg =
				(struct irati_kmsg_multi_msg *) msg;
		event = new UpdateConnectionResponseEvent(sp_msg->port_id, sp_msg->result,
				  	  	  	  sp_msg->event_id, msg->src_port,
							  msg->src_ipcp_id);
		break;
	}
	case RINA_C_IPCP_CONN_DESTROY_RESULT: {
		struct irati_kmsg_multi_msg * sp_msg =
				(struct irati_kmsg_multi_msg *) msg;
		event = new DestroyConnectionResultEvent(sp_msg->port_id, sp_msg->result,
				 	 	 	 sp_msg->event_id, msg->src_port,
							 msg->src_ipcp_id);
		break;
	}
	case RINA_C_IPCP_SET_POLICY_SET_PARAM_REQUEST: {
		struct irati_kmsg_ipcp_select_ps_param * sp_msg =
				(struct irati_kmsg_ipcp_select_ps_param *) msg;
		event = new SetPolicySetParamRequestEvent(sp_msg->path, sp_msg->name,
	                        			  sp_msg->value, sp_msg->event_id,
							  msg->src_port, msg->src_ipcp_id);
		break;
	}
	case RINA_C_IPCP_SET_POLICY_SET_PARAM_RESPONSE: {
		struct irati_msg_base_resp * sp_msg =
				(struct irati_msg_base_resp *) msg;
		event = new SetPolicySetParamResponseEvent(sp_msg->result, sp_msg->event_id,
							   msg->src_port, msg->src_ipcp_id);
		break;
	}
	case RINA_C_IPCP_SELECT_POLICY_SET_REQUEST: {
		struct irati_kmsg_ipcp_select_ps * sp_msg =
				(struct irati_kmsg_ipcp_select_ps *) msg;
		event = new SelectPolicySetRequestEvent(sp_msg->path, sp_msg->name,
	                        			sp_msg->event_id, msg->src_port,
							msg->src_ipcp_id);
		break;
	}
	case RINA_C_IPCP_SELECT_POLICY_SET_RESPONSE: {
		struct irati_msg_base_resp * sp_msg =
				(struct irati_msg_base_resp *) msg;
		event = new SelectPolicySetResponseEvent(sp_msg->result, sp_msg->event_id,
							 msg->src_port, msg->src_ipcp_id);
		break;
	}
	case RINA_C_IPCP_UPDATE_CRYPTO_STATE_RESPONSE: {
		struct irati_kmsg_multi_msg * sp_msg =
				(struct irati_kmsg_multi_msg *) msg;
		event = new UpdateCryptoStateResponseEvent(sp_msg->result, sp_msg->port_id,
				   	   	   	   sp_msg->event_id, msg->src_port,
							   msg->src_ipcp_id);
		break;
	}
	case RINA_C_IPCP_ALLOCATE_PORT_RESPONSE: {
		struct irati_kmsg_multi_msg * sp_msg =
				(struct irati_kmsg_multi_msg *) msg;
		event = new AllocatePortResponseEvent(sp_msg->result, sp_msg->port_id,
				      	      	      sp_msg->event_id, msg->src_port,
						      msg->src_ipcp_id);
		break;
	}
	case RINA_C_IPCP_DEALLOCATE_PORT_RESPONSE: {
		struct irati_kmsg_multi_msg * sp_msg =
				(struct irati_kmsg_multi_msg *) msg;
		event = new DeallocatePortResponseEvent(sp_msg->result, sp_msg->port_id,
				      	      	        sp_msg->event_id, msg->src_port,
							msg->src_ipcp_id);
		break;
	}
	case RINA_C_IPCP_MANAGEMENT_SDU_WRITE_RESPONSE: {
		struct irati_msg_base_resp * sp_msg =
				(struct irati_msg_base_resp *) msg;
		event = new WriteMgmtSDUResponseEvent(sp_msg->result, sp_msg->event_id,
						      msg->src_port, msg->src_ipcp_id);
		break;
	}
	case RINA_C_IPCP_MANAGEMENT_SDU_READ_NOTIF: {
		struct irati_kmsg_ipcp_mgmt_sdu * sp_msg =
				(struct irati_kmsg_ipcp_mgmt_sdu *) msg;
		event = new ReadMgmtSDUResponseEvent(0, sp_msg->sdu,
						     sp_msg->port_id, 0,
						     msg->src_port, msg->src_ipcp_id);
		break;
	}
	case RINA_C_IPCM_CREATE_IPCP_RESPONSE: {
		struct irati_msg_base_resp * sp_msg =
				(struct irati_msg_base_resp *) msg;
		event = new CreateIPCPResponseEvent(sp_msg->result, sp_msg->event_id,
						    msg->src_port, msg->src_ipcp_id);
		break;
	}
	case RINA_C_IPCM_DESTROY_IPCP_RESPONSE: {
		struct irati_msg_base_resp * sp_msg =
				(struct irati_msg_base_resp *) msg;
		event = new DestroyIPCPResponseEvent(sp_msg->result, sp_msg->event_id,
						     msg->src_port, msg->src_ipcp_id);
		break;
	}
	case RINA_C_IPCM_ENROLL_TO_DIF_REQUEST: {
		struct irati_msg_ipcm_enroll_to_dif * sp_msg =
				(struct irati_msg_ipcm_enroll_to_dif *) msg;
		event = new EnrollToDAFRequestEvent(ApplicationProcessNamingInformation(sp_msg->dif_name),
                                		    ApplicationProcessNamingInformation(sp_msg->sup_dif_name),
						    ApplicationProcessNamingInformation(sp_msg->neigh_name),
						    sp_msg->prepare_for_handover,
						    ApplicationProcessNamingInformation(sp_msg->disc_neigh_name),
						    sp_msg->event_id, msg->src_port, msg->src_ipcp_id);
		break;
	}
	case RINA_C_IPCM_ENROLL_TO_DIF_RESPONSE: {
		std::list<Neighbor> neighbors;
		Neighbor nei;
		struct ipcp_neighbor_entry * pos;
		struct irati_msg_ipcm_enroll_to_dif_resp * sp_msg =
				(struct irati_msg_ipcm_enroll_to_dif_resp *) msg;
		DIFInformation dif_info(sp_msg->dif_config, sp_msg->dif_name, sp_msg->dif_type);
		if (sp_msg->neighbors) {
		        list_for_each_entry(pos, &(sp_msg->neighbors->ipcp_neighbors), next) {
		        	Neighbor::from_c_neighbor(nei, pos->entry);
		        	neighbors.push_back(nei);
		        }
		}
		event = new EnrollToDIFResponseEvent(neighbors, dif_info, sp_msg->result,
						     sp_msg->event_id, msg->src_port, msg->src_ipcp_id);
		break;
	}
	case RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_REQUEST: {
		struct irati_msg_with_name * sp_msg =
				(struct irati_msg_with_name *) msg;
		event = new DisconnectNeighborRequestEvent(ApplicationProcessNamingInformation(sp_msg->name),
							   sp_msg->event_id, msg->src_port, msg->src_ipcp_id);
		break;
	}
	case RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_RESPONSE: {
		struct irati_msg_base_resp * sp_msg =
				(struct irati_msg_base_resp *) msg;
		event = new DisconnectNeighborResponseEvent(sp_msg->result, sp_msg->event_id,
							    msg->src_port, msg->src_ipcp_id);
		break;
	}
	case RINA_C_IPCM_IPC_PROCESS_INITIALIZED: {
		struct irati_msg_with_name * sp_msg =
				(struct irati_msg_with_name *) msg;
		event = new IPCProcessDaemonInitializedEvent(sp_msg->src_ipcp_id,
							     ApplicationProcessNamingInformation(sp_msg->name),
							     sp_msg->event_id, msg->src_port,
							     msg->src_ipcp_id);
		break;
	}
	case RINA_C_APP_ALLOCATE_FLOW_REQUEST: {
		struct irati_kmsg_ipcm_allocate_flow * sp_msg =
				(struct irati_kmsg_ipcm_allocate_flow *) msg;
		event = new FlowRequestEvent(FlowSpecification(sp_msg->fspec), true,
					     ApplicationProcessNamingInformation(sp_msg->local),
					     ApplicationProcessNamingInformation(sp_msg->remote),
					     sp_msg->src_ipcp_id, sp_msg->event_id,
					     msg->src_port, msg->src_ipcp_id, sp_msg->pid);
		((FlowRequestEvent *) event)->DIFName = ApplicationProcessNamingInformation(sp_msg->dif_name);
		break;
	}
	case RINA_C_APP_ALLOCATE_FLOW_REQUEST_RESULT: {
		struct irati_msg_app_alloc_flow_result * sp_msg =
				(struct irati_msg_app_alloc_flow_result *) msg;
		event = new AllocateFlowRequestResultEvent(ApplicationProcessNamingInformation(sp_msg->source_app_name),
							   ApplicationProcessNamingInformation(sp_msg->dif_name),
							   sp_msg->port_id, sp_msg->event_id,
							   msg->src_port, msg->src_ipcp_id);
		break;
	}
	case RINA_C_APP_ALLOCATE_FLOW_REQUEST_ARRIVED: {
		struct irati_kmsg_ipcm_allocate_flow * sp_msg =
				(struct irati_kmsg_ipcm_allocate_flow *) msg;
		event = new FlowRequestEvent(sp_msg->port_id, FlowSpecification(sp_msg->fspec), false,
					     ApplicationProcessNamingInformation(sp_msg->local),
					     ApplicationProcessNamingInformation(sp_msg->remote),
					     ApplicationProcessNamingInformation(sp_msg->dif_name),
					     sp_msg->event_id, msg->src_port, msg->src_ipcp_id,
					     sp_msg->pid);
		break;
	}
	case RINA_C_APP_ALLOCATE_FLOW_RESPONSE: {
		struct irati_msg_app_alloc_flow_response * sp_msg =
				(struct irati_msg_app_alloc_flow_response *) msg;
		event = new AllocateFlowResponseEvent(sp_msg->result, sp_msg->not_source,
						      sp_msg->src_ipcp_id, sp_msg->event_id,
						      msg->src_port, msg->src_ipcp_id, sp_msg->pid);
		break;
	}
	case RINA_C_APP_DEALLOCATE_FLOW_REQUEST: {
		struct irati_msg_app_dealloc_flow * sp_msg =
				(struct irati_msg_app_dealloc_flow *) msg;
		event = new FlowDeallocateRequestEvent(sp_msg->port_id, sp_msg->event_id,
						       msg->src_port, msg->src_ipcp_id);
		break;
	}
	case RINA_C_APP_REGISTER_APPLICATION_REQUEST: {
		struct irati_msg_app_reg_app * sp_msg =
				(struct irati_msg_app_reg_app *) msg;
		ApplicationRegistrationInformation ari;
		ari.appName = ApplicationProcessNamingInformation(sp_msg->app_name);
		ari.applicationRegistrationType =  (ApplicationRegistrationType) sp_msg->reg_type;
		ari.dafName = ApplicationProcessNamingInformation(sp_msg->daf_name);
		ari.difName = ApplicationProcessNamingInformation(sp_msg->dif_name);
		ari.ipcProcessId = sp_msg->ipcp_id;
		ari.ctrl_port = sp_msg->fa_ctrl_port;
		ari.pid = sp_msg->pid;
		event = new ApplicationRegistrationRequestEvent(ari, msg->event_id,
								msg->src_port, msg->src_ipcp_id);
		break;
	}
	case RINA_C_APP_REGISTER_APPLICATION_RESPONSE: {
		struct irati_msg_app_reg_app_resp * sp_msg =
				(struct irati_msg_app_reg_app_resp *) msg;
		event = new RegisterApplicationResponseEvent(ApplicationProcessNamingInformation(sp_msg->app_name),
							     ApplicationProcessNamingInformation(sp_msg->dif_name),
							     sp_msg->result, sp_msg->event_id,
							     msg->src_port, msg->src_ipcp_id);
		break;
	}
	case RINA_C_APP_UNREGISTER_APPLICATION_REQUEST: {
		struct irati_msg_app_reg_app_resp * sp_msg =
				(struct irati_msg_app_reg_app_resp *) msg;
		event = new ApplicationUnregistrationRequestEvent(ApplicationProcessNamingInformation(sp_msg->app_name),
								  ApplicationProcessNamingInformation(sp_msg->dif_name),
								  sp_msg->event_id, msg->src_port, msg->src_ipcp_id);
		break;
	}
	case RINA_C_APP_UNREGISTER_APPLICATION_RESPONSE: {
		struct irati_msg_app_reg_app_resp * sp_msg =
				(struct irati_msg_app_reg_app_resp *) msg;
		event = new UnregisterApplicationResponseEvent(ApplicationProcessNamingInformation(sp_msg->app_name),
                                			       sp_msg->result, sp_msg->event_id,
							       msg->src_port, msg->src_ipcp_id);
		break;
	}
	case RINA_C_APP_APPLICATION_REGISTRATION_CANCELED_NOTIFICATION: {
		struct irati_msg_app_reg_cancel * sp_msg =
				(struct irati_msg_app_reg_cancel *) msg;
		event = new AppRegistrationCanceledEvent(sp_msg->code, sp_msg->reason,
							 ApplicationProcessNamingInformation(sp_msg->dif_name),
							 sp_msg->event_id, msg->src_port, msg->src_ipcp_id);
		break;
	}
	case RINA_C_APP_GET_DIF_PROPERTIES_REQUEST: {
		struct irati_msg_app_reg_app_resp * sp_msg =
				(struct irati_msg_app_reg_app_resp *) msg;
		event = new GetDIFPropertiesRequestEvent(ApplicationProcessNamingInformation(sp_msg->app_name),
							 ApplicationProcessNamingInformation(sp_msg->dif_name),
							 sp_msg->event_id, msg->src_port, msg->src_ipcp_id);
		break;
	}
	case RINA_C_APP_GET_DIF_PROPERTIES_RESPONSE: {
		std::list<DIFProperties> properties;
		struct dif_properties_entry * pos;
		struct irati_msg_get_dif_prop * sp_msg =
				(struct irati_msg_get_dif_prop *) msg;
		if (sp_msg->dif_props) {
		        list_for_each_entry(pos, &(sp_msg->dif_props->dif_propery_entries), next) {
				DIFProperties prop;
				prop.DIFName = ApplicationProcessNamingInformation(pos->dif_name);
				prop.maxSDUSize = pos->max_sdu_size;
				properties.push_back(prop);
		        }
		}
		event =  new GetDIFPropertiesResponseEvent(ApplicationProcessNamingInformation(sp_msg->app_name),
                                			   properties, sp_msg->code, sp_msg->event_id,
							   msg->src_port, msg->src_ipcp_id);
		break;
	}
	case RINA_C_IPCM_PLUGIN_LOAD_REQUEST: {
		struct irati_msg_ipcm_plugin_load * sp_msg =
				(struct irati_msg_ipcm_plugin_load *) msg;
		event = new PluginLoadRequestEvent(sp_msg->plugin_name, sp_msg->load,
	                              		   sp_msg->event_id, msg->src_port, msg->src_ipcp_id);
		break;
	}
	case RINA_C_IPCM_PLUGIN_LOAD_RESPONSE: {
		struct irati_msg_base_resp * sp_msg =
				(struct irati_msg_base_resp *) msg;
		event = new PluginLoadResponseEvent(sp_msg->result, sp_msg->event_id,
						    msg->src_port, msg->src_ipcp_id);
		break;
	}
	case RINA_C_IPCM_FWD_CDAP_MSG_RESPONSE:
	case RINA_C_IPCM_FWD_CDAP_MSG_REQUEST: {
		ser_obj_t ser;
		struct irati_msg_ipcm_fwd_cdap_msg * sp_msg =
				(struct irati_msg_ipcm_fwd_cdap_msg *) msg;
		ser.size_ = sp_msg->cdap_msg->size;
		ser.message_ = new unsigned char[ser.size_];
		memcpy(ser.message_, sp_msg->cdap_msg->data, ser.size_);

		if (msg->msg_type == RINA_C_IPCM_FWD_CDAP_MSG_REQUEST) {
			event = new FwdCDAPMsgRequestEvent(ser, sp_msg->result, sp_msg->event_id,
							   msg->src_port, msg->src_ipcp_id);
		} else {
			event = new FwdCDAPMsgResponseEvent(ser, sp_msg->result, sp_msg->event_id,
							    msg->src_port, msg->src_ipcp_id);
		}
		break;
	}
	case RINA_C_IPCM_MEDIA_REPORT: {
		MediaReport report;
		struct irati_msg_ipcm_media_report * sp_msg =
				(struct irati_msg_ipcm_media_report *) msg;
		MediaReport::from_c_media_report(report, sp_msg->report);
		event = new MediaReportEvent(report, sp_msg->event_id,
					      msg->src_port, msg->src_ipcp_id);
		break;
	}
	case RINA_C_IPCM_FINALIZE_REQUEST: {
		event = new IPCMFinalizationRequestEvent();
		break;
	}
	case RINA_C_IPCM_SCAN_MEDIA_REQUEST: {
		event = new ScanMediaRequestEvent(msg->event_id, msg->src_port,
						  msg->src_ipcp_id);
		break;
	}
	default: {
		LOG_WARN("Unrecognized ctrl message type: %d", msg->msg_type);
		event = 0;
	}
	}

	return event;
}

IPCEvent * IRATICtrlManager::get_next_ctrl_msg()
{
	struct irati_msg_base * msg;
	IPCEvent * event = 0;

	msg = irati_read_next_msg(cfd);
	if (!msg) {
		LOG_ERR("Could not retrieve next ctrl message for fd %d. Errno (%d): %s",
			cfd, errno, strerror(errno));
		return 0;
	}

	event = IRATICtrlManager::irati_ctrl_msg_to_ipc_event(msg);
	if (event) {
		LOG_DBG("Added event of type(%d) %s and sequence number %u to events queue",
				event->eventType,
				IPCEvent::eventTypeToString(event->eventType).c_str(),
				event->sequenceNumber);
	} else
		LOG_WARN("Event is null for message type %d", msg->msg_type);

	irati_ctrl_msg_free(msg);

	return event;
}

Singleton<IRATICtrlManager> irati_ctrl_mgr;

}

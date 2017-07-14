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

/* CLASS CTRL PORT ID MAP */
CtrlPortIdMap::~CtrlPortIdMap(){
        for (std::map<ipc_process_id_t, struct irati_ep *>::iterator iterator
                        = ipcp_id_map.begin();
                        iterator != ipcp_id_map.end(); ++iterator) {
                if (iterator->second) {
                        delete iterator->second;
                }
        }
        for (std::map<std::string, struct irati_ep *>::iterator iterator
                  = app_name_map.begin();
                  iterator != app_name_map.end(); ++iterator) {
                if (iterator->second) {
                        delete iterator->second;
                }
        }
}

void CtrlPortIdMap::add_ipcp_id_to_ctrl_port_mapping(irati_msg_port_t ctrl_port,
		      	      	      	      	     ipc_process_id_t ipcp_id)
{
	struct irati_ep * current;
	bool add = false;

	current = ipcp_id_map[ipcp_id];
	if (!current) {
		current = new irati_ep();
		add = true;
	}

	current->ipcp_id = ipcp_id;
	current->ctrl_port = ctrl_port;

	if (add)
		ipcp_id_map[ipcp_id] = current;
}

struct irati_ep * CtrlPortIdMap::get_ctrl_port_from_ipcp_id(ipc_process_id_t ipcp_id)
{
	std::map<ipc_process_id_t, struct irati_ep *>::iterator it;

	it = ipcp_id_map.find(ipcp_id);
	if (it == ipcp_id_map.end()) {
		LOG_ERR("Could not find the netlink endpoint of IPC Process %d",
			ipcp_id);
		return 0;
	}

	return it->second;
}

void CtrlPortIdMap::add_app_name_to_ctrl_port_map(const ApplicationProcessNamingInformation& app_name,
					   	  irati_msg_port_t ctrl_port,
						  ipc_process_id_t ipcp_id)
{
	struct irati_ep * current;
	bool add = false;

	current = app_name_map[app_name.getProcessNamePlusInstance()];
	if (!current) {
		current = new irati_ep();
		current->app_name = app_name;
		add = true;
	}

	current->ipcp_id = ipcp_id;
	current->ctrl_port = ctrl_port;

	if (add)
		app_name_map[app_name.getProcessNamePlusInstance()] = current;
}

struct irati_ep * CtrlPortIdMap::get_ctrl_port_from_app_name(const ApplicationProcessNamingInformation& app_name)
{
	std::map<std::string, struct irati_ep *>::iterator it ;

	it = app_name_map.find(app_name.getProcessNamePlusInstance());
	if (it == app_name_map.end()) {
		LOG_ERR("Could not find the netlink endpoint of Application %s",
			 app_name.toString().c_str());
		return 0;
	};

	return it->second;
}

irati_msg_port_t CtrlPortIdMap::get_ipcm_ctrl_port()
{
	return IRATI_IPCM_PORT;
}

int CtrlPortIdMap::update_msg_or_pid_map(struct irati_msg_base * msg, bool send)
{
	ApplicationProcessNamingInformation app_name;
	struct irati_ep * irati_ep;

	switch (msg->msg_type) {
	case RINA_C_APP_ALLOCATE_FLOW_REQUEST: {
		if (send) {
			msg->dest_port = get_ipcm_ctrl_port();
		} else {
			struct irati_kmsg_ipcm_allocate_flow * sp_msg =
					(struct irati_kmsg_ipcm_allocate_flow *) msg;

			app_name = ApplicationProcessNamingInformation(sp_msg->source);
			add_app_name_to_ctrl_port_map(app_name, sp_msg->src_port,
						      sp_msg->src_ipcp_id);
		}
		break;
	}
	case RINA_C_APP_ALLOCATE_FLOW_REQUEST_RESULT: {
		struct irati_kmsg_ipcm_allocate_flow * sp_msg =
				(struct irati_kmsg_ipcm_allocate_flow *) msg;
		if (send) {
			app_name = ApplicationProcessNamingInformation(sp_msg->source);
			irati_ep = get_ctrl_port_from_app_name(app_name);
			if (!irati_ep) {
				LOG_ERR("Could not locate IRATI ep for app_name %s",
					app_name.toString().c_str());
				return -1;
			}
			msg->dest_port = irati_ep->ctrl_port;
			msg->dest_ipcp_id = irati_ep->ipcp_id;
		}
		break;
	}
	case RINA_C_APP_ALLOCATE_FLOW_REQUEST_ARRIVED: {
		struct irati_kmsg_ipcm_allocate_flow * sp_msg =
				(struct irati_kmsg_ipcm_allocate_flow *) msg;
		if (send) {
			app_name = ApplicationProcessNamingInformation(sp_msg->dest);
			irati_ep = get_ctrl_port_from_app_name(app_name);
			if (!irati_ep) {
				LOG_ERR("Could not locate IRATI ep for app_name %s",
					app_name.toString().c_str());
				return -1;
			}
			msg->dest_port = irati_ep->ctrl_port;
			msg->dest_ipcp_id = irati_ep->ipcp_id;
		}
		break;
	}
	case RINA_C_APP_FLOW_DEALLOCATED_NOTIFICATION:
	case RINA_C_APP_DEALLOCATE_FLOW_RESPONSE: {
		struct irati_msg_app_dealloc_flow * sp_msg =
				(struct irati_msg_app_dealloc_flow *) msg;
		if (send) {
			app_name = ApplicationProcessNamingInformation(sp_msg->name);
			irati_ep = get_ctrl_port_from_app_name(app_name);
			if (!irati_ep) {
				LOG_ERR("Could not locate IRATI ep for app_name %s",
					app_name.toString().c_str());
				return -1;
			}
			msg->dest_port = irati_ep->ctrl_port;
			msg->dest_ipcp_id = irati_ep->ipcp_id;
		}
		break;
	}
	case RINA_C_APP_REGISTER_APPLICATION_REQUEST: {
		struct irati_msg_app_reg_app * sp_msg =
				(struct irati_msg_app_reg_app *) msg;
		if (send) {
			msg->dest_port = get_ipcm_ctrl_port();
		} else {
			app_name = ApplicationProcessNamingInformation(sp_msg->app_name);
			add_app_name_to_ctrl_port_map(app_name, sp_msg->src_port,
						      sp_msg->src_ipcp_id);
		}
		break;
	}
	case RINA_C_APP_UNREGISTER_APPLICATION_RESPONSE:
	case RINA_C_APP_REGISTER_APPLICATION_RESPONSE: {
		struct irati_msg_app_reg_app_resp * sp_msg =
				(struct irati_msg_app_reg_app_resp *) msg;
		if (send) {
			app_name = ApplicationProcessNamingInformation(sp_msg->app_name);
			irati_ep = get_ctrl_port_from_app_name(app_name);
			if (!irati_ep) {
				LOG_ERR("Could not locate IRATI ep for app_name %s",
					app_name.toString().c_str());
				return -1;
			}
			msg->dest_port = irati_ep->ctrl_port;
			msg->dest_ipcp_id = irati_ep->ipcp_id;
		}
		break;
	}
	case RINA_C_APP_GET_DIF_PROPERTIES_REQUEST:
	case RINA_C_APP_UNREGISTER_APPLICATION_REQUEST: {
		if (send) {
			msg->dest_port = get_ipcm_ctrl_port();
		} else {
			struct irati_msg_app_reg_app_resp * sp_msg =
					(struct irati_msg_app_reg_app_resp *) msg;
			app_name = ApplicationProcessNamingInformation(sp_msg->app_name);
			add_app_name_to_ctrl_port_map(app_name, sp_msg->src_port,
						      sp_msg->src_ipcp_id);
		}
		break;
	}
	case RINA_C_APP_GET_DIF_PROPERTIES_RESPONSE: {
		struct irati_msg_get_dif_prop * sp_msg =
				(struct irati_msg_get_dif_prop *) msg;
		if (send) {
			app_name = ApplicationProcessNamingInformation(sp_msg->app_name);
			irati_ep = get_ctrl_port_from_app_name(app_name);
			if (!irati_ep) {
				LOG_ERR("Could not locate IRATI ep for app_name %s",
					app_name.toString().c_str());
				return -1;
			}
			msg->dest_port = irati_ep->ctrl_port;
			msg->dest_ipcp_id = irati_ep->ipcp_id;
		}
		break;
	}
	case RINA_C_APP_ALLOCATE_FLOW_RESPONSE:
	case RINA_C_APP_DEALLOCATE_FLOW_REQUEST:
	case RINA_C_IPCM_REGISTER_APPLICATION_RESPONSE:
	case RINA_C_IPCM_UNREGISTER_APPLICATION_RESPONSE:
	case RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE:
	case RINA_C_IPCM_QUERY_RIB_RESPONSE:
	case RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_RESULT:
	case RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_ARRIVED:
	case RINA_C_IPCM_DEALLOCATE_FLOW_RESPONSE:
	case RINA_C_IPCM_FLOW_DEALLOCATED_NOTIFICATION:
	case RINA_C_IPCP_SET_POLICY_SET_PARAM_RESPONSE:
	case RINA_C_IPCP_SELECT_POLICY_SET_RESPONSE:
	case RINA_C_IPCM_PLUGIN_LOAD_RESPONSE: {
		if (send) {
			msg->dest_port = get_ipcm_ctrl_port();
		}
		break;
	}
	case RINA_C_IPCM_IPC_PROCESS_INITIALIZED: {
		if (send) {
		} else {
			struct irati_msg_with_name * sp_msg =
					(struct irati_msg_with_name *) msg;

			add_ipcp_id_to_ctrl_port_mapping(msg->src_port, msg->src_ipcp_id);
			app_name = ApplicationProcessNamingInformation(sp_msg->name);
			add_app_name_to_ctrl_port_map(app_name, sp_msg->src_port,
						      sp_msg->src_ipcp_id);
		}
		break;
	}
	default: {
		//Do nothing
	}
	}

	return 0;
}

/**
 * An OS Process has finalized. Retrieve the information associated to
 * the ctrñ port-id (application name, IPC Process id if it is IPC process),
 * and return it in the form of an OSProcessFinalized event
 * @param nl_portid
 * @return
 */
IPCEvent * CtrlPortIdMap::os_process_finalized(irati_msg_port_t ctrl_port)
{
	//1 Try to get application process name, if not there return 0
	ApplicationProcessNamingInformation apNamingInfo;
	std::map<std::string, struct irati_ep *>::iterator iterator;
	std::map<ipc_process_id_t, struct irati_ep *>::iterator iterator2;
	bool foundAppName = false;
	unsigned short ipcp_id = 0;

	for (iterator = app_name_map.begin();
			iterator != app_name_map.end(); ++iterator) {
		if (iterator->second->ctrl_port == ctrl_port) {
			apNamingInfo = iterator->second->app_name;
			foundAppName = true;
			delete iterator->second;
			app_name_map.erase(iterator);
			break;
		}
	}

	if (!foundAppName) {
		return 0;
	}

	//2 Try to get IPC Process id
	for (iterator2 = ipcp_id_map.begin();
			iterator2 != ipcp_id_map.end(); ++iterator2) {
		if (iterator2->second->ctrl_port == ctrl_port) {
			ipcp_id = iterator2->first;
			delete iterator2->second;
			ipcp_id_map.erase(iterator2);
			break;
		}
	}

	OSProcessFinalizedEvent * event =
			new OSProcessFinalizedEvent(apNamingInfo, ipcp_id, 0);
	return event;
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
		ctrl_port = cfd;

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
	unsigned int result;

	if (next_seq_number == 0)
		next_seq_number = 1;

	result = next_seq_number;
	next_seq_number++;

	return result;
}

int IRATICtrlManager::send_msg(struct irati_msg_base *msg, bool fill_seq_num)
{
	int result = 0;

	ScopedLock g(sendReceiveLock);

	result = ctrl_pid_map.update_msg_or_pid_map(msg, true);
	if (result != 0)
		return result;

	if (fill_seq_num)
		msg->event_id = get_next_seq_number();

	msg->src_port = ctrl_port;

	return irati_write_msg(cfd, msg);
}

int IRATICtrlManager::send_msg_max_size(struct irati_msg_base *msg,
					size_t maxSize, bool fill_seq_num)
{
	int result = 0;

	ScopedLock g(sendReceiveLock);

	result = ctrl_pid_map.update_msg_or_pid_map(msg, true);
	if (result != 0)
		return result;

	if (fill_seq_num)
		msg->event_id = get_next_seq_number();

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
		event = new AssignToDIFRequestEvent(dif_info,
						    msg->event_id);
		break;
	}
	case RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE: {
		struct irati_msg_base_resp * sp_msg =
				(struct irati_msg_base_resp *) msg;
		event = new AssignToDIFResponseEvent(sp_msg->result, msg->event_id);
		break;
	}
	case RINA_C_IPCM_UPDATE_DIF_CONFIG_REQUEST: {
		struct irati_kmsg_ipcm_update_config * sp_msg =
				(struct irati_kmsg_ipcm_update_config *) msg;

		DIFConfiguration dif_config;
		DIFConfiguration::from_c_dif_config(dif_config, sp_msg->dif_config);
		event = new UpdateDIFConfigurationRequestEvent(dif_config,
							       msg->event_id);
		break;
	}
	case RINA_C_IPCM_UPDATE_DIF_CONFIG_RESPONSE: {
		struct irati_msg_base_resp * sp_msg =
				(struct irati_msg_base_resp *) msg;
		event = new UpdateDIFConfigurationResponseEvent(sp_msg->result,
								msg->event_id);
		break;
	}
	case RINA_C_IPCM_IPC_PROCESS_DIF_UNREGISTRATION_NOTIFICATION:
	case RINA_C_IPCM_IPC_PROCESS_DIF_REGISTRATION_NOTIFICATION: {
		struct irati_kmsg_ipcp_dif_reg_not * sp_msg =
				(struct irati_kmsg_ipcp_dif_reg_not *) msg;
		 event = new IPCProcessDIFRegistrationEvent(ApplicationProcessNamingInformation(sp_msg->ipcp_name),
				 	 	 	    ApplicationProcessNamingInformation(sp_msg->dif_name),
							    sp_msg->is_registered, msg->event_id);
		break;
	}
	case RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_ARRIVED:
	case RINA_C_IPCM_ALLOCATE_FLOW_REQUEST: {
		struct irati_kmsg_ipcm_allocate_flow * sp_msg =
				(struct irati_kmsg_ipcm_allocate_flow *) msg;
		bool local = true;
		int port_id = -1;
		if (msg->msg_type == RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_ARRIVED) {
			local = false;
			port_id = sp_msg->port_id;
		}
		 event = new FlowRequestEvent(port_id, FlowSpecification(sp_msg->fspec), local,
			                      ApplicationProcessNamingInformation(sp_msg->source),
					      ApplicationProcessNamingInformation(sp_msg->dest),
					      ApplicationProcessNamingInformation(sp_msg->dif_name),
					      sp_msg->src_ipcp_id, sp_msg->event_id);
		break;
	}
	case RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_RESULT: {
		struct irati_kmsg_multi_msg * sp_msg =
				(struct irati_kmsg_multi_msg *) msg;
		event = new IpcmAllocateFlowRequestResultEvent(sp_msg->result, sp_msg->port_id,
				       	       	       	       sp_msg->event_id);
		break;
	}
	case RINA_C_IPCM_ALLOCATE_FLOW_RESPONSE: {
		struct irati_kmsg_ipcm_allocate_flow_resp * sp_msg =
				(struct irati_kmsg_ipcm_allocate_flow_resp *) msg;
		event = new AllocateFlowResponseEvent(sp_msg->result, sp_msg->notify_src,
                                		      sp_msg->src_ipcp_id, sp_msg->event_id);
		break;
	}
	case RINA_C_IPCM_DEALLOCATE_FLOW_RESPONSE: {
		struct irati_msg_base_resp * sp_msg =
				(struct irati_msg_base_resp *) msg;
		event = new IpcmDeallocateFlowResponseEvent(sp_msg->result, sp_msg->event_id);
		break;
	}
	case RINA_C_IPCM_DEALLOCATE_FLOW_REQUEST: {
		struct irati_kmsg_multi_msg * sp_msg =
				(struct irati_kmsg_multi_msg *) msg;
		event = new FlowDeallocateRequestEvent(sp_msg->port_id, sp_msg->event_id);
		break;
	}
	case RINA_C_IPCM_FLOW_DEALLOCATED_NOTIFICATION: {
		struct irati_kmsg_multi_msg * sp_msg =
				(struct irati_kmsg_multi_msg *) msg;
		event = new FlowDeallocatedEvent(sp_msg->port_id, sp_msg->result);
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
		event = new ApplicationRegistrationRequestEvent(information, sp_msg->event_id);
		break;
	}
	case RINA_C_IPCM_REGISTER_APPLICATION_RESPONSE: {
		struct irati_msg_base_resp * sp_msg =
				(struct irati_msg_base_resp *) msg;
		event = new IpcmRegisterApplicationResponseEvent(sp_msg->result, sp_msg->event_id);
		break;
	}
	case RINA_C_IPCM_UNREGISTER_APPLICATION_REQUEST: {
		struct irati_kmsg_ipcm_unreg_app * sp_msg =
				(struct irati_kmsg_ipcm_unreg_app *) msg;
		event = new ApplicationUnregistrationRequestEvent(ApplicationProcessNamingInformation(sp_msg->app_name),
								  ApplicationProcessNamingInformation(sp_msg->dif_name),
								  sp_msg->event_id);
		break;
	}
	case RINA_C_IPCM_UNREGISTER_APPLICATION_RESPONSE: {
		struct irati_msg_base_resp * sp_msg =
				(struct irati_msg_base_resp *) msg;
		event = new IpcmUnregisterApplicationResponseEvent(sp_msg->result, sp_msg->event_id);
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
						 sp_msg->scope, fil, sp_msg->event_id);
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
		event = new QueryRIBResponseEvent(objects, sp_msg->result, sp_msg->event_id);
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
		event = new DumpFTResponseEvent(entries, sp_msg->result, sp_msg->event_id);
		break;
	}
	case RINA_C_IPCP_CONN_CREATE_RESPONSE: {
		struct irati_kmsg_ipcp_conn_update * sp_msg =
				(struct irati_kmsg_ipcp_conn_update *) msg;
		event = new CreateConnectionResponseEvent(sp_msg->port_id, sp_msg->src_cep,
							  sp_msg->event_id);
		break;
	}
	case RINA_C_IPCP_CONN_CREATE_RESULT: {
		struct irati_kmsg_ipcp_conn_update * sp_msg =
				(struct irati_kmsg_ipcp_conn_update *) msg;
		event = new CreateConnectionResultEvent(sp_msg->port_id, sp_msg->src_cep,
	                        			sp_msg->dst_cep, sp_msg->event_id);
		break;
	}
	case RINA_C_IPCP_CONN_UPDATE_RESULT: {
		struct irati_kmsg_multi_msg * sp_msg =
				(struct irati_kmsg_multi_msg *) msg;
		event = new UpdateConnectionResponseEvent(sp_msg->port_id, sp_msg->result,
				  	  	  	  sp_msg->event_id);
		break;
	}
	case RINA_C_IPCP_CONN_DESTROY_RESULT: {
		struct irati_kmsg_multi_msg * sp_msg =
				(struct irati_kmsg_multi_msg *) msg;
		event = new DestroyConnectionResultEvent(sp_msg->port_id, sp_msg->result,
				 	 	 	 sp_msg->event_id);
		break;
	}
	case RINA_C_IPCP_SET_POLICY_SET_PARAM_REQUEST: {
		struct irati_kmsg_ipcp_select_ps_param * sp_msg =
				(struct irati_kmsg_ipcp_select_ps_param *) msg;
		event = new SetPolicySetParamRequestEvent(sp_msg->path, sp_msg->name,
	                        			  sp_msg->value, sp_msg->event_id);
		break;
	}
	case RINA_C_IPCP_SET_POLICY_SET_PARAM_RESPONSE: {
		struct irati_msg_base_resp * sp_msg =
				(struct irati_msg_base_resp *) msg;
		event = new SetPolicySetParamResponseEvent(sp_msg->result, sp_msg->event_id);
		break;
	}
	case RINA_C_IPCP_SELECT_POLICY_SET_REQUEST: {
		struct irati_kmsg_ipcp_select_ps * sp_msg =
				(struct irati_kmsg_ipcp_select_ps *) msg;
		event = new SelectPolicySetRequestEvent(sp_msg->path, sp_msg->name,
	                        			sp_msg->event_id);
		break;
	}
	case RINA_C_IPCP_SELECT_POLICY_SET_RESPONSE: {
		struct irati_msg_base_resp * sp_msg =
				(struct irati_msg_base_resp *) msg;
		event = new SelectPolicySetResponseEvent(sp_msg->result, sp_msg->event_id);
		break;
	}
	case RINA_C_IPCP_UPDATE_CRYPTO_STATE_RESPONSE: {
		struct irati_kmsg_multi_msg * sp_msg =
				(struct irati_kmsg_multi_msg *) msg;
		event = new UpdateCryptoStateResponseEvent(sp_msg->result, sp_msg->port_id,
				   	   	   	   sp_msg->event_id);
		break;
	}
	case RINA_C_IPCP_ALLOCATE_PORT_RESPONSE: {
		struct irati_kmsg_multi_msg * sp_msg =
				(struct irati_kmsg_multi_msg *) msg;
		event = new AllocatePortResponseEvent(sp_msg->result, sp_msg->port_id,
				      	      	      sp_msg->event_id);
		break;
	}
	case RINA_C_IPCP_DEALLOCATE_PORT_RESPONSE: {
		struct irati_kmsg_multi_msg * sp_msg =
				(struct irati_kmsg_multi_msg *) msg;
		event = new DeallocatePortResponseEvent(sp_msg->result, sp_msg->port_id,
				      	      	        sp_msg->event_id);
		break;
	}
	case RINA_C_IPCP_MANAGEMENT_SDU_WRITE_RESPONSE: {
		struct irati_msg_base_resp * sp_msg =
				(struct irati_msg_base_resp *) msg;
		event = new WriteMgmtSDUResponseEvent(sp_msg->result, sp_msg->event_id);
		break;
	}
	case RINA_C_IPCP_MANAGEMENT_SDU_READ_NOTIF: {
		struct irati_kmsg_ipcp_mgmt_sdu * sp_msg =
				(struct irati_kmsg_ipcp_mgmt_sdu *) msg;
		event = new ReadMgmtSDUResponseEvent(0, sp_msg->sdu->data,
						     sp_msg->sdu->size, sp_msg->port_id, 0);
		break;
	}
	case RINA_C_IPCM_CREATE_IPCP_RESPONSE: {
		struct irati_msg_base_resp * sp_msg =
				(struct irati_msg_base_resp *) msg;
		event = new CreateIPCPResponseEvent(sp_msg->result, sp_msg->event_id);
		break;
	}
	case RINA_C_IPCM_DESTROY_IPCP_RESPONSE: {
		struct irati_msg_base_resp * sp_msg =
				(struct irati_msg_base_resp *) msg;
		event = new DestroyIPCPResponseEvent(sp_msg->result, sp_msg->event_id);
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
						    sp_msg->event_id);
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
		event = new EnrollToDIFResponseEvent(neighbors, dif_info, sp_msg->result, sp_msg->event_id);
		break;
	}
	case RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_REQUEST: {
		struct irati_msg_with_name * sp_msg =
				(struct irati_msg_with_name *) msg;
		event = new DisconnectNeighborRequestEvent(ApplicationProcessNamingInformation(sp_msg->name),
							   sp_msg->event_id);
		break;
	}
	case RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_RESPONSE: {
		struct irati_msg_base_resp * sp_msg =
				(struct irati_msg_base_resp *) msg;
		event = new DisconnectNeighborResponseEvent(sp_msg->result, sp_msg->event_id);
		break;
	}
	case RINA_C_IPCM_IPC_PROCESS_INITIALIZED: {
		struct irati_msg_with_name * sp_msg =
				(struct irati_msg_with_name *) msg;
		event = new IPCProcessDaemonInitializedEvent(sp_msg->src_ipcp_id,
							     ApplicationProcessNamingInformation(sp_msg->name),
							     sp_msg->event_id);
		break;
	}
	case RINA_C_APP_ALLOCATE_FLOW_REQUEST: {
		struct irati_kmsg_ipcm_allocate_flow * sp_msg =
				(struct irati_kmsg_ipcm_allocate_flow *) msg;
		event = new FlowRequestEvent(FlowSpecification(sp_msg->fspec), true,
					     ApplicationProcessNamingInformation(sp_msg->source),
					     ApplicationProcessNamingInformation(sp_msg->dest),
					     sp_msg->src_ipcp_id, sp_msg->event_id);
		break;
	}
	case RINA_C_APP_ALLOCATE_FLOW_REQUEST_RESULT: {
		struct irati_msg_app_alloc_flow_result * sp_msg =
				(struct irati_msg_app_alloc_flow_result *) msg;
		event = new AllocateFlowRequestResultEvent(ApplicationProcessNamingInformation(sp_msg->source_app_name),
							   ApplicationProcessNamingInformation(sp_msg->dif_name),
							   sp_msg->port_id, sp_msg->event_id);
		break;
	}
	case RINA_C_APP_ALLOCATE_FLOW_REQUEST_ARRIVED: {
		struct irati_kmsg_ipcm_allocate_flow * sp_msg =
				(struct irati_kmsg_ipcm_allocate_flow *) msg;
		event = new FlowRequestEvent(sp_msg->port_id, FlowSpecification(sp_msg->fspec), false,
					     ApplicationProcessNamingInformation(sp_msg->dest),
					     ApplicationProcessNamingInformation(sp_msg->source),
					     ApplicationProcessNamingInformation(sp_msg->dif_name),
					     sp_msg->src_ipcp_id, sp_msg->event_id);
		break;
	}
	case RINA_C_APP_ALLOCATE_FLOW_RESPONSE: {
		struct irati_msg_app_alloc_flow_response * sp_msg =
				(struct irati_msg_app_alloc_flow_response *) msg;
		event = new AllocateFlowResponseEvent(sp_msg->result, sp_msg->not_source,
						      sp_msg->src_ipcp_id, sp_msg->event_id);
		break;
	}
	case RINA_C_APP_DEALLOCATE_FLOW_REQUEST: {
		struct irati_msg_app_dealloc_flow * sp_msg =
				(struct irati_msg_app_dealloc_flow *) msg;
		event = new FlowDeallocateRequestEvent(sp_msg->port_id,
						       ApplicationProcessNamingInformation(sp_msg->name),
						       sp_msg->event_id);
		break;
	}
	case RINA_C_APP_DEALLOCATE_FLOW_RESPONSE: {
		struct irati_msg_app_dealloc_flow * sp_msg =
				(struct irati_msg_app_dealloc_flow *) msg;
		event = new DeallocateFlowResponseEvent(ApplicationProcessNamingInformation(sp_msg->name),
							sp_msg->port_id, sp_msg->result, sp_msg->event_id);
		break;
	}
	case RINA_C_APP_FLOW_DEALLOCATED_NOTIFICATION: {
		struct irati_msg_app_dealloc_flow * sp_msg =
				(struct irati_msg_app_dealloc_flow *) msg;
		event = new FlowDeallocatedEvent(sp_msg->port_id, sp_msg->result);
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
		event = new ApplicationRegistrationRequestEvent(ari, sp_msg->event_id);
		break;
	}
	case RINA_C_APP_REGISTER_APPLICATION_RESPONSE: {
		struct irati_msg_app_reg_app_resp * sp_msg =
				(struct irati_msg_app_reg_app_resp *) msg;
		event = new RegisterApplicationResponseEvent(ApplicationProcessNamingInformation(sp_msg->app_name),
							     ApplicationProcessNamingInformation(sp_msg->dif_name),
							     sp_msg->result, sp_msg->event_id);
		break;
	}
	case RINA_C_APP_UNREGISTER_APPLICATION_REQUEST: {
		struct irati_msg_app_reg_app_resp * sp_msg =
				(struct irati_msg_app_reg_app_resp *) msg;
		event = new ApplicationUnregistrationRequestEvent(ApplicationProcessNamingInformation(sp_msg->app_name),
								  ApplicationProcessNamingInformation(sp_msg->dif_name),
								  sp_msg->event_id);
		break;
	}
	case RINA_C_APP_UNREGISTER_APPLICATION_RESPONSE: {
		struct irati_msg_app_reg_app_resp * sp_msg =
				(struct irati_msg_app_reg_app_resp *) msg;
		event = new UnregisterApplicationResponseEvent(ApplicationProcessNamingInformation(sp_msg->app_name),
                                			       sp_msg->result, sp_msg->event_id);
		break;
	}
	case RINA_C_APP_APPLICATION_REGISTRATION_CANCELED_NOTIFICATION: {
		struct irati_msg_app_reg_cancel * sp_msg =
				(struct irati_msg_app_reg_cancel *) msg;
		event = new AppRegistrationCanceledEvent(sp_msg->code, sp_msg->reason,
							 ApplicationProcessNamingInformation(sp_msg->dif_name), sp_msg->event_id);
		break;
	}
	case RINA_C_APP_GET_DIF_PROPERTIES_REQUEST: {
		struct irati_msg_app_reg_app_resp * sp_msg =
				(struct irati_msg_app_reg_app_resp *) msg;
		event = new GetDIFPropertiesRequestEvent(ApplicationProcessNamingInformation(sp_msg->app_name),
							 ApplicationProcessNamingInformation(sp_msg->dif_name),
							 sp_msg->event_id);
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
                                			   properties, sp_msg->code, sp_msg->event_id);
		break;
	}
	case RINA_C_IPCM_PLUGIN_LOAD_REQUEST: {
		struct irati_msg_ipcm_plugin_load * sp_msg =
				(struct irati_msg_ipcm_plugin_load *) msg;
		event = new PluginLoadRequestEvent(sp_msg->plugin_name, sp_msg->load,
	                              		   sp_msg->event_id);
		break;
	}
	case RINA_C_IPCM_PLUGIN_LOAD_RESPONSE: {
		struct irati_msg_base_resp * sp_msg =
				(struct irati_msg_base_resp *) msg;
		event = new PluginLoadResponseEvent(sp_msg->result, sp_msg->event_id);
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
			event = new FwdCDAPMsgRequestEvent(ser, sp_msg->result,
		                       	           	   sp_msg->event_id);
		} else {
			event = new FwdCDAPMsgResponseEvent(ser, sp_msg->result,
		                       	           	    sp_msg->event_id);
		}
		break;
	}
	case RINA_C_IPCM_MEDIA_REPORT: {
		MediaReport report;
		struct irati_msg_ipcm_media_report * sp_msg =
				(struct irati_msg_ipcm_media_report *) msg;
		MediaReport::from_c_media_report(report, sp_msg->report);
		event = new MediaReportEvent(report, sp_msg->event_id);
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
		LOG_ERR("Could not retrieve next ctrl message for fd %d", cfd);
		//TODO read errno
		return 0;
	}

	if (msg->msg_type == RINA_C_IPCM_SOCKET_CLOSED_NOTIFICATION) {
		LOG_DBG("Ctrl device at port %d is closed", msg->src_port);

		event = os_process_finalized(msg->src_port);
		if (event) {
			LOG_DBG("Added event of type %s and sequence number %u to events queue",
				 IPCEvent::eventTypeToString(event->eventType).c_str(),
				 event->sequenceNumber);
		}
	} else {
		ctrl_msg_arrived(msg);
		event = IRATICtrlManager::irati_ctrl_msg_to_ipc_event(msg);
		if (event) {
			LOG_DBG("Added event of type %s and sequence number %u to events queue",
				IPCEvent::eventTypeToString(event->eventType).c_str(),
				event->sequenceNumber);
		} else
			LOG_WARN("Event is null for message type %d", msg->msg_type);
	}

	irati_ctrl_msg_free(msg);

	return event;
}

void IRATICtrlManager::ctrl_msg_arrived(struct irati_msg_base *msg)
{
	ScopedLock g(sendReceiveLock);
	ctrl_pid_map.update_msg_or_pid_map(msg, false);
}

IPCEvent * IRATICtrlManager::os_process_finalized(irati_msg_port_t ctrl_port)
{
	ScopedLock g(sendReceiveLock);
	return ctrl_pid_map.os_process_finalized(ctrl_port);
}

Singleton<IRATICtrlManager> irati_ctrl_mgr;

}

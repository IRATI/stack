//
// IPC Manager
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

#include <algorithm>
#include <dirent.h>
#include <errno.h>
#include <iostream>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdlib>

#define RINA_PREFIX "librina.ipc-manager"

#include "librina/logs.h"

#include "librina/ipc-manager.h"
#include "librina/concurrency.h"

#include "utils.h"
#include "core.h"
#include "ctrl.h"

namespace rina {

std::string _installation_path;
std::string _library_path;
std::string _log_path;
std::string _log_level;

void initializeIPCManager(unsigned int        localPort,
                          const std::string & installationPath,
                          const std::string & libraryPath,
                          const std::string & logLevel,
                          const std::string & pathToLogFolder)
{
	std::string ipcmPathToLogFile = pathToLogFolder + "/ipcm.log";
	initialize(localPort, logLevel, ipcmPathToLogFile);

	_installation_path = installationPath;
	_library_path = libraryPath;
	_log_path = pathToLogFolder;
	_log_level = logLevel;
}

void request_ipcm_finalization(unsigned int localPort)
{
	struct irati_msg_base msg;

	msg.src_port = localPort;
	msg.dest_port = localPort;
	msg.msg_type = RINA_C_IPCM_FINALIZE_REQUEST;

        if (irati_ctrl_mgr->send_msg(&msg, false) != 0) {
        	throw IPCException("Problems sending CTRL message");
        }
}

void destroyIPCManager()
{
	librina_finalize();
}

/* CLASS IPC PROCESS*/
const std::string IPCProcessProxy::error_assigning_to_dif =
		"Error assigning IPC Process to DIF";
const std::string IPCProcessProxy::error_update_dif_config =
                "Error updating DIF Configuration";
const std::string IPCProcessProxy::error_registering_app =
		"Error registering application";
const std::string IPCProcessProxy::error_unregistering_app =
		"Error unregistering application";
const std::string IPCProcessProxy::error_not_a_dif_member =
		"Error: the IPC Process is not member of a DIF";
const std::string IPCProcessProxy::error_allocating_flow =
		"Error allocating flow";
const std::string IPCProcessProxy::error_deallocating_flow =
		"Error deallocating flow";
const std::string IPCProcessProxy::error_querying_rib =
		"Error querying rib";


IPCProcessProxy::IPCProcessProxy() {
	id = 0;
	portId = 0;
	pid = 0;
	seq_num = 0;
}

IPCProcessProxy::IPCProcessProxy(unsigned short id, unsigned int portId,
                       pid_t pid, const std::string& type,
                       const ApplicationProcessNamingInformation& name)
{
	this->id = id;
	this->portId = portId;
	this->pid = pid;
	this->type = type;
	this->name = name;
	seq_num = 0;
}

unsigned short IPCProcessProxy::getId() const
{ return id; }

const std::string& IPCProcessProxy::getType() const
{ return type; }

const ApplicationProcessNamingInformation& IPCProcessProxy::getName() const
{ return name; }

unsigned int IPCProcessProxy::getPortId() const
{ return portId; }

void IPCProcessProxy::setPortId(unsigned int portId)
{ this->portId = portId; }

pid_t IPCProcessProxy::getPid() const
{ return pid; }

void IPCProcessProxy::setPid(pid_t pid)
{ this->pid = pid; }

void IPCProcessProxy::assignToDIF(const DIFInformation& difInformation,
				  unsigned int opaque)
{
#if STUB_API
        //Do nothing
#else
        struct irati_kmsg_ipcm_assign_to_dif * msg;

        msg = new irati_kmsg_ipcm_assign_to_dif();
        msg->msg_type = RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST;
        msg->dif_name = difInformation.dif_name_.to_c_name();
        msg->type = stringToCharArray(difInformation.dif_type_);
        msg->dif_config = difInformation.dif_configuration_.to_c_dif_config();
        msg->dest_ipcp_id = id;
        msg->dest_port = portId;
        msg->event_id = opaque;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, false) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif
}

void
IPCProcessProxy::updateDIFConfiguration(const DIFConfiguration& difConfiguration,
					unsigned int opaque)
{
#if STUB_API
        //Do nothing
#else
        struct irati_kmsg_ipcm_update_config * msg;

        msg = new irati_kmsg_ipcm_update_config();
        msg->msg_type = RINA_C_IPCM_UPDATE_DIF_CONFIG_REQUEST;
        msg->dif_config = difConfiguration.to_c_dif_config();
        msg->dest_ipcp_id = id;
        msg->dest_port = portId;
        msg->event_id = opaque;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, false) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        irati_ctrl_msg_free((struct irati_msg_base *) msg);

#endif
}

void IPCProcessProxy::notifyRegistrationToSupportingDIF(const ApplicationProcessNamingInformation& ipcProcessName,
							const ApplicationProcessNamingInformation& difName)
{
#if STUB_API
	//Do nothing
#else
        struct irati_kmsg_ipcp_dif_reg_not * msg;

        msg = new irati_kmsg_ipcp_dif_reg_not();
        msg->msg_type = RINA_C_IPCM_IPC_PROCESS_DIF_REGISTRATION_NOTIFICATION;
        msg->dif_name = difName.to_c_name();
        msg->ipcp_name = ipcProcessName.to_c_name();
        msg->dest_ipcp_id = id;
        msg->dest_port = portId;
        msg->is_registered = true;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, false) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif
}

void IPCProcessProxy::notifyUnregistrationFromSupportingDIF(const ApplicationProcessNamingInformation& ipcProcessName,
							    const ApplicationProcessNamingInformation& difName)
{
#if STUB_API
	//Do nothing
#else
        struct irati_kmsg_ipcp_dif_reg_not * msg;

        msg = new irati_kmsg_ipcp_dif_reg_not();
        msg->msg_type = RINA_C_IPCM_IPC_PROCESS_DIF_UNREGISTRATION_NOTIFICATION;
        msg->dif_name = difName.to_c_name();
        msg->ipcp_name = ipcProcessName.to_c_name();
        msg->dest_ipcp_id = id;
        msg->dest_port = portId;
        msg->is_registered = false;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, false) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif
}

void IPCProcessProxy::enroll(const ApplicationProcessNamingInformation& difName,
			     const ApplicationProcessNamingInformation& supportingDifName,
			     const ApplicationProcessNamingInformation& neighborName,
			     unsigned int opaque)
{
#if STUB_API
        //Do nothing
#else
        struct irati_msg_ipcm_enroll_to_dif * msg;
        ApplicationProcessNamingInformation no_name;

        msg = new irati_msg_ipcm_enroll_to_dif();
        msg->msg_type = RINA_C_IPCM_ENROLL_TO_DIF_REQUEST;
        msg->dif_name = difName.to_c_name();
        msg->sup_dif_name = supportingDifName.to_c_name();
        msg->neigh_name = neighborName.to_c_name();
        msg->dest_ipcp_id = id;
        msg->dest_port = portId;
        msg->prepare_for_handover = false;
        msg->disc_neigh_name = no_name.to_c_name();
        msg->event_id = opaque;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, false) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif
}

void IPCProcessProxy::enroll_prepare_hand(const ApplicationProcessNamingInformation& difName,
			 	 	  const ApplicationProcessNamingInformation& supportingDifName,
					  const ApplicationProcessNamingInformation& neighborName,
					  const ApplicationProcessNamingInformation& disc_neigh_name,
					  unsigned int opaque)
{
#if STUB_API
        //Do nothing
#else
        struct irati_msg_ipcm_enroll_to_dif * msg;

        msg = new irati_msg_ipcm_enroll_to_dif();
        msg->msg_type = RINA_C_IPCM_ENROLL_TO_DIF_REQUEST;
        msg->dif_name = difName.to_c_name();
        msg->sup_dif_name = supportingDifName.to_c_name();
        msg->neigh_name = neighborName.to_c_name();
        msg->prepare_for_handover = true;
        msg->disc_neigh_name = disc_neigh_name.to_c_name();
        msg->dest_ipcp_id = id;
        msg->dest_port = portId;
        msg->event_id = opaque;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, false) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif
}

void IPCProcessProxy::disconnectFromNeighbor(const ApplicationProcessNamingInformation& neighbor,
					     unsigned int opaque)
{
#if STUB_API
        //Do nothing
#else
        struct irati_msg_with_name * msg;

        msg = new irati_msg_with_name();
        msg->msg_type = RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_REQUEST;
        msg->name = neighbor.to_c_name();
        msg->dest_ipcp_id = id;
        msg->dest_port = portId;
        msg->event_id = opaque;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, false) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif
}

void IPCProcessProxy::registerApplication(const ApplicationProcessNamingInformation& applicationName,
					  const ApplicationProcessNamingInformation& dafName,
					  unsigned short regIpcProcessId,
					  const ApplicationProcessNamingInformation& dif_name,
					  unsigned int opaque)
{
#if STUB_API
	//Do nothing
#else
        struct irati_kmsg_ipcm_reg_app * msg;

        msg = new irati_kmsg_ipcm_reg_app();
        msg->msg_type = RINA_C_IPCM_REGISTER_APPLICATION_REQUEST;
        msg->app_name = applicationName.to_c_name();
        msg->daf_name = dafName.to_c_name();
        msg->dif_name = dif_name.to_c_name();
        msg->reg_ipcp_id = regIpcProcessId;
        msg->dest_ipcp_id = id;
        msg->dest_port = portId;
        msg->event_id = opaque;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, false) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif
}

void IPCProcessProxy::unregisterApplication(const ApplicationProcessNamingInformation& applicationName,
					    const ApplicationProcessNamingInformation& dif_name,
					    unsigned int opaque)
{

#if STUB_API
	//Do nothing
#else
        struct irati_kmsg_ipcm_unreg_app * msg;

        msg = new irati_kmsg_ipcm_unreg_app();
        msg->msg_type = RINA_C_IPCM_UNREGISTER_APPLICATION_REQUEST;
        msg->app_name = applicationName.to_c_name();
        msg->dif_name = dif_name.to_c_name();
        msg->dest_ipcp_id = id;
        msg->dest_port = portId;
        msg->event_id = opaque;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, false) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif
}

void IPCProcessProxy::allocateFlow(const FlowRequestEvent& flowRequest,
		unsigned int opaque)
{
#if STUB_API
	//Do nothing
#else
        struct irati_kmsg_ipcm_allocate_flow * msg;

        msg = new irati_kmsg_ipcm_allocate_flow();
        msg->msg_type = RINA_C_IPCM_ALLOCATE_FLOW_REQUEST;
        msg->local = flowRequest.localApplicationName.to_c_name();
        msg->remote = flowRequest.remoteApplicationName.to_c_name();
        msg->dif_name = flowRequest.DIFName.to_c_name();
        msg->src_ipcp_id = flowRequest.flowRequestorIpcProcessId;
        msg->fspec = flowRequest.flowSpecification.to_c_flowspec();
        msg->dest_ipcp_id = id;
        msg->dest_port = portId;
        msg->event_id = opaque;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, false) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif
}

void IPCProcessProxy::allocateFlowResponse(const FlowRequestEvent& flowRequest,
					   int result,
					   bool notifySource,
					   int flowAcceptorIpcProcessId)
{
#if STUB_API
	//Do nothing
#else
        struct irati_kmsg_ipcm_allocate_flow_resp * msg;

        msg = new irati_kmsg_ipcm_allocate_flow_resp();
        msg->msg_type = RINA_C_IPCM_ALLOCATE_FLOW_RESPONSE;
        msg->notify_src = notifySource;
        msg->result = result;
        msg->src_ipcp_id = flowAcceptorIpcProcessId;
        msg->dest_ipcp_id = id;
        msg->dest_port = portId;
        msg->event_id = flowRequest.sequenceNumber;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, false) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif
}

void IPCProcessProxy::deallocateFlow(int flowPortId, unsigned int opaque)
{
#if STUB_API
	//Do nothing
#else
        struct irati_kmsg_multi_msg * msg;

        msg = new irati_kmsg_multi_msg();
        msg->msg_type = RINA_C_IPCM_DEALLOCATE_FLOW_REQUEST;
        msg->port_id = flowPortId;
        msg->dest_ipcp_id = id;
        msg->dest_port = portId;
        msg->event_id = opaque;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, false) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif
}

void IPCProcessProxy::queryRIB(const std::string& objectClass,
		const std::string& objectName, unsigned long objectInstance,
		unsigned int scope, const std::string& filter,
		unsigned int opaque)
{
#if STUB_API
#else
        struct irati_kmsg_ipcm_query_rib * msg;

        msg = new irati_kmsg_ipcm_query_rib();
        msg->msg_type = RINA_C_IPCM_QUERY_RIB_REQUEST;
        msg->object_class = stringToCharArray(objectClass);
        msg->object_name = stringToCharArray(objectName);
        msg->object_instance = objectInstance;
        msg->scope = scope;
        msg->filter = stringToCharArray(filter);
        msg->dest_ipcp_id = id;
        msg->dest_port = portId;
        msg->event_id = opaque;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, false) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif
}

void IPCProcessProxy::setPolicySetParam(const std::string& path,
                        const std::string& name, const std::string& value,
                        unsigned int opaque)
{
#if STUB_API
#else
        struct irati_kmsg_ipcp_select_ps_param * msg;

        msg = new irati_kmsg_ipcp_select_ps_param();
        msg->msg_type = RINA_C_IPCP_SET_POLICY_SET_PARAM_REQUEST;
        msg->path = stringToCharArray(path);
        msg->name = stringToCharArray(name);
        msg->value = stringToCharArray(value);
        msg->dest_ipcp_id = id;
        msg->dest_port = portId;
        msg->event_id = opaque;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, false) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif
}

void IPCProcessProxy::selectPolicySet(const std::string& path,
                                 const std::string& name,
                                 unsigned int opaque)
{
#if STUB_API
#else
        struct irati_kmsg_ipcp_select_ps * msg;

        msg = new irati_kmsg_ipcp_select_ps();
        msg->msg_type = RINA_C_IPCP_SELECT_POLICY_SET_REQUEST;
        msg->path = stringToCharArray(path);
        msg->name = stringToCharArray(name);
        msg->dest_ipcp_id = id;
        msg->dest_port = portId;
        msg->event_id = opaque;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, false) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif
}

void IPCProcessProxy::pluginLoad(const std::string& name, bool load,
		unsigned int opaque)
{
#if STUB_API
#else
        struct irati_msg_ipcm_plugin_load * msg;

        msg = new irati_msg_ipcm_plugin_load();
        msg->msg_type = RINA_C_IPCM_PLUGIN_LOAD_REQUEST;
        msg->plugin_name = stringToCharArray(name);
        msg->load = load;
        msg->dest_ipcp_id = id;
        msg->dest_port = portId;
        msg->event_id = opaque;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, false) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif
}

void IPCProcessProxy::forwardCDAPRequestMessage(const ser_obj_t& sermsg,
					 	unsigned int opaque)
{
#if STUB_API
#else
        struct irati_msg_ipcm_fwd_cdap_msg * msg;

        msg = new irati_msg_ipcm_fwd_cdap_msg();
        msg->msg_type = RINA_C_IPCM_FWD_CDAP_MSG_REQUEST;
        msg->cdap_msg = new buffer();
        msg->cdap_msg->data = new unsigned char[sermsg.size_];
        msg->cdap_msg->size = sermsg.size_;
        memcpy(msg->cdap_msg->data, sermsg.message_, sermsg.size_);
        msg->dest_ipcp_id = id;
        msg->dest_port = portId;
        msg->event_id = opaque;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, false) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif
}

void IPCProcessProxy::forwardCDAPResponseMessage(const ser_obj_t& sermsg,
                                         	 unsigned int opaque)
{
#if STUB_API
#else
        struct irati_msg_ipcm_fwd_cdap_msg * msg;

        msg = new irati_msg_ipcm_fwd_cdap_msg();
        msg->msg_type = RINA_C_IPCM_FWD_CDAP_MSG_RESPONSE;
        msg->cdap_msg = new buffer();
        msg->cdap_msg->data = new unsigned char[sermsg.size_];
        msg->cdap_msg->size = sermsg.size_;
        memcpy(msg->cdap_msg->data, sermsg.message_, sermsg.size_);
        msg->dest_ipcp_id = id;
        msg->dest_port = portId;
        msg->event_id = opaque;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, false) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif
}

void IPCProcessProxy::scan_media(void)
{
#if STUB_API
#else
        struct irati_msg_base * msg;

        msg = new irati_msg_base();
        msg->msg_type = RINA_C_IPCM_SCAN_MEDIA_REQUEST;
        msg->dest_ipcp_id = id;
        msg->dest_port = portId;
        msg->event_id = 0;

        if (irati_ctrl_mgr->send_msg(msg, false) != 0) {
        	irati_ctrl_msg_free(msg);
        	throw IPCException("Problems sending CTRL message");
        }

        irati_ctrl_msg_free(msg);
#endif
}


/** CLASS IPC PROCESS FACTORY */
const std::string IPCProcessFactory::unknown_ipc_process_error =
		"Could not find an IPC Process with the provided id";
const std::string IPCProcessFactory::path_to_ipc_process_types =
		"/sys/rina/ipcp-factories/";
const std::string IPCProcessFactory::normal_ipc_process_type =
		"normal";

IPCProcessFactory::IPCProcessFactory() {
}

IPCProcessFactory::~IPCProcessFactory() throw() {
}

std::list<std::string> IPCProcessFactory::getSupportedIPCProcessTypes() {
	std::list<std::string> result;

	DIR *dp;
	struct dirent *dirp;
	if((dp = opendir(path_to_ipc_process_types.c_str())) == 0) {
		LOG_ERR("Error %d opening %s", errno,
				path_to_ipc_process_types.c_str());
		return result;
	}

	std::string name;
	while ((dirp = readdir(dp)) != 0) {
		name = std::string(dirp->d_name);
		if (name.compare(".") != 0 && name.compare("..") != 0) {
			result.push_back(name);
		}
	}

	closedir(dp);

	return result;
}

IPCProcessProxy * IPCProcessFactory::create(
		const ApplicationProcessNamingInformation& ipcProcessName,
		const std::string& difType,
		unsigned short ipcProcessId)
{
	unsigned int portId = 0;
	unsigned int seq_num = 0;
	pid_t pid=0;

#if STUB_API
	//Do nothing
#else
	if (difType == NORMAL_IPC_PROCESS ||
		difType == SHIM_WIFI_IPC_PROCESS_STA ||
		difType == SHIM_WIFI_IPC_PROCESS_AP)	{
		pid = fork();
		if (pid == 0) {
			//This is the OS process that has to execute the IPC Process
			//program and then exit
			LOG_DBG("New OS Process created, executing IPC Process ...");

			char * argv[] =
			{
                                /* FIXME: These hardwired things must disappear */
				stringToCharArray(_installation_path +"/ipcp"),
				stringToCharArray(ipcProcessName.processName),
				stringToCharArray(ipcProcessName.processInstance),
				intToCharArray(ipcProcessId),
				intToCharArray(irati_ctrl_mgr->get_irati_ctrl_port()),
				stringToCharArray(_log_level),
				stringToCharArray(_log_path + "/" + ipcProcessName.processName
						+ "-" + ipcProcessName.processInstance + ".log"),
				stringToCharArray(difType),
				0
			};
			char * envp[] =
			{
                                /* FIXME: These hardwired things must disappear */
				stringToCharArray("PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"),
				stringToCharArray("LD_LIBRARY_PATH=$LD_LIBRARY_PATH:"
					          +_library_path),
				(char*) 0
			};

			execve(argv[0], &argv[0], envp);

			LOG_ERR("Problems loading IPC Process program, finalizing OS Process with error %s", strerror(errno));

			exit(-1);
		}else if (pid < 0) {
			throw CreateIPCProcessException();
		}else{
			//This is the IPC Manager, and fork was successful
		        portId = pid;
			LOG_DBG("Craeted a new IPC Process with pid = %d", pid);
		}
	}

        struct irati_kmsg_ipcm_create_ipcp * msg;

        msg = new irati_kmsg_ipcm_create_ipcp();
        msg->msg_type = RINA_C_IPCM_CREATE_IPCP_REQUEST;
        msg->dif_type = stringToCharArray(difType);
        msg->ipcp_name = ipcProcessName.to_c_name();
        msg->irati_port_id = portId;
        msg->ipcp_id = ipcProcessId;
        msg->dest_ipcp_id = 0;
        msg->dest_port = 0;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, true) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        seq_num = msg->event_id;
        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif

	IPCProcessProxy * ipcProcess = new IPCProcessProxy(ipcProcessId, portId,
							   pid, difType, ipcProcessName);
	ipcProcess->seq_num = seq_num;
	return ipcProcess;
}

unsigned int IPCProcessFactory::destroy(IPCProcessProxy* ipcp)
{
	int resultUserSpace = 0;
	unsigned int seq_num = 0;

#if STUB_API
	//Do nothing
#else
        struct irati_kmsg_ipcm_destroy_ipcp * msg;

        msg = new irati_kmsg_ipcm_destroy_ipcp();
        msg->msg_type = RINA_C_IPCM_DESTROY_IPCP_REQUEST;
        msg->ipcp_id = ipcp->id;
        msg->dest_ipcp_id = 0;
        msg->dest_port = 0;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, true) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        seq_num = msg->event_id;
        irati_ctrl_msg_free((struct irati_msg_base *) msg);

	if (ipcp->getType() == NORMAL_IPC_PROCESS ||
			ipcp->getType() == SHIM_WIFI_IPC_PROCESS_AP ||
			ipcp->getType() == SHIM_WIFI_IPC_PROCESS_STA)
	{
		// BUG: Zombie processes are automatically reaped using a
		//      SIGCHLD handler, so the following lines often result
		//      in ESRCH (or worse if the IPCP pid was reallocated to
		//      a new process).
		resultUserSpace = kill(ipcp->getPid(), SIGINT);
		// TODO: Change magic number
		usleep(2000000);
		resultUserSpace = kill(ipcp->getPid(), SIGKILL);
	}
#endif

	delete ipcp;

	if (resultUserSpace)
	{
	        std::string error = "Problems destroying IPCP.";
	        error = error + "Result in user space:  " +
	                        intToCharArray(resultUserSpace);
	        LOG_ERR("%s", error.c_str());
	}

	return seq_num;
}

/** CLASS APPLICATION MANAGER */
void ApplicationManager::applicationRegistered(const ApplicationRegistrationRequestEvent& event,
					       const ApplicationProcessNamingInformation& difName, int result)
{
	LOG_DBG("ApplicationManager::applicationRegistered called");

#if STUB_API
	//Do nothing
#else
        struct irati_msg_app_reg_app_resp * msg;

        msg = new irati_msg_app_reg_app_resp();
        msg->msg_type = RINA_C_APP_REGISTER_APPLICATION_RESPONSE;
        msg->app_name = event.applicationRegistrationInformation.appName.to_c_name();
        msg->dif_name = difName.to_c_name();
        msg->result = result;
        msg->event_id = event.sequenceNumber;
        msg->dest_port = event.ctrl_port;
        msg->dest_ipcp_id = event.ipcp_id;
        msg->src_ipcp_id = 0;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, false) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif
}

void ApplicationManager::applicationUnregistered(const ApplicationUnregistrationRequestEvent& event,
						 int result)
{
	LOG_DBG("ApplicationManager::applicationUnregistered called");

#if STUB_API
	//Do nothing
#else
        struct irati_msg_app_reg_app_resp * msg;

        msg = new irati_msg_app_reg_app_resp();
        msg->msg_type = RINA_C_APP_UNREGISTER_APPLICATION_RESPONSE;
        msg->app_name = event.applicationName.to_c_name();
        msg->dif_name = event.DIFName.to_c_name();
        msg->result = result;
        msg->event_id = event.sequenceNumber;
        msg->dest_port = event.ctrl_port;
        msg->dest_ipcp_id = event.ipcp_id;
        msg->src_ipcp_id = 0;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, false) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif
}

void ApplicationManager::flowAllocated(const FlowRequestEvent& flowRequestEvent)
{
	LOG_DBG("ApplicationManager::flowAllocated called");

#if STUB_API
	//Do nothing
#else
        struct irati_msg_app_alloc_flow_result * msg;

        msg = new irati_msg_app_alloc_flow_result();
        msg->msg_type = RINA_C_APP_ALLOCATE_FLOW_REQUEST_RESULT;
        msg->port_id = flowRequestEvent.portId;
        msg->source_app_name = flowRequestEvent.localApplicationName.to_c_name();
        msg->dif_name = flowRequestEvent.DIFName.to_c_name();
        msg->event_id = flowRequestEvent.sequenceNumber;
        msg->dest_port = flowRequestEvent.ctrl_port;
        msg->dest_ipcp_id = flowRequestEvent.ipcp_id;
        msg->src_ipcp_id = 0;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, false) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif
}

void ApplicationManager::flowRequestArrived(const ApplicationProcessNamingInformation& localAppName,
					    const ApplicationProcessNamingInformation& remoteAppName,
					    const FlowSpecification& flowSpec,
					    const ApplicationProcessNamingInformation& difName,
					    int portId,
					    unsigned int opaque, unsigned int ctrl_port)
{
#if STUB_API
#else
        struct irati_kmsg_ipcm_allocate_flow * msg;

        msg = new irati_kmsg_ipcm_allocate_flow();
        msg->msg_type = RINA_C_APP_ALLOCATE_FLOW_REQUEST_ARRIVED;
        msg->port_id = portId;
        msg->local = localAppName.to_c_name();
        msg->remote = remoteAppName.to_c_name();
        msg->fspec = flowSpec.to_c_flowspec();
        msg->dif_name = difName.to_c_name();
        msg->event_id = opaque;
        msg->dest_port = ctrl_port;
        msg->src_ipcp_id = 0;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, false) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif
}

void ApplicationManager::flowDeallocatedRemotely(int portId, int code,
						 unsigned int ctrl_port)
{
	LOG_DBG("ApplicationManager::flowDeallocatedRemotely called");
#if STUB_API
	//Do nothing
#else
        struct irati_msg_app_dealloc_flow * msg;

        msg = new irati_msg_app_dealloc_flow();
        msg->msg_type = RINA_C_APP_FLOW_DEALLOCATED_NOTIFICATION;
        msg->port_id = portId;
        msg->result = code;
        msg->dest_port = ctrl_port;
        msg->event_id = 0;
        msg->src_ipcp_id = 0;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, false) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif
}

void ApplicationManager::getDIFPropertiesResponse(const GetDIFPropertiesRequestEvent &event,
						  int result, const std::list<DIFProperties>& difProperties)
{
#if STUB_API
	//Do nothing
#else
        struct irati_msg_get_dif_prop * msg;
        struct dif_properties_entry * dpe;
        std::list<DIFProperties>::const_iterator it;

        msg = new irati_msg_get_dif_prop();
        msg->msg_type = RINA_C_APP_GET_DIF_PROPERTIES_RESPONSE;
        msg->code = result;
        msg->app_name = event.getApplicationName().to_c_name();
        msg->event_id = event.sequenceNumber;
        msg->dest_port = event.ctrl_port;
        msg->dest_ipcp_id = event.ipcp_id;
        msg->src_ipcp_id = 0;
        msg->dif_props = new get_dif_prop_resp();
        INIT_LIST_HEAD(&msg->dif_props->dif_propery_entries);
        for(it = difProperties.begin(); it != difProperties.end(); ++it) {
        	dpe = new dif_properties_entry();
        	INIT_LIST_HEAD(&dpe->next);
        	dpe->max_sdu_size = it->maxSDUSize;
        	dpe->dif_name = it->DIFName.to_c_name();
        	list_add_tail(&dpe->next, &msg->dif_props->dif_propery_entries);
        }

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, false) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif
}

Singleton<ApplicationManager> applicationManager;

/* CLASS GET DIF PROPERTIES REQUEST EVENT */
GetDIFPropertiesRequestEvent::GetDIFPropertiesRequestEvent(
		const ApplicationProcessNamingInformation& appName,
		const ApplicationProcessNamingInformation& DIFName,
		unsigned int sequenceNumber,
		unsigned int ctrl_port, unsigned short ipcp_id):
        IPCEvent(GET_DIF_PROPERTIES, sequenceNumber,
        		ctrl_port, ipcp_id)
{
	this->applicationName = appName;
	this->DIFName = DIFName;
}

const ApplicationProcessNamingInformation&
	GetDIFPropertiesRequestEvent::getApplicationName() const
{ return applicationName; }

const ApplicationProcessNamingInformation&
	GetDIFPropertiesRequestEvent::getDIFName() const
{ return DIFName; }

/* CLASS IPCM REGISTER APPLICATION RESPONSE EVENT */
IpcmRegisterApplicationResponseEvent::IpcmRegisterApplicationResponseEvent(
                int result, unsigned int sequenceNumber,
		unsigned int ctrl_port, unsigned short ipcp_id):
                        BaseResponseEvent(result,
                                        IPCM_REGISTER_APP_RESPONSE_EVENT,
                                        sequenceNumber, ctrl_port, ipcp_id)
{ }

/* CLASS IPCM UNREGISTER APPLICATION RESPONSE EVENT */
IpcmUnregisterApplicationResponseEvent::IpcmUnregisterApplicationResponseEvent(
                int result, unsigned int sequenceNumber,
		unsigned int ctrl_port, unsigned short ipcp_id):
                        BaseResponseEvent(result,
                                        IPCM_UNREGISTER_APP_RESPONSE_EVENT,
                                        sequenceNumber, ctrl_port, ipcp_id)
{ }

/* CLASS IPCM ALLOCATE FLOW REQUEST RESULT EVENT */
IpcmAllocateFlowRequestResultEvent::IpcmAllocateFlowRequestResultEvent(
                int result, int portId, unsigned int sequenceNumber,
		unsigned int ctrl_port, unsigned short ipcp_id):
                        BaseResponseEvent(result,
                                        IPCM_ALLOCATE_FLOW_REQUEST_RESULT,
                                        sequenceNumber, ctrl_port, ipcp_id)
{ this->portId = portId; }

int IpcmAllocateFlowRequestResultEvent::getPortId() const
{ return portId; }

/* CLASS QUERY RIB RESPONSE EVENT */
QueryRIBResponseEvent::QueryRIBResponseEvent(
                const std::list<rib::RIBObjectData>& ribObjects,
                int result,
                unsigned int sequenceNumber,
		unsigned int ctrl_port, unsigned short ipcp_id) :
        BaseResponseEvent(result,
                          QUERY_RIB_RESPONSE_EVENT,
                          sequenceNumber, ctrl_port, ipcp_id)
{ this->ribObjects = ribObjects; }

const std::list<rib::RIBObjectData>& QueryRIBResponseEvent::getRIBObject() const
{ return ribObjects; }

/* CLASS UPDATE DIF CONFIGURATION RESPONSE EVENT */
UpdateDIFConfigurationResponseEvent::UpdateDIFConfigurationResponseEvent(
                int result, unsigned int sequenceNumber,
		unsigned int ctrl_port, unsigned short ipcp_id):
                        BaseResponseEvent(result,
                                        UPDATE_DIF_CONFIG_RESPONSE_EVENT,
                                        sequenceNumber, ctrl_port, ipcp_id)
{ }

/* CLASS ENROLL TO DIF RESPONSE EVENT */
EnrollToDIFResponseEvent::EnrollToDIFResponseEvent(
                const std::list<Neighbor>& neighbors,
                const DIFInformation& difInformation,
                int result, unsigned int sequenceNumber,
		unsigned int ctrl_port, unsigned short ipcp_id):
                        BaseResponseEvent(result,
                                        ENROLL_TO_DIF_RESPONSE_EVENT,
                                        sequenceNumber,
					ctrl_port, ipcp_id)
{
        this->neighbors = neighbors;
        this->difInformation = difInformation;
}

const std::list<Neighbor>&
EnrollToDIFResponseEvent::getNeighbors() const
{ return neighbors; }

const DIFInformation&
EnrollToDIFResponseEvent::getDIFInformation() const
{ return difInformation; }

/* CLASS IPC PROCESS DAEMON INITIALIZED EVENT */
IPCProcessDaemonInitializedEvent::IPCProcessDaemonInitializedEvent(
                unsigned short ipcProcessId,
                const ApplicationProcessNamingInformation&  name,
                unsigned int sequenceNumber,
		unsigned int ctrl_port, unsigned short ipcp_id):
                        IPCEvent(IPC_PROCESS_DAEMON_INITIALIZED_EVENT,
                                        sequenceNumber, ctrl_port, ipcp_id)
{
        this->ipcProcessId = ipcProcessId;
        this->name = name;
}

unsigned short IPCProcessDaemonInitializedEvent::getIPCProcessId() const
{ return ipcProcessId; }

const ApplicationProcessNamingInformation&
IPCProcessDaemonInitializedEvent::getName() const
{ return name; }

/* CLASS TIMER EXPIRED EVENT */
TimerExpiredEvent::TimerExpiredEvent(unsigned int sequenceNumber,
		unsigned int ctrl_port, unsigned short ipcp_id) :
                IPCEvent(TIMER_EXPIRED_EVENT, sequenceNumber,
                		ctrl_port, ipcp_id)
{ }

/* Class Media Report Event */
MediaReportEvent::MediaReportEvent(const MediaReport& report,
		 	 	   unsigned int sequenceNumber,
				   unsigned int ctrl_port, unsigned short ipcp_id) :
			 IPCEvent(IPCM_MEDIA_REPORT_EVENT, sequenceNumber,
					 ctrl_port, ipcp_id)
{
	media_report = report;
}

CreateIPCPResponseEvent::CreateIPCPResponseEvent(int res,
						 unsigned int sequenceNumber,
						 unsigned int ctrl_port, unsigned short ipcp_id):
		IPCEvent(IPCM_CREATE_IPCP_RESPONSE,
			 sequenceNumber, ctrl_port, ipcp_id)
{
	result = res;
}

DestroyIPCPResponseEvent::DestroyIPCPResponseEvent(int res,
						   unsigned int sequenceNumber,
						   unsigned int ctrl_port, unsigned short ipcp_id):
		IPCEvent(IPCM_DESTROY_IPCP_RESPONSE,
			 sequenceNumber, ctrl_port, ipcp_id)
{
	result = res;
}

}

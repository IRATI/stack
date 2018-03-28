//
// IPC Process
//
//    Eduard Grasa          <eduard.grasa@i2cat.net>
//    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
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

#include <ostream>
#include <sstream>
#include <errno.h>

#define RINA_PREFIX "librina.ipc-process"

#include "librina/logs.h"
#include "librina/ipc-process.h"
#include "core.h"
#include "utils.h"
#include "ctrl.h"

namespace rina {

/* CLASS ASSIGN TO DIF REQUEST EVENT */
AssignToDIFRequestEvent::AssignToDIFRequestEvent(
		const DIFInformation& difInformation,
			unsigned int sequenceNumber,
			unsigned int ctrl_p, unsigned short ipcp_id):
		IPCEvent(ASSIGN_TO_DIF_REQUEST_EVENT, sequenceNumber,
				ctrl_p, ipcp_id)
{
	this->difInformation = difInformation;
}

        const DIFInformation&
        AssignToDIFRequestEvent::getDIFInformation() const
{ return difInformation; }

/* CLASS UPDATE DIF CONFIGURATION REQUEST EVENT */
const DIFConfiguration&
UpdateDIFConfigurationRequestEvent::getDIFConfiguration() const
{
        return difConfiguration;
}

UpdateDIFConfigurationRequestEvent::UpdateDIFConfigurationRequestEvent(
                const DIFConfiguration& difConfiguration,
                        unsigned int sequenceNumber,
			unsigned int ctrl_p, unsigned short ipcp_id):
                IPCEvent(UPDATE_DIF_CONFIG_REQUEST_EVENT, sequenceNumber,
                		ctrl_p, ipcp_id)
{
        this->difConfiguration = difConfiguration;
}

/* CLASS IPC PROCESS DIF REGISTRATION EVENT */
IPCProcessDIFRegistrationEvent::IPCProcessDIFRegistrationEvent(
		const ApplicationProcessNamingInformation& ipcProcessName,
		const ApplicationProcessNamingInformation& difName,
		bool registered,
		unsigned int sequenceNumber,
		unsigned int ctrl_p, unsigned short ipcp_id): IPCEvent(
		                IPC_PROCESS_DIF_REGISTRATION_NOTIFICATION,
		                sequenceNumber, ctrl_p, ipcp_id)
{
        this->ipcProcessName = ipcProcessName;
        this->difName = difName;
        this->registered = registered;
}

const ApplicationProcessNamingInformation&
IPCProcessDIFRegistrationEvent::getIPCProcessName() const{
	return ipcProcessName;
}

const ApplicationProcessNamingInformation&
IPCProcessDIFRegistrationEvent::getDIFName() const{
	return difName;
}

bool IPCProcessDIFRegistrationEvent::isRegistered() const {
        return registered;
}

/* CLASS QUERY RIB REQUEST EVENT */
QueryRIBRequestEvent::QueryRIBRequestEvent(const std::string& objectClass,
		const std::string& objectName, long objectInstance,
		int scope, const std::string& filter,
		unsigned int sequenceNumber,
		unsigned int ctrl_p, unsigned short ipcp_id):
				IPCEvent(IPC_PROCESS_QUERY_RIB,
                                         sequenceNumber, ctrl_p, ipcp_id)
{
	this->objectClass = objectClass;
	this->objectName = objectName;
	this->objectInstance = objectInstance;
	this->scope = scope;
	this->filter = filter;
}

const std::string& QueryRIBRequestEvent::getObjectClass() const{
	return objectClass;
}

const std::string& QueryRIBRequestEvent::getObjectName() const{
	return objectName;
}

long QueryRIBRequestEvent::getObjectInstance() const{
	return objectInstance;
}

int QueryRIBRequestEvent::getScope() const{
	return scope;
}

const std::string& QueryRIBRequestEvent::getFilter() const{
	return filter;
}

/* CLASS SET POLICY SET PARAM REQUEST EVENT */
SetPolicySetParamRequestEvent::SetPolicySetParamRequestEvent(
                const std::string& path, const std::string& name,
                const std::string& value, unsigned int sequenceNumber,
		unsigned int ctrl_p, unsigned short ipcp_id) :
				IPCEvent(IPC_PROCESS_SET_POLICY_SET_PARAM,
                                         sequenceNumber, ctrl_p, ipcp_id)
{
        this->path = path;
        this->name = name;
        this->value = value;
}

/* CLASS SELECT POLICY SET REQUEST EVENT */
SelectPolicySetRequestEvent::SelectPolicySetRequestEvent(
                const std::string& path, const std::string& name,
                                        unsigned int sequenceNumber,
					unsigned int ctrl_p, unsigned short ipcp_id) :
				IPCEvent(IPC_PROCESS_SELECT_POLICY_SET,
                                         sequenceNumber, ctrl_p, ipcp_id)
{
        this->path = path;
        this->name = name;
}

/* CLASS PLUGIN LOAD REQUEST EVENT */
PluginLoadRequestEvent::PluginLoadRequestEvent(const std::string& name,
                                bool load, unsigned int sequenceNumber,
				unsigned int ctrl_p, unsigned short ipcp_id) :
				IPCEvent(IPC_PROCESS_PLUGIN_LOAD,
                                         sequenceNumber, ctrl_p, ipcp_id)
{
        this->name = name;
        this->load = load;
}

/* CLASS CREATE CONNECTION RESPONSE EVENT */
CreateConnectionResponseEvent::CreateConnectionResponseEvent(int portId,
        int cepId, unsigned int sequenceNumber,
	unsigned int ctrl_p, unsigned short ipcp_id):
                IPCEvent(IPC_PROCESS_CREATE_CONNECTION_RESPONSE,
                                sequenceNumber, ctrl_p, ipcp_id) {
        this->cepId = cepId;
        this->portId = portId;
}

int CreateConnectionResponseEvent::getCepId() const {
        return cepId;
}

int CreateConnectionResponseEvent::getPortId() const {
        return portId;
}

/* CLASS UPDATE CONNECTION RESPONSE EVENT */
UpdateConnectionResponseEvent::UpdateConnectionResponseEvent(int portId,
        int result, unsigned int sequenceNumber,
	unsigned int ctrl_p, unsigned short ipcp_id):
                IPCEvent(IPC_PROCESS_UPDATE_CONNECTION_RESPONSE,
                                sequenceNumber, ctrl_p, ipcp_id) {
        this->result = result;
        this->portId = portId;
}

int UpdateConnectionResponseEvent::getResult() const {
        return result;
}

int UpdateConnectionResponseEvent::getPortId() const {
        return portId;
}

/* CLASS CREATE CONNECTION RESULT EVENT */
CreateConnectionResultEvent::CreateConnectionResultEvent(int portId,
        int sourceCepId, int destCepId, unsigned int sequenceNumber,
	unsigned int ctrl_p, unsigned short ipcp_id):
                IPCEvent(IPC_PROCESS_CREATE_CONNECTION_RESULT,
                                sequenceNumber, ctrl_p, ipcp_id) {
        this->sourceCepId = sourceCepId;
        this->destCepId = destCepId;
        this->portId = portId;
}

int CreateConnectionResultEvent::getSourceCepId() const {
        return sourceCepId;
}

int CreateConnectionResultEvent::getDestCepId() const {
        return destCepId;
}

int CreateConnectionResultEvent::getPortId() const {
        return portId;
}

/* CLASS DESTROY CONNECTION RESULT EVENT */
DestroyConnectionResultEvent::DestroyConnectionResultEvent(int portId,
        int result, unsigned int sequenceNumber,
	unsigned int ctrl_p, unsigned short ipcp_id):
                IPCEvent(IPC_PROCESS_DESTROY_CONNECTION_RESULT,
                                sequenceNumber, ctrl_p, ipcp_id) {
        this->result = result;
        this->portId = portId;
}

int DestroyConnectionResultEvent::getResult() const {
        return result;
}

int DestroyConnectionResultEvent::getPortId() const {
        return portId;
}

/* CLASS DUMP PDU FT RESULT EVENT*/
DumpFTResponseEvent::DumpFTResponseEvent(const std::list<PDUForwardingTableEntry>& entries,
                int result, unsigned int sequenceNumber,
		unsigned int ctrl_p, unsigned short ipcp_id):
                IPCEvent(IPC_PROCESS_DUMP_FT_RESPONSE,
                                sequenceNumber, ctrl_p, ipcp_id) {
        this->entries = entries;
        this->result = result;
}

const std::list<PDUForwardingTableEntry>&
DumpFTResponseEvent::getEntries() const {
        return entries;
}

int DumpFTResponseEvent::getResult() const {
        return result;
}

// Class enable encryption response event
UpdateCryptoStateResponseEvent::UpdateCryptoStateResponseEvent(int res,
                int port, unsigned int sequenceNumber,
		unsigned int ctrl_p, unsigned short ipcp_id) :
                		IPCEvent(IPC_PROCESS_UPDATE_CRYPTO_STATE_RESPONSE,
                				sequenceNumber, ctrl_p, ipcp_id)
{
	port_id = port;
	result = res;
}

AllocatePortResponseEvent::AllocatePortResponseEvent(int res,
						     int port,
						     unsigned int sequenceNumber,
						     unsigned int ctrl_p, unsigned short ipcp_id):
		IPCEvent(IPC_PROCESS_ALLOCATE_PORT_RESPONSE,
			 sequenceNumber, ctrl_p, ipcp_id)
{
	port_id = port;
	result = res;
}

DeallocatePortResponseEvent::DeallocatePortResponseEvent(int res,
						         int port,
							 unsigned int sequenceNumber,
							 unsigned int ctrl_p, unsigned short ipcp_id):
		IPCEvent(IPC_PROCESS_DEALLOCATE_PORT_RESPONSE,
			 sequenceNumber, ctrl_p, ipcp_id)
{
	port_id = port;
	result = res;
}

WriteMgmtSDUResponseEvent::WriteMgmtSDUResponseEvent(int res,
			  	  	  	     unsigned int sequenceNumber,
						     unsigned int ctrl_p, unsigned short ipcp_id):
		IPCEvent(IPC_PROCESS_WRITE_MGMT_SDU_RESPONSE,
			 sequenceNumber, ctrl_p, ipcp_id)
{
	result = res;
}

ReadMgmtSDUResponseEvent::ReadMgmtSDUResponseEvent(int res,
			 	 	 	   struct buffer * buf,
						   unsigned int pid,
						   unsigned int sequenceNumber,
						   unsigned int ctrl_p, unsigned short ipcp_id):
		IPCEvent(IPC_PROCESS_READ_MGMT_SDU_NOTIF,
			 sequenceNumber, ctrl_p, ipcp_id)
{
	result = res;
	port_id = pid;
	if (buf) {
		msg.size_ = buf->size;
		msg.message_ = new unsigned char[msg.size_];
		memcpy(msg.message_, buf->data, msg.size_);
	}
}

/* CLASS EXTENDED IPC MANAGER */
const std::string ExtendedIPCManager::error_allocate_flow =
		"Error allocating flow";

ExtendedIPCManager::ExtendedIPCManager() {
        ipcManagerPort = 0;
        ipcProcessId = 0;
        ipcProcessInitialized = false;
}

ExtendedIPCManager::~ExtendedIPCManager() throw()
{ }

const DIFInformation& ExtendedIPCManager::getCurrentDIFInformation() const{
	return currentDIFInformation;
}

void ExtendedIPCManager::setCurrentDIFInformation(
		const DIFInformation& currentDIFInformation)
{
	this->currentDIFInformation = currentDIFInformation;
}

unsigned short ExtendedIPCManager::getIpcProcessId() const
{ return ipcProcessId; }

void ExtendedIPCManager::setIpcProcessId(unsigned short ipcProcessId)
{ this->ipcProcessId = ipcProcessId; }

void ExtendedIPCManager::setIPCManagerPort(
                unsigned int ipcManagerPort) {
        this->ipcManagerPort = ipcManagerPort;
}

void ExtendedIPCManager::send_base_resp_msg(irati_msg_t msg_t,
					    unsigned int seq_num, int result)
{
#if STUB_API
	//Do nothing
#else
        struct irati_msg_base_resp * msg;

        msg = new irati_msg_base_resp();
        msg->msg_type = msg_t;
        msg->result = result;
        msg->event_id = seq_num;
        msg->src_ipcp_id = ipcProcessId;
        msg->dest_port = ipcManagerPort;
        msg->dest_ipcp_id = 0;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, false) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif
}

void ExtendedIPCManager::notifyIPCProcessInitialized(const ApplicationProcessNamingInformation& name)
{
	ScopedLock slock(lock);

        if (ipcProcessInitialized) {
                throw IPCException("IPC Process already initialized");
        }

#if STUB_API
        // Do nothing
#else
        struct irati_msg_with_name * msg;

        msg = new irati_msg_with_name();
        msg->msg_type = RINA_C_IPCM_IPC_PROCESS_INITIALIZED;
        msg->name = name.to_c_name();
        msg->src_ipcp_id = ipcProcessId;
        msg->dest_port = ipcManagerPort;
        msg->dest_ipcp_id = 0;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, false) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif
        ipcProcessInitialized = true;
}

bool ExtendedIPCManager::isIPCProcessInitialized() const
{
        return ipcProcessInitialized;
}

ApplicationRegistration *
ExtendedIPCManager::appRegistered(const ApplicationProcessNamingInformation& appName,
				  const ApplicationProcessNamingInformation& DIFName)
{
        ApplicationRegistration * applicationRegistration;

        ScopedLock slock(lock);

        applicationRegistration = getApplicationRegistration(
                        appName);

        if (!applicationRegistration) {
                applicationRegistration = new ApplicationRegistration(
                                appName);
                putApplicationRegistration(appName,
                                applicationRegistration);
        }

        applicationRegistration->addDIFName(DIFName);

        return applicationRegistration;
}

void ExtendedIPCManager::appUnregistered(const ApplicationProcessNamingInformation& appName,
                			 const ApplicationProcessNamingInformation& DIFName)
{
	ScopedLock slock(lock);

        ApplicationRegistration * applicationRegistration =
                        getApplicationRegistration(appName);
        if (!applicationRegistration) {
                throw ApplicationUnregistrationException(
                                IPCManager::application_not_registered_error.c_str());
        }

        std::list<ApplicationProcessNamingInformation>::const_iterator iterator;
        for (iterator = applicationRegistration->DIFNames.begin();
                        iterator != applicationRegistration->DIFNames.end();
                        ++iterator) {
                if (*iterator == DIFName) {
                        applicationRegistration->removeDIFName(DIFName);
                        if (applicationRegistration->DIFNames.size() == 0) {
                                removeApplicationRegistration(appName);
                        }

                        break;
                }
        }
}

void ExtendedIPCManager::assignToDIFResponse(const AssignToDIFRequestEvent& event, int result)
{
	if (result == 0) {
		this->currentDIFInformation = event.getDIFInformation();
	}

	send_base_resp_msg(RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE,
			   event.sequenceNumber, result);
}

void ExtendedIPCManager::enrollToDIFResponse(const EnrollToDAFRequestEvent& event,
                        		     int result, const std::list<Neighbor> & newNeighbors,
					     const DIFInformation& difInformation)
{
#if STUB_API
        // Do nothing
#else
        struct irati_msg_ipcm_enroll_to_dif_resp * msg;
        struct ipcp_neighbor_entry * nei;
        std::list<Neighbor>::const_iterator it;

        msg = new irati_msg_ipcm_enroll_to_dif_resp();
        msg->msg_type = RINA_C_IPCM_ENROLL_TO_DIF_RESPONSE;
        msg->result = result;
        msg->event_id = event.sequenceNumber;
        msg->src_ipcp_id = ipcProcessId;
        msg->dest_port = ipcManagerPort;
        msg->dest_ipcp_id = 0;
        msg->dif_type = stringToCharArray(difInformation.dif_type_);
        msg->dif_name = difInformation.dif_name_.to_c_name();
        msg->dif_config = difInformation.dif_configuration_.to_c_dif_config();
        msg->neighbors = new ipcp_neigh_list();
        INIT_LIST_HEAD(&msg->neighbors->ipcp_neighbors);
        for (it = newNeighbors.begin(); it != newNeighbors.end(); ++it) {
        	nei = new ipcp_neighbor_entry();
        	INIT_LIST_HEAD(&nei->next);
        	nei->entry = it->to_c_neighbor();
        	list_add_tail(&nei->next, &msg->neighbors->ipcp_neighbors);
        }

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, false) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif
}

void ExtendedIPCManager::disconnectNeighborResponse(const DisconnectNeighborRequestEvent& event,
			        		    int result)
{
	send_base_resp_msg(RINA_C_IPCM_DISCONNECT_FROM_NEIGHBOR_RESPONSE,
			   event.sequenceNumber, result);
}

void ExtendedIPCManager::registerApplicationResponse(const ApplicationRegistrationRequestEvent& event,
						     int result)
{
	send_base_resp_msg(RINA_C_IPCM_REGISTER_APPLICATION_RESPONSE,
			   event.sequenceNumber, result);
}

void ExtendedIPCManager::unregisterApplicationResponse(const ApplicationUnregistrationRequestEvent& event,
						       int result)
{
	send_base_resp_msg(RINA_C_IPCM_UNREGISTER_APPLICATION_RESPONSE,
			   event.sequenceNumber, result);
}

void ExtendedIPCManager::allocateFlowRequestResult(const FlowRequestEvent& event,
						   int result)
{
#if STUP_API
	//Do nothing
#else
        struct irati_kmsg_multi_msg * msg;

        msg = new irati_kmsg_multi_msg();
        msg->msg_type = RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_RESULT;
        msg->result = result;
        msg->port_id = event.portId;
        msg->event_id = event.sequenceNumber;
        msg->src_ipcp_id = ipcProcessId;
        msg->dest_port = ipcManagerPort;
        msg->dest_ipcp_id = 0;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, false) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        irati_ctrl_msg_free((struct irati_msg_base *) msg);

#endif
}

unsigned int
ExtendedIPCManager::allocateFlowRequestArrived(const ApplicationProcessNamingInformation& localAppName,
					       const ApplicationProcessNamingInformation& remoteAppName,
					       const FlowSpecification& flowSpecification,
					       int portId)
{
	unsigned int seq_num = 0;

#if STUP_API
	//Do nothing
#else
        struct irati_kmsg_ipcm_allocate_flow * msg;

        msg = new irati_kmsg_ipcm_allocate_flow();
        msg->msg_type = RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_ARRIVED;
        msg->local = localAppName.to_c_name();
        msg->remote = remoteAppName.to_c_name();
        msg->fspec = flowSpecification.to_c_flowspec();
        msg->dif_name = currentDIFInformation.get_dif_name().to_c_name();
        msg->src_ipcp_id = ipcProcessId;
        msg->dest_port = ipcManagerPort;
        msg->dest_ipcp_id = 0;
        msg->port_id = portId;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, true) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        seq_num = msg->event_id;
        irati_ctrl_msg_free((struct irati_msg_base *) msg);

#endif
        return seq_num;
}

unsigned int
ExtendedIPCManager::requestFlowAllocation(const ApplicationProcessNamingInformation& localAppName,
                			  const ApplicationProcessNamingInformation& remoteAppName,
					  const FlowSpecification& flowSpec)
{
        return internalRequestFlowAllocation(localAppName, remoteAppName,
        				     flowSpec, ipcProcessId);
}

unsigned int
ExtendedIPCManager::requestFlowAllocationInDIF(const ApplicationProcessNamingInformation& localAppName,
					       const ApplicationProcessNamingInformation& remoteAppName,
					       const ApplicationProcessNamingInformation& difName,
					       const FlowSpecification& flowSpec)
{
        return internalRequestFlowAllocationInDIF(localAppName, remoteAppName,
        					  difName, ipcProcessId, flowSpec);
}

FlowInformation ExtendedIPCManager::allocateFlowResponse(const FlowRequestEvent& flowRequestEvent,
							 int result,
							 bool notifySource)
{
        return internalAllocateFlowResponse(flowRequestEvent,
                                            result,
                                            notifySource,
                                            ipcProcessId);
}

void ExtendedIPCManager::flowDeallocatedRemotely(int portId, int code)
{
#if STUB_API
	// Do nothing
#else
        struct irati_kmsg_multi_msg * msg;

        msg = new irati_kmsg_multi_msg();
        msg->msg_type = RINA_C_IPCM_FLOW_DEALLOCATED_NOTIFICATION;
        msg->result = code;
        msg->port_id = portId;
        msg->src_ipcp_id = ipcProcessId;
        msg->dest_port = ipcManagerPort;
        msg->dest_ipcp_id = 0;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, false) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif
}

void ExtendedIPCManager::queryRIBResponse(const QueryRIBRequestEvent& event,
					  int result,
					  const std::list<rib::RIBObjectData>& ribObjects)
{
#if STUB_API
	//Do nothing
#else
        struct irati_kmsg_ipcm_query_rib_resp * msg;
        struct rib_object_data * rod;
        std::list<rib::RIBObjectData>::const_iterator it;

        msg = new irati_kmsg_ipcm_query_rib_resp();
        msg->msg_type = RINA_C_IPCM_QUERY_RIB_RESPONSE;
        msg->result = result;
        msg->event_id = event.sequenceNumber;
        msg->src_ipcp_id = ipcProcessId;
        msg->dest_port = ipcManagerPort;
        msg->dest_ipcp_id = 0;
        msg->rib_entries = new query_rib_resp();
        INIT_LIST_HEAD(&msg->rib_entries->rib_object_data_entries);
        for (it = ribObjects.begin(); it != ribObjects.end(); ++ it) {
        	rod = new rib_object_data();
        	INIT_LIST_HEAD(&rod->next);
        	rod->instance = it->instance_;
        	rod->name = stringToCharArray(it->name_);
        	rod->disp_value = stringToCharArray(it->displayable_value_);
        	rod->clazz = stringToCharArray(it->class_);
        	list_add_tail(&rod->next, &msg->rib_entries->rib_object_data_entries);
        }

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, false) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif
}

unsigned int ExtendedIPCManager::allocatePortId(const ApplicationProcessNamingInformation& appName,
						const FlowSpecification& fspec)
{
	unsigned int result = 0;

#if STUB_API
#else
        struct irati_kmsg_ipcp_allocate_port * msg;

        msg = new irati_kmsg_ipcp_allocate_port();
        msg->msg_type = RINA_C_IPCP_ALLOCATE_PORT_REQUEST;
        msg->app_name = appName.to_c_name();
        msg->msg_boundaries = fspec.msg_boundaries;
        msg->src_ipcp_id = ipcProcessId;
        msg->dest_ipcp_id = ipcProcessId;
        msg->dest_port = 0;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, true) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        result = msg->event_id;
        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif

        return result;
}

unsigned int ExtendedIPCManager::deallocatePortId(int portId)
{
	unsigned int result = 0;

#if STUB_API
#else
        struct irati_kmsg_multi_msg * msg;

        msg = new irati_kmsg_multi_msg();
        msg->msg_type = RINA_C_IPCP_DEALLOCATE_PORT_REQUEST;
        msg->port_id = portId;
        msg->src_ipcp_id = ipcProcessId;
        msg->dest_ipcp_id = ipcProcessId;
        msg->dest_port = 0;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, true) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        result = msg->event_id;
        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif

        return result;
}

void ExtendedIPCManager::setPolicySetParamResponse(const SetPolicySetParamRequestEvent& event,
						   int result)
{
	send_base_resp_msg(RINA_C_IPCP_SET_POLICY_SET_PARAM_RESPONSE,
			   event.sequenceNumber, result);
}

void ExtendedIPCManager::selectPolicySetResponse(const SelectPolicySetRequestEvent& event,
						 int result)
{
	send_base_resp_msg(RINA_C_IPCP_SELECT_POLICY_SET_RESPONSE,
			   event.sequenceNumber, result);
}

void ExtendedIPCManager::pluginLoadResponse(const PluginLoadRequestEvent& event,
					    int result)
{
	send_base_resp_msg(RINA_C_IPCM_PLUGIN_LOAD_RESPONSE,
			   event.sequenceNumber, result);
}

void ExtendedIPCManager::send_fwd_msg(irati_msg_t msg_t, unsigned int sequenceNumber,
		  	  	      const ser_obj_t& sermsg, int result)
{
#if STUB_API
        //Do nothing
#else
        struct irati_msg_ipcm_fwd_cdap_msg * msg;

        msg = new irati_msg_ipcm_fwd_cdap_msg();
        msg->msg_type = msg_t;
        msg->result = result;
        msg->event_id = sequenceNumber;
        msg->src_ipcp_id = ipcProcessId;
        msg->dest_port = ipcManagerPort;
        msg->dest_ipcp_id = 0;
        msg->cdap_msg = new buffer();
        msg->cdap_msg->size = sermsg.size_;
        msg->cdap_msg->data = new unsigned char[sermsg.size_];
        memcpy(msg->cdap_msg->data, sermsg.message_, sermsg.size_);

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, false) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif
}

void ExtendedIPCManager::forwardCDAPRequest(unsigned int sequenceNumber,
                                            const ser_obj_t& sermsg,
                                            int result)
{
	send_fwd_msg(RINA_C_IPCM_FWD_CDAP_MSG_REQUEST, sequenceNumber,
		     sermsg, result);
}

void ExtendedIPCManager::forwardCDAPResponse(unsigned int sequenceNumber,
					     const ser_obj_t& sermsg,
					     int result)
{
	send_fwd_msg(RINA_C_IPCM_FWD_CDAP_MSG_RESPONSE, sequenceNumber,
		     sermsg, result);
}

void ExtendedIPCManager::sendMediaReport(const MediaReport& report)
{
#if STUB_API
	//Do nothing
#else
        struct irati_msg_ipcm_media_report * msg;

        msg = new irati_msg_ipcm_media_report();
        msg->msg_type = RINA_C_IPCM_MEDIA_REPORT;
        msg->report = report.to_c_media_report();
        msg->src_ipcp_id = ipcProcessId;
        msg->dest_port = ipcManagerPort;
        msg->dest_ipcp_id = 0;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, false) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif
}

int ExtendedIPCManager::internal_flow_allocated(const rina::FlowInformation& flow_info)
{
	FlowInformation * flow;
	WriteScopedLock writeLock(flows_rw_lock);

	flow = new FlowInformation();
	flow->portId = flow_info.portId;
	flow->difName = flow_info.difName;
	flow->flowSpecification = flow_info.flowSpecification;
	flow->localAppName = flow_info.localAppName;
	flow->remoteAppName = flow_info.remoteAppName;
	flow->state = FlowInformation::FLOW_ALLOCATED;

	initIodev(flow, flow->portId);

	allocatedFlows[flow->portId] = flow;

	return flow->fd;
}

void ExtendedIPCManager::internal_flow_deallocated(int port_id)
{
        FlowInformation * flow;

        WriteScopedLock writeLock(flows_rw_lock);

        flow = getAllocatedFlow(port_id);
        if (flow == 0) {
                throw FlowDeallocationException(
                                IPCManager::unknown_flow_error);
        }

        close(flow->fd);
        allocatedFlows.erase(port_id);
        delete flow;
}

Singleton<ExtendedIPCManager> extendedIPCManager;

/* CLASS CONNECTION */
Connection::Connection() {
        portId = 0;
        sourceAddress = 0;
        destAddress = 0;
        qosId = 0;
        sourceCepId = 0;
        destCepId = 0;
        flowUserIpcProcessId = 0;
}

unsigned int Connection::getDestAddress() const {
        return destAddress;
}

void Connection::setDestAddress(unsigned int destAddress) {
        this->destAddress = destAddress;
}

int Connection::getPortId() const {
        return portId;
}

void Connection::setPortId(int portId) {
        this->portId = portId;
}

unsigned int Connection::getQosId() const {
        return qosId;
}

void Connection::setQosId(unsigned int qosId) {
        this->qosId = qosId;
}

unsigned int Connection::getSourceAddress() const {
        return sourceAddress;
}

void Connection::setSourceAddress(unsigned int sourceAddress) {
        this->sourceAddress = sourceAddress;
}

int Connection::getDestCepId() const {
        return destCepId;
}

void Connection::setDestCepId(int destCepId) {
        this->destCepId = destCepId;
}

unsigned short Connection::getFlowUserIpcProcessId() const {
        return flowUserIpcProcessId;
}

void Connection::setFlowUserIpcProcessId(unsigned short flowUserIpcProcessId) {
        this->flowUserIpcProcessId = flowUserIpcProcessId;
}

int Connection::getSourceCepId() const {
        return sourceCepId;
}

void Connection::setSourceCepId(int sourceCepId) {
        this->sourceCepId = sourceCepId;
}

const DTPConfig& Connection::getDTPConfig() const {
        return dtpConfig;
}

void Connection::setDTPConfig(const DTPConfig& dtpConfig) {
        this->dtpConfig = dtpConfig;
}

const DTCPConfig& Connection::getDTCPConfig() const {
        return dtcpConfig;
}

void Connection::setDTCPConfig(const DTCPConfig& dtcpConfig) {
        this->dtcpConfig = dtcpConfig;
}

const std::string Connection::toString() {
        std::stringstream ss;
        ss<<"Source address: "<<sourceAddress;
        ss<<"; Source cep-id: "<<sourceCepId;
        ss<<"; Dest address: "<<destAddress;
        ss<<"; Dest cep-id: "<<destCepId<<std::endl;
        ss<<"Por-id: "<<portId<<"; QoS-id: "<<qosId;
        ss<<"; Flow user IPC Process id: "<<flowUserIpcProcessId<<std::endl;
        ss<<"DTP Configuration: "<<dtpConfig.toString();
        ss<<"DTCP Configuration: "<<dtcpConfig.toString();
        return ss.str();
}

DTPInformation::DTPInformation()
{
	src_cep_id = 0;
	dest_cep_id = 0;
	src_address = 0;
	dest_address = 0;
	qos_id = 0;
	port_id = 0;
}

DTPInformation::DTPInformation(Connection * connection)
{
	src_cep_id = connection->sourceCepId;
	dest_cep_id = connection->destCepId;
	src_address = connection->sourceAddress;
	dest_address = connection->destAddress;
	qos_id = connection->qosId;
	port_id = connection->portId;
	dtp_config = connection->dtpConfig;
	stats = connection->stats;
}

const std::string DTPInformation::toString() const
{
	std::stringstream ss;
	ss << "CEP-ids: src = " << src_cep_id << ", dest = " << dest_cep_id
	   << "; Addresses: src = " << src_address << ", dest = " << dest_address
	   << "; Qos-id: " << qos_id << "; Port-id: " << port_id << std::endl;
	ss << " DTP config: " << dtp_config.toString();
	ss << " Tx: pdus = " << stats.tx_pdus << ", Bytes = " << stats.tx_bytes
	   << " RX: pdus = " << stats.rx_pdus << ", Bytes = " << stats.rx_bytes << std::endl;

	return ss.str();
}


std::string IPCPNameAddresses::get_addresses_as_string() const
{
	std::stringstream ss;

	for (std::list<unsigned int>::const_iterator it = addresses.begin();
			it != addresses.end(); ++it) {
		ss << *it << "; ";
	}

	return ss.str();
}

/* CLASS ROUTING TABLE ENTRE */
RoutingTableEntry::RoutingTableEntry(){
	cost = 1;
	qosId = 0;
}

const std::string RoutingTableEntry::getKey() const
{
	std::stringstream ss;
	if (destination.name != "") {
		ss << destination.name << "-" << qosId << "-" << cost << "-"
		   << nextHopNames.front().alts.front().name;
	} else {
		ss << destination.addresses.front() << "-" << qosId << "-" << cost << "-"
		   << nextHopNames.front().alts.front().addresses.front();
	}

	return ss.str();
}

PortIdAltlist::PortIdAltlist()
{
}

PortIdAltlist::PortIdAltlist(unsigned int nh)
{
	add_alt(nh);
}

void PortIdAltlist::from_c_pid_list(PortIdAltlist & pi,
			            struct port_id_altlist * pia)
{
	if (!pia)
		return;

	for (int i=0; i<pia->num_ports; i++) {
		pi.alts.push_back(pia->ports[i]);
	}
}

struct port_id_altlist * PortIdAltlist::to_c_pid_list() const
{
	struct port_id_altlist * result;
	std::list<unsigned int>::const_iterator it;
	int i = 0;

	result = new port_id_altlist();
	INIT_LIST_HEAD(&result->next);
	result->num_ports = alts.size();
	result->ports = new port_id_t[result->num_ports];
	for (it = alts.begin(); it != alts.end(); ++it) {
		result->ports[i] = (*it);
		i++;
	}

	return result;
}

void PortIdAltlist::add_alt(unsigned int nh)
{
	alts.push_back(nh);
}

/* CLASS PDU FORWARDING TABLE ENTRY */
PDUForwardingTableEntry::PDUForwardingTableEntry() {
        address = 0;
        qosId = 0;
        cost = 0;
}

void PDUForwardingTableEntry::from_c_pff_entry(PDUForwardingTableEntry & pf,
			     	     	       struct mod_pff_entry * pff)
{
	struct port_id_altlist * pos;

	pf.address = pff->fwd_info;
	pf.cost = pff->cost;
	pf.qosId = pff->qos_id;

	list_for_each_entry(pos, &(pff->port_id_altlists), next) {
		PortIdAltlist pia;
		PortIdAltlist::from_c_pid_list(pia, pos);
		pf.portIdAltlists.push_back(pia);
	}
}

struct mod_pff_entry * PDUForwardingTableEntry::to_c_pff_entry() const
{
	struct mod_pff_entry * result;
	std::list<PortIdAltlist>::const_iterator it;
	struct port_id_altlist * pia;

	result = new mod_pff_entry();
	INIT_LIST_HEAD(&result->next);
	INIT_LIST_HEAD(&result->port_id_altlists);
	result->qos_id = qosId;
	result->fwd_info = address;
	result->cost = cost;

	for(it = portIdAltlists.begin(); it != portIdAltlists.end(); ++it) {
		pia = it->to_c_pid_list();
		list_add_tail(&pia->next, &result->port_id_altlists);
	}

	return result;
}

bool PDUForwardingTableEntry::operator==(
                const PDUForwardingTableEntry &other) const {
        if (address != other.getAddress())
                return false;

        if (qosId != other.getQosId())
                return false;

        if (cost != other.cost)
        	return false;

        return true;
}

bool PDUForwardingTableEntry::operator!=(
                const PDUForwardingTableEntry &other) const {
        return !(*this == other);
}

unsigned int PDUForwardingTableEntry::getAddress() const {
        return address;
}

void PDUForwardingTableEntry::setAddress(unsigned int address) {
        this->address = address;
}

const std::list<PortIdAltlist> PDUForwardingTableEntry::getPortIdAltlists() const {
        return portIdAltlists;
}

void PDUForwardingTableEntry::
setPortIdAltlists(const std::list<PortIdAltlist>& portIdAltlists) {
        this->portIdAltlists = portIdAltlists;
}

unsigned int PDUForwardingTableEntry::getQosId() const {
        return qosId;
}

void PDUForwardingTableEntry::setQosId(unsigned int qosId) {
        this->qosId = qosId;
}

const std::string PDUForwardingTableEntry::toString() {
        std::stringstream ss;

        ss<<"Address: "<<address<<" QoS-id: "<<qosId<< " Cost: "<<cost;
        ss<<"List of N-1 port-ids: ";
        for (std::list<PortIdAltlist>::iterator it = portIdAltlists.begin();
                        it != portIdAltlists.end(); it++) {
		for (std::list<unsigned int>::iterator jt = it->alts.begin();
				jt != it->alts.end(); jt++) {
			ss<< *jt << ",";
		}
		ss << ";";
        }
        ss<<std::endl;

        return ss.str();
}

const std::string PDUForwardingTableEntry::getKey() const
{
	std::stringstream ss;
	ss << address << "-" << qosId << "-" << portIdAltlists.front().alts.front();

	return ss.str();
}


/* CLASS READ MANAGEMENT SDU RESULT */
ReadManagementSDUResult::ReadManagementSDUResult() {
        bytesRead = 0;
        portId = 0;
}

int ReadManagementSDUResult::getBytesRead() const {
        return bytesRead;
}

void ReadManagementSDUResult::setBytesRead(int bytesRead) {
        this->bytesRead = bytesRead;
}

int ReadManagementSDUResult::getPortId() const {
        return portId;
}

void ReadManagementSDUResult::setPortId(int portId) {
        this->portId = portId;
}

/* CLASS KERNEL IPC PROCESS */
void KernelIPCProcess::setIPCProcessId(unsigned short ipcProcessId) {
        this->ipcProcessId = ipcProcessId;
}

unsigned short KernelIPCProcess::getIPCProcessId() const {
        return ipcProcessId;
}

unsigned int KernelIPCProcess::assignToDIF(const DIFInformation& difInformation)
{
        unsigned int seqNum = 0;

#if STUB_API
        // Do nothing
#else
        struct irati_kmsg_ipcm_assign_to_dif * msg;

        msg = new irati_kmsg_ipcm_assign_to_dif();
        msg->msg_type = RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST;
        msg->dif_config = difInformation.dif_configuration_.to_c_dif_config();
        msg->dif_name = difInformation.dif_name_.to_c_name();
        msg->type = stringToCharArray(difInformation.dif_type_);
        msg->src_ipcp_id = ipcProcessId;
        msg->dest_ipcp_id = ipcProcessId;
        msg->dest_port = 0;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, true) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        seqNum = msg->event_id;
        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif
        return seqNum;
}

unsigned int
KernelIPCProcess::updateDIFConfiguration(const DIFConfiguration& difConfiguration)
{
        unsigned int seqNum=0;

#if STUB_API
        // Do nothing
#else
        struct irati_kmsg_ipcm_update_config * msg;

        msg = new irati_kmsg_ipcm_update_config();
        msg->msg_type = RINA_C_IPCM_UPDATE_DIF_CONFIG_REQUEST;
        msg->dif_config = difConfiguration.to_c_dif_config();
        msg->src_ipcp_id = ipcProcessId;
        msg->dest_ipcp_id = ipcProcessId;
        msg->dest_port = 0;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, true) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        seqNum = msg->event_id;
        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif
        return seqNum;
}

unsigned int KernelIPCProcess::createConnection(const Connection& connection)
{
        unsigned int seqNum=0;

#if STUB_API
        // Do nothing
#else
        struct irati_kmsg_ipcp_conn_create_arrived * msg;

        msg = new irati_kmsg_ipcp_conn_create_arrived();
        msg->msg_type = RINA_C_IPCP_CONN_CREATE_REQUEST;
        msg->dst_addr = connection.destAddress;
        msg->dst_cep = connection.destCepId;
        msg->dtcp_cfg = connection.dtcpConfig.to_c_dtcp_config();
        msg->dtp_cfg = connection.dtpConfig.to_c_dtp_config();
        msg->port_id = connection.portId;
        msg->qos_id = connection.qosId;
        msg->flow_user_ipc_process_id = connection.flowUserIpcProcessId;
        msg->src_addr = connection.sourceAddress;
        msg->src_ipcp_id = ipcProcessId;
        msg->dest_ipcp_id = ipcProcessId;
        msg->dest_port = 0;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, true) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        seqNum = msg->event_id;
        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif
        return seqNum;
}

unsigned int KernelIPCProcess::updateConnection(const Connection& connection)
{
        unsigned int seqNum=0;

#if STUB_API
        // Do nothing
#else
        struct irati_kmsg_ipcp_conn_update * msg;

        msg = new irati_kmsg_ipcp_conn_update();
        msg->msg_type = RINA_C_IPCP_CONN_UPDATE_REQUEST;
        msg->dst_cep = connection.destCepId;
        msg->port_id = connection.portId;
        msg->src_cep = connection.sourceCepId;
        msg->src_ipcp_id = ipcProcessId;
        msg->dest_ipcp_id = ipcProcessId;
        msg->dest_port = 0;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, true) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        seqNum = msg->event_id;
        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif
        return seqNum;
}

void KernelIPCProcess::modify_connection(const Connection& connection)
{
#if STUB_API
        // Do nothing
#else
        struct irati_kmsg_ipcp_conn_update * msg;

        msg = new irati_kmsg_ipcp_conn_update();
        msg->msg_type = RINA_C_IPCP_CONN_MODIFY_REQUEST;
        msg->src_addr = connection.sourceAddress;
        msg->dest_addr = connection.destAddress;
        msg->src_cep = connection.sourceCepId;
        msg->src_ipcp_id = ipcProcessId;
        msg->dest_ipcp_id = ipcProcessId;
        msg->dest_port = 0;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, true) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif
}

unsigned int
KernelIPCProcess::createConnectionArrived(const Connection& connection)
{
        unsigned int seqNum=0;

#if STUB_API
        // Do nothing
#else
        struct irati_kmsg_ipcp_conn_create_arrived * msg;

        msg = new irati_kmsg_ipcp_conn_create_arrived();
        msg->msg_type = RINA_C_IPCP_CONN_CREATE_ARRIVED;
        msg->dst_addr = connection.destAddress;
        msg->dst_cep = connection.destCepId;
        msg->dtcp_cfg = connection.dtcpConfig.to_c_dtcp_config();
        msg->dtp_cfg = connection.dtpConfig.to_c_dtp_config();
        msg->port_id = connection.portId;
        msg->qos_id = connection.qosId;
        msg->src_cep = connection.sourceCepId;
        msg->flow_user_ipc_process_id = connection.flowUserIpcProcessId;
        msg->src_addr = connection.sourceAddress;
        msg->src_ipcp_id = ipcProcessId;
        msg->dest_ipcp_id = ipcProcessId;
        msg->dest_port = 0;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, true) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        seqNum = msg->event_id;
        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif
        return seqNum;
}

unsigned int
KernelIPCProcess::destroyConnection(const Connection& connection)
{
        unsigned int seqNum = 0;

#if STUB_API
        //Do nothing
#else
        struct irati_kmsg_multi_msg * msg;

        msg = new irati_kmsg_multi_msg();
        msg->msg_type = RINA_C_IPCP_CONN_DESTROY_REQUEST;
        msg->port_id = connection.portId;
        msg->cep_id = connection.sourceCepId;
        msg->src_ipcp_id = ipcProcessId;
        msg->dest_ipcp_id = ipcProcessId;
        msg->dest_port = 0;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, true) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        seqNum = msg->event_id;
        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif
        return seqNum;
}

void KernelIPCProcess::modifyPDUForwardingTableEntries(const std::list<PDUForwardingTableEntry *>& entries,
                        			       int mode)
{
#if STUB_API
        //Do nothing
#else
        struct irati_kmsg_rmt_dump_ft * msg;
        std::list<PDUForwardingTableEntry *>::const_iterator it;
        struct mod_pff_entry * entry;

        msg = new irati_kmsg_rmt_dump_ft();
        msg->msg_type = RINA_C_RMT_MODIFY_FTE_REQUEST;
        msg->mode = mode;
        msg->src_ipcp_id = ipcProcessId;
        msg->dest_ipcp_id = ipcProcessId;
        msg->dest_port = 0;
        msg->pft_entries = new pff_entry_list();
        INIT_LIST_HEAD(&msg->pft_entries->pff_entries);
        for (it = entries.begin(); it != entries.end(); ++it) {
        	entry = (*it)->to_c_pff_entry();
        	list_add_tail(&entry->next, &msg->pft_entries->pff_entries);
        }

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, true) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif
}

unsigned int KernelIPCProcess::dumptPDUFT()
{
        unsigned int seqNum=0;

#if STUB_API
        //Do nothing
#else
        struct irati_msg_base * msg;

        msg = new irati_msg_base();
        msg->msg_type = RINA_C_RMT_DUMP_FT_REQUEST;
        msg->src_ipcp_id = ipcProcessId;
        msg->dest_ipcp_id = ipcProcessId;
        msg->dest_port = 0;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, true) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        seqNum = msg->event_id;
        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif

        return seqNum;
}

unsigned int KernelIPCProcess::updateCryptoState(const CryptoState& state)
{
        unsigned int seqNum=0;

#if STUB_API
        //Do nothing
#else
        struct irati_kmsg_ipcp_update_crypto_state * msg;

        msg = new irati_kmsg_ipcp_update_crypto_state();
        msg->msg_type = RINA_C_IPCP_UPDATE_CRYPTO_STATE_REQUEST;
        msg->state = state.to_c_crypto_state();
        msg->port_id = state.port_id;
        msg->src_ipcp_id = ipcProcessId;
        msg->dest_ipcp_id = ipcProcessId;
        msg->dest_port = 0;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, true) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        seqNum = msg->event_id;
        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif

        return seqNum;
}

unsigned int KernelIPCProcess::changeAddress(unsigned int new_address,
					     unsigned int old_address,
				             unsigned int use_new_t,
				             unsigned int deprecate_old_t)
{
	unsigned int seqNum=0;

#if STUB_API
	//Do nothing
#else
        struct irati_kmsg_ipcp_address_change * msg;

        msg = new irati_kmsg_ipcp_address_change();
        msg->msg_type = RINA_C_IPCP_ADDRESS_CHANGE_REQUEST;
        msg->new_address = new_address;
        msg->old_address = old_address;
        msg->use_new_timeout = use_new_t;
        msg->deprecate_old_timeout = deprecate_old_t;
        msg->src_ipcp_id = ipcProcessId;
        msg->dest_ipcp_id = ipcProcessId;
        msg->dest_port = 0;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, true) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        seqNum = msg->event_id;
        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif

	return seqNum;
}

unsigned int KernelIPCProcess::setPolicySetParam(const std::string& path,
                                		 const std::string& name,
						 const std::string& value)
{
        unsigned int seqNum=0;

#if STUB_API
        //Do nothing
#else
        struct irati_kmsg_ipcp_select_ps_param * msg;

        msg = new irati_kmsg_ipcp_select_ps_param();
        msg->msg_type = RINA_C_IPCP_SET_POLICY_SET_PARAM_REQUEST;
        msg->path = stringToCharArray(path);
        msg->name = stringToCharArray(name);
        msg->value = stringToCharArray(value);
        msg->src_ipcp_id = ipcProcessId;
        msg->dest_ipcp_id = ipcProcessId;
        msg->dest_port = 0;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, true) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        seqNum = msg->event_id;
        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif

        return seqNum;
}

unsigned int KernelIPCProcess::selectPolicySet(const std::string& path,
                                	       const std::string& name)
{
        unsigned int seqNum=0;

#if STUB_API
        //Do nothing
#else
        struct irati_kmsg_ipcp_select_ps * msg;

        msg = new irati_kmsg_ipcp_select_ps();
        msg->msg_type = RINA_C_IPCP_SELECT_POLICY_SET_REQUEST;
        msg->path = stringToCharArray(path);
        msg->name = stringToCharArray(name);
        msg->src_ipcp_id = ipcProcessId;
        msg->dest_ipcp_id = ipcProcessId;
        msg->dest_port = 0;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, true) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        seqNum = msg->event_id;
        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif

        return seqNum;
}

unsigned int KernelIPCProcess::writeMgmgtSDUToPortId(void * sdu,
						     int size,
						     unsigned int portId)
{
	unsigned int seqNum=0;

#if STUB_API
	//Do nothing
#else
        struct irati_kmsg_ipcp_mgmt_sdu * msg;

        msg = new irati_kmsg_ipcp_mgmt_sdu();
        msg->msg_type = RINA_C_IPCP_MANAGEMENT_SDU_WRITE_REQUEST;
        msg->sdu = new buffer();
        msg->sdu->size = size;
        msg->sdu->data = new unsigned char[size];
        memcpy(msg->sdu->data, sdu, size);
        msg->port_id = portId;
        msg->src_ipcp_id = ipcProcessId;
        msg->dest_ipcp_id = ipcProcessId;
        msg->dest_port = 0;

        if (irati_ctrl_mgr->send_msg((struct irati_msg_base *) msg, true) != 0) {
        	irati_ctrl_msg_free((struct irati_msg_base *) msg);
        	throw IPCException("Problems sending CTRL message");
        }

        seqNum = msg->event_id;
        irati_ctrl_msg_free((struct irati_msg_base *) msg);
#endif

	return seqNum;
}

Singleton<KernelIPCProcess> kernelIPCProcess;

// CLASS DirectoryForwardingTableEntry
DirectoryForwardingTableEntry::DirectoryForwardingTableEntry() {
	address_ = 0;
	seqnum_ = 0;
}

ApplicationProcessNamingInformation DirectoryForwardingTableEntry::get_ap_naming_info() const {
	return ap_naming_info_;
}

void DirectoryForwardingTableEntry::set_ap_naming_info(
		const ApplicationProcessNamingInformation &ap_naming_info) {
	ap_naming_info_ = ap_naming_info;
}

unsigned int DirectoryForwardingTableEntry::get_address() const {
	return address_;
}

void DirectoryForwardingTableEntry::set_address(unsigned int address) {
	address_ = address;
}

long DirectoryForwardingTableEntry::get_seqnum() const {
	return seqnum_;
}

void DirectoryForwardingTableEntry::set_seqnum(unsigned int seqnum) {
	seqnum_ = seqnum;
}

const std::string DirectoryForwardingTableEntry::getKey() const{
	return ap_naming_info_.getEncodedString();
}

bool DirectoryForwardingTableEntry::operator==(const DirectoryForwardingTableEntry &object) {
	if (object.get_ap_naming_info() != ap_naming_info_) {
		return false;
	}

	if (object.get_address() != address_) {
		return false;
	}
	return true;
}

std::string DirectoryForwardingTableEntry::toString() {
    std::stringstream ss;
    ss << this->get_ap_naming_info().toString() << std::endl;
	ss << "IPC Process address: " << address_ << std::endl;
	ss << "Sequence number: " << seqnum_ << std::endl;

	return ss.str();
}

//	CLASS WhatevercastName
bool WhatevercastName::operator==(const WhatevercastName &other) {
	if (name_ == other.name_) {
		return true;
	}
	return false;
}

std::string WhatevercastName::toString() {
	std::string result = "Name: " + name_ + "\n";
	result = result + "Rule: " + rule_;
	return result;
}

ScanMediaRequestEvent::ScanMediaRequestEvent(unsigned int sequenceNumber,
					     unsigned int ctrl_p,
					     unsigned short ipcp_id) :
		IPCEvent(IPCP_SCAN_MEDIA_REQUEST_EVENT, sequenceNumber, ctrl_p, ipcp_id)
{}

}

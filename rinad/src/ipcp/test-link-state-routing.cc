//
// test-link-state-routing
//
//    Bernat Gaston <bernat.gaston@i2cat.net>
//    Eduard Grasa  <eduard.grasa@i2cat.net>
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

#define RINA_PREFIX "lsr-tests"

#include <librina/logs.h>

#include "ipcp/link-state-routing-ps.h"

class FakeEncoder: public rina::EncoderInterface {
public:
	const rina::SerializedObject* encode(const void* object) {
		if (!object) {
			return 0;
		}
		rina::SerializedObject * result;
		result = new rina::SerializedObject(0, 0);
		return result;
	}

	void* decode(const rina::ObjectValueInterface * object_value) const {
		LOG_DBG("%p", object_value);
		return 0;
	}
};

class FakeRIBDaemon: public rinad::IPCPRIBDaemon {
public:
	void subscribeToEvent(const rinad::IPCProcessEventType& eventId,
			rinad::EventListener * eventListener) {
		if (!eventListener) {
			return;
		}
		LOG_DBG("%d", eventId);
	}
	void unsubscribeFromEvent(const rinad::IPCProcessEventType& eventId,
			rinad::EventListener * eventListener) {
		if (!eventListener) {
			return;
		}
		LOG_DBG("%d", eventId);
	}
	void deliverEvent(rinad::Event * event) {
		if (!event) {
			return;
		}
	}
	void set_ipc_process(rinad::IPCProcess * ipc_process){
		ipc_process_ = ipc_process;
	}
	void set_dif_configuration(const rina::DIFConfiguration& dif_configuration) {
		LOG_DBG("DIF Configuration set: %u", dif_configuration.address_);
	}
	void addRIBObject(rina::BaseRIBObject * ribObject){
		if (!ribObject) {
			return;
		}
	}
	void removeRIBObject(rina::BaseRIBObject * ribObject) {
		if (!ribObject) {
			return;
		}
	}
	void removeRIBObject(const std::string& objectName){
		LOG_DBG("Removing object with name %s", objectName.c_str());
	}
        void sendMessageSpecific(bool useAddress, const rina::CDAPMessage & cdapMessage, int sessionId,
                        unsigned int address, rina::ICDAPResponseMessageHandler * cdapMessageHandler) {
                (void) useAddress;
                (void) cdapMessage;
                (void) sessionId;
                (void) address;
                (void) cdapMessageHandler;
        }
	void sendMessages(const std::list<const rina::CDAPMessage*>& cdapMessages,
				const rina::IUpdateStrategy& updateStrategy){
		LOG_DBG("%d, %p", cdapMessages.size(), &updateStrategy);
	}
	void sendMessage(const rina::CDAPMessage & cdapMessage,
	            int sessionId, rina::ICDAPResponseMessageHandler * cdapMessageHandler){
		if (!cdapMessageHandler)  {
			return;
		}
		LOG_DBG("%d, %d ", cdapMessage.op_code_, sessionId);
	}
	void sendMessageToAddress(const rina::CDAPMessage & cdapMessage,
	            int sessionId, unsigned int address,
	            rina::ICDAPResponseMessageHandler * cdapMessageHandler){
		if (!cdapMessageHandler) {
			return;
		}
		LOG_DBG("%d, %d, %ud", cdapMessage.op_code_, sessionId, address);
	}
	void cdapMessageDelivered(char* message, int length, int portId){
		if (!message) {
			return;
		}

		LOG_DBG("Message delivered: %d, %d", length, portId);
	}
	void createObject(const std::string& objectClass, const std::string& objectName,
	             const void* objectValue, const rina::NotificationPolicy * notificationPolicy){
		operationCalled(objectClass, objectName, objectValue);
		if (!notificationPolicy) {
			return;
		}
	}
	void deleteObject(const std::string& objectClass, const std::string& objectName,
	             const void* objectValue, const rina::NotificationPolicy * notificationPolicy) {
		operationCalled(objectClass, objectName, objectValue);
		if (!notificationPolicy) {
			return;
		}
	}
	rina::BaseRIBObject * readObject(const std::string& objectClass,
				const std::string& objectName){
		operationCalled(objectClass, objectName, 0);
		return 0;
	}
	void writeObject(const std::string& objectClass,
	                                 const std::string& objectName,
	                                 const void* objectValue){
		operationCalled(objectClass, objectName, objectValue);
	}
	void startObject(const std::string& objectClass,
	                                 const std::string& objectName,
	                                 const void* objectValue){
		operationCalled(objectClass, objectName, objectValue);
	}
	void stopObject(const std::string& objectClass,
	                                const std::string& objectName,
	                                const void* objectValue){
		operationCalled(objectClass, objectName, objectValue);
	}
	void processQueryRIBRequestEvent(const rina::QueryRIBRequestEvent& event){
		LOG_DBG("Event: %d", event.eventType);
	}
	std::list<rina::BaseRIBObject *> getRIBObjects() {
		std::list<rina::BaseRIBObject *> result;
		return result;
	}
	void openApplicationConnection(rina::CDAPMessage::AuthTypes auth_mech,
			const rina::AuthValue &auth_value, const std::string &dest_ae_inst,
			const std::string &dest_ae_name, const std::string &dest_ap_inst,
			const std::string &dest_ap_name, const std::string &src_ae_inst,
			const std::string &src_ae_name, const std::string &src_ap_inst,
			const std::string &src_ap_name, const rina::RemoteProcessId& remote_id) {
		(void) auth_mech;
		(void) auth_value;
		(void) dest_ae_inst;
		(void) dest_ae_name;
		(void) dest_ap_inst;
		(void) dest_ap_name;
		(void) src_ae_inst;
		(void) src_ae_name;
		(void) src_ap_inst;
		(void) src_ap_name;
		(void) remote_id;
	}
	void closeApplicationConnection(const rina::RemoteProcessId& remote_id,
				rina::ICDAPResponseMessageHandler * response_handler) {
		(void) remote_id;
		(void) response_handler;
	}
	void remoteCreateObject(const std::string& object_class, const std::string& object_name,
				rina::RIBObjectValue& object_value, int scope, const rina::RemoteProcessId& remote_id,
				rina::ICDAPResponseMessageHandler * response_handler) {
		(void) object_class;
		(void) object_name;
		(void) object_value;
		(void) scope;
		(void) remote_id;
		(void) response_handler;
	}
	void remoteDeleteObject(const std::string& object_class, const std::string& object_name,
				int scope, const rina::RemoteProcessId& remote_id,
				rina::ICDAPResponseMessageHandler * response_handler) {
		(void) object_class;
		(void) object_name;
		(void) scope;
		(void) remote_id;
		(void) response_handler;
	}
	void remoteReadObject(const std::string& object_class, const std::string& object_name,
				int scope, const rina::RemoteProcessId& remote_id,
				rina::ICDAPResponseMessageHandler * response_handler) {
		(void) object_class;
		(void) object_name;
		(void) scope;
		(void) remote_id;
		(void) response_handler;
	}
	void remoteWriteObject(const std::string& object_class, const std::string& object_name,
				rina::RIBObjectValue& object_value, int scope, const rina::RemoteProcessId& remote_id,
				rina::ICDAPResponseMessageHandler * response_handler) {
		(void) object_class;
		(void) object_name;
		(void) object_value;
		(void) scope;
		(void) remote_id;
		(void) response_handler;
	}
	void remoteStartObject(const std::string& object_class, const std::string& object_name,
				rina::RIBObjectValue& object_value, int scope, const rina::RemoteProcessId& remote_id,
				rina::ICDAPResponseMessageHandler * response_handler) {
		(void) object_class;
		(void) object_name;
		(void) object_value;
		(void) scope;
		(void) remote_id;
		(void) response_handler;
	}
	void remoteStopObject(const std::string& object_class, const std::string& object_name,
				rina::RIBObjectValue& object_value, int scope, const rina::RemoteProcessId& remote_id,
				rina::ICDAPResponseMessageHandler * response_handler) {
		(void) object_class;
		(void) object_name;
		(void) object_value;
		(void) scope;
		(void) remote_id;
		(void) response_handler;
	}
	void openApplicationConnectionResponse(rina::CDAPMessage::AuthTypes auth_mech,
				const rina::AuthValue &auth_value, const std::string &dest_ae_inst,
				const std::string &dest_ae_name, const std::string &dest_ap_inst, const std::string &dest_ap_name,
				int result, const std::string &result_reason, const std::string &src_ae_inst,
				const std::string &src_ae_name, const std::string &src_ap_inst, const std::string &src_ap_name,
				int invoke_id, const rina::RemoteProcessId& remote_id) {
		(void) auth_mech;
		(void) auth_value;
		(void) dest_ae_inst;
		(void) dest_ae_name;
		(void) dest_ap_inst;
		(void) dest_ap_name;
		(void) src_ae_inst;
		(void) src_ae_name;
		(void) src_ap_inst;
		(void) src_ap_name;
		(void) remote_id;
		(void) result;
		(void) result_reason;
		(void) invoke_id;
	}
	void closeApplicationConnectionResponse(int result, const std::string result_reason,
				int invoke_id, const rina::RemoteProcessId& remote_id) {
		(void) invoke_id;
		(void) result;
		(void) result_reason;
		(void) remote_id;
	}
	void remoteCreateObjectResponse(const std::string& object_class, const std::string& object_name,
				rina::RIBObjectValue& object_value, int result, const std::string result_reason, int invoke_id,
				const rina::RemoteProcessId& remote_id) {
		(void) invoke_id;
		(void) object_class;
		(void) object_name;
		(void) object_value;
		(void) result;
		(void) result_reason;
		(void) remote_id;
	}
	void remoteDeleteObjectResponse(const std::string& object_class, const std::string& object_name,
			int result, const std::string result_reason, int invoke_id,
			const rina::RemoteProcessId& remote_id) {
		(void) invoke_id;
		(void) object_class;
		(void) object_name;
		(void) result;
		(void) result_reason;
		(void) remote_id;
	}
	void remoteReadObjectResponse(const std::string& object_class, const std::string& object_name,
				rina::RIBObjectValue& object_value, int result, const std::string result_reason,
				bool read_incomplete, int invoke_id, const rina::RemoteProcessId& remote_id) {
		(void) invoke_id;
		(void) object_class;
		(void) object_name;
		(void) object_value;
		(void) result;
		(void) result_reason;
		(void) remote_id;
		(void) read_incomplete;
	}
	void remoteWriteObjectResponse(const std::string& object_class, const std::string& object_name,
			int result, const std::string result_reason, int invoke_id,
			const rina::RemoteProcessId& remote_id) {
		(void) invoke_id;
		(void) object_class;
		(void) object_name;
		(void) result;
		(void) result_reason;
		(void) remote_id;
	}
	void remoteStartObjectResponse(const std::string& object_class, const std::string& object_name,
				rina::RIBObjectValue& object_value, int result, const std::string result_reason, int invoke_id,
				const rina::RemoteProcessId& remote_id) {
		(void) invoke_id;
		(void) object_class;
		(void) object_name;
		(void) object_value;
		(void) result;
		(void) result_reason;
		(void) remote_id;
	}
	void remoteStopObjectResponse(const std::string& object_class, const std::string& object_name,
				rina::RIBObjectValue& object_value, int result, const std::string result_reason, int invoke_id,
				const rina::RemoteProcessId& remote_id) {
		(void) invoke_id;
		(void) object_class;
		(void) object_name;
		(void) object_value;
		(void) result;
		(void) result_reason;
		(void) remote_id;
	}

private:
	void operationCalled(const std::string& objectClass,
	                                 const std::string& objectName,
	                                 const void* objectValue) {
		if (!objectValue){
			return;
		}
		LOG_DBG("operation called, %s, %s", objectClass.c_str(),
				objectName.c_str());
	}
	rinad::IPCProcess * ipc_process_;
};

class FakeNamespaceManager: public rinad::INamespaceManager {
public:
	void set_ipc_process(rinad::IPCProcess * ipc_process){
		ipc_process_ = ipc_process;
	}
	void set_dif_configuration(const rina::DIFConfiguration& dif_configuration) {
		LOG_DBG("DIF Configuration set: %u", dif_configuration.address_);
	}
	unsigned int getDFTNextHop(const rina::ApplicationProcessNamingInformation& apNamingInfo) {
		(void) apNamingInfo;
		return 0;
	}
	unsigned short getRegIPCProcessId(const rina::ApplicationProcessNamingInformation& apNamingInfo) {
		(void) apNamingInfo;
		return 0;
	}
	void addDFTEntry(rina::DirectoryForwardingTableEntry * entry){
		(void) entry;
	}
	rina::DirectoryForwardingTableEntry * getDFTEntry(
				const rina::ApplicationProcessNamingInformation& apNamingInfo){
		(void) apNamingInfo;
		return 0;
	}
	void removeDFTEntry(const rina::ApplicationProcessNamingInformation& apNamingInfo){
		(void) apNamingInfo;
	}
	void processApplicationRegistrationRequestEvent(
				const rina::ApplicationRegistrationRequestEvent& event){
		(void) event;
	}
	void processApplicationUnregistrationRequestEvent(
				const rina::ApplicationUnregistrationRequestEvent& event){
		(void) event;
	}
	bool isValidAddress(unsigned int address, const std::string& ipcp_name,
				const std::string& ipcp_instance){
		(void) address;
		(void) ipcp_name;
		(void) ipcp_instance;
		return true;
	}
	unsigned int getValidAddress(const std::string& ipcp_name,
					const std::string& ipcp_instance) {
		(void) ipcp_name;
		(void) ipcp_instance;
		return 0;
	}
	unsigned int getAdressByname(const rina::ApplicationProcessNamingInformation& name) {
		if (name.processName.compare("") == 0) {
			throw Exception();
		}
		return 0;
	}

private:
	rinad::IPCProcess * ipc_process_;

};

class FakeIPCProcess: public rinad::IPCProcess {
public:
	FakeIPCProcess() {
		encoder_ = new rinad::Encoder();
		encoder_->addEncoder(rinad::EncoderConstants::FLOW_STATE_OBJECT_RIB_OBJECT_CLASS,
				new FakeEncoder());
		rib_daemon_ = new FakeRIBDaemon();
		state_= rinad::INITIALIZED;
		address_ = 0;
		timeout_ = 2000;
		cdap_session_manager_ = cdap_manager_factory_.createCDAPSessionManager(&wire_factory_,
				timeout_);
		namespace_manager_ = new FakeNamespaceManager();
	}
	~FakeIPCProcess(){
		delete encoder_;
		delete rib_daemon_;
		delete cdap_session_manager_;
		delete namespace_manager_;
	}
	unsigned short get_id() {
		return 0;
	}
	unsigned int get_address() const{
		return address_;
	}
	void set_address(unsigned int address){
		address_ = address;
	}
	const rinad::IPCProcessOperationalState& get_operational_state() const {
		return state_;
	}
	void set_operational_state(const rinad::IPCProcessOperationalState& operational_state){
		state_ = operational_state;
	}
	rina::DIFInformation& get_dif_information() const {
		throw Exception();
	}
	void set_dif_information(const rina::DIFInformation& dif_information) {
		dif_information_ = dif_information;
	}
	const std::list<rina::Neighbor *> get_neighbors() const {
		return neighbors_;
	}

    std::vector<rinad::PsFactory>::iterator
                    psFactoryLookup(const std::string& component,
                                   const std::string& name) {
    	std::vector<rinad::PsFactory>::iterator response;
    	(void) component;
    	(void) name;
    	return response;
    }

    int psFactoryPublish(const rinad::PsFactory& factory) {
    	(void) factory;
    	return 0;
    }

    int psFactoryUnpublish(const std::string& component,
                                          const std::string& name){
    	(void) component;
    	(void) name;
    	return 0;
    }

    rinad::IPolicySet * psCreate(const std::string& component,
                                 const std::string& name,
                                 rinad::IPCProcessComponent * context) {
    	(void) component;
    	(void) name;
    	(void) context;
    	return 0;
    }

    int psDestroy(const std::string& component,
                                        const std::string& name,
                                        rinad::IPolicySet * instance) {
    	(void) component;
    	(void) name;
    	(void) instance;
    	return 0;
    }

	rinad::IPCProcessOperationalState state_;
	rina::DIFInformation dif_information_;
	std::list<rina::Neighbor*> neighbors_;
	unsigned int address_;
	rina::WireMessageProviderFactory wire_factory_;
	rina::CDAPSessionManagerFactory cdap_manager_factory_;
	long timeout_;
};

int addObjectToGroup_NoObjectCheckModified_False() {
	rinad::FlowStateDatabase fsdb = rinad::FlowStateDatabase(0,0,0,0,0);
	if (fsdb.modified_ == true) {
		return -1;
	}

	return 0;
}

int addObjectToGroup_AddObjectCheckModified_True() {
	FakeIPCProcess ipcProcess;
	rinad::FlowStateRIBObjectGroup group = rinad::FlowStateRIBObjectGroup(&ipcProcess, 0);
	rinad::FlowStateDatabase fsdb = rinad::FlowStateDatabase(ipcProcess.encoder_,
			&group,0,ipcProcess.rib_daemon_,0);


	fsdb.addObjectToGroup(1, 1, 1, 1);
	if (fsdb.modified_ == false) {
		return -1;
	}

	return 0;
}

int incrementAge_AddObjectCheckModified_False() {
	FakeIPCProcess ipcProcess;
	rinad::FlowStateRIBObjectGroup group = rinad::FlowStateRIBObjectGroup(&ipcProcess, 0);
	unsigned int max_age = 5;
	rinad::FlowStateDatabase fsdb = rinad::FlowStateDatabase(ipcProcess.encoder_,
			&group,0, ipcProcess.rib_daemon_, &max_age);

	fsdb.addObjectToGroup(1, 1, 1, 1);
	fsdb.modified_ = false;
	fsdb.incrementAge();

	if (fsdb.modified_ == true) {
		return -1;
	}

	return 0;
}

int test_flow_state_object_db () {
	int result = 0;

	result = addObjectToGroup_NoObjectCheckModified_False();
	if (result < 0) {
		LOG_ERR("addObjectToGroup_NoObjectCheckModified_False test failed");
		return result;
	}
	LOG_INFO("addObjectToGroup_NoObjectCheckModified_False test passed");

	result = addObjectToGroup_AddObjectCheckModified_True();
	if (result < 0) {
		LOG_ERR("addObjectToGroup_AddObjectCheckModified_True test failed");
		return result;
	}
	LOG_INFO("addObjectToGroup_AddObjectCheckModified_True test passed");

	result = incrementAge_AddObjectCheckModified_False();
	if (result < 0) {
		LOG_ERR("addObjectToGroup_AddObjectCheckModified_True test failed");
		return result;
	}
	LOG_INFO("incrementAge_AddObjectCheckModified_False test passed");

	return result;
}

int Graph_EmptyGraph_Empty() {
	std::list<rinad::FlowStateObject *> objects;
	rinad::Graph g = rinad::Graph(objects);

	if (g.vertices_.size() != 0) {
		return -1;
	}

	if (g.edges_.size() != 0) {
		return -1;
	}

	return 0;
}

int Graph_Contruct2Nodes_True() {
	std::list<rinad::FlowStateObject *> objects;
	rinad::FlowStateObject fso1 = rinad::FlowStateObject(1, 2, 1, true, 1, 1);
	rinad::FlowStateObject fso2 = rinad::FlowStateObject(2, 1, 1, true, 1, 1);
	objects.push_back(&fso1);
	objects.push_back(&fso2);
    rinad::Graph g = rinad::Graph(objects);

    if (g.vertices_.size() != 2) {
    	return -1;
    }

    if (g.edges_.size() != 1) {
    	return -1;
    }

    std::list<rinad::Edge*>::const_iterator it;
    for (it = g.edges_.begin(); it != g.edges_.end(); ++it) {
    	if (!(*it)->isVertexIn(1) || !(*it)->isVertexIn(2)) {
    		return -1;
    	}
    }

    return 0;
}

int Graph_StateFalseisEmpty_True() {
	std::list<rinad::FlowStateObject *> objects;
	rinad::FlowStateObject fso1 = rinad::FlowStateObject(1, 2, 1, true, 1, 1);
	rinad::FlowStateObject fso2 = rinad::FlowStateObject(2, 1, 1, false, 1, 1);
	objects.push_back(&fso1);
	objects.push_back(&fso2);
	rinad::Graph g = rinad::Graph(objects);

	if (g.edges_.size() != 0) {
		return -1;
	}

	return 0;
}

int Graph_NotConnected_True() {
	std::list<rinad::FlowStateObject *> objects;
	rinad::FlowStateObject fso1 = rinad::FlowStateObject(1, 2, 1, true, 1, 1);
	rinad::FlowStateObject fso2 = rinad::FlowStateObject(2, 3, 1, false, 1, 1);
	rinad::FlowStateObject fso3 = rinad::FlowStateObject(3, 2, 1, false, 1, 1);
	objects.push_back(&fso1);
	objects.push_back(&fso2);
	rinad::Graph g = rinad::Graph(objects);

	if (g.vertices_.size() != 3) {
		return -1;
	}

	if (g.edges_.size() != 0) {
		return -1;
	}

	return 0;
}

int Graph_ContructNoBiderectionalFlow_False() {
	std::list<rinad::FlowStateObject *> objects;
	rinad::FlowStateObject fso1 = rinad::FlowStateObject(1, 2, 1, true, 1, 1);
	objects.push_back(&fso1);
	rinad::Graph g = rinad::Graph(objects);

	if (g.vertices_.size() != 2) {
		return -1;
	}

	if (g.edges_.size() != 0) {
		return -1;
	}

	return 0;
}

int Graph_ContructTriangleGraph_True() {
	std::list<rinad::FlowStateObject *> objects;
	rinad::FlowStateObject fso1 = rinad::FlowStateObject(1, 2, 1, true, 1, 1);
	rinad::FlowStateObject fso2 = rinad::FlowStateObject(2, 1, 1, true, 1, 1);
	rinad::FlowStateObject fso3 = rinad::FlowStateObject(1, 3, 1, true, 1, 1);
	rinad::FlowStateObject fso4 = rinad::FlowStateObject(3, 1, 1, true, 1, 1);
	rinad::FlowStateObject fso5 = rinad::FlowStateObject(2, 3, 1, true, 1, 1);
	rinad::FlowStateObject fso6 = rinad::FlowStateObject(3, 2, 1, true, 1, 1);
	objects.push_back(&fso1);
	objects.push_back(&fso2);
	objects.push_back(&fso3);
	objects.push_back(&fso4);
	objects.push_back(&fso5);
	objects.push_back(&fso6);
	rinad::Graph g = rinad::Graph(objects);

	if (g.vertices_.size() != 3) {
		return -1;
	}

	if (g.edges_.size() != 3) {
		return -1;
	}

	return 0;
}

int test_graph () {
	int result = 0;

	result = Graph_EmptyGraph_Empty();
	if (result < 0) {
		LOG_ERR("Graph_EmptyGraph_Empty test failed");
		return result;
	}
	LOG_INFO("Graph_EmptyGraph_Empty test passed");

	result = Graph_Contruct2Nodes_True();
	if (result < 0) {
		LOG_ERR("Graph_Contruct2Nodes_True test failed");
		return result;
	}
	LOG_INFO("Graph_Contruct2Nodes_True test passed");

	result = Graph_StateFalseisEmpty_True();
	if (result < 0) {
		LOG_ERR("Graph_StateFalseisEmpty_True test failed");
		return result;
	}
	LOG_INFO("Graph_StateFalseisEmpty_True test passed");

	result = Graph_NotConnected_True();
	if (result < 0) {
		LOG_ERR("Graph_NotConnected_True test failed");
		return result;
	}
	LOG_INFO("Graph_NotConnected_True test passed");

	result = Graph_ContructNoBiderectionalFlow_False();
	if (result < 0) {
		LOG_ERR("Graph_ContructNoBiderectionalFlow_False test failed");
		return result;
	}
	LOG_INFO("Graph_ContructNoBiderectionalFlow_False test passed");

	result = Graph_ContructTriangleGraph_True();
	if (result < 0) {
		LOG_ERR("Graph_ContructTriangleGraph_True test failed");
		return result;
	}
	LOG_INFO("Graph_ContructTriangleGraph_True test passed");

	return result;
}

int getRoutingTable_NoFSO_size0() {
	int result = 0;

	rinad::IRoutingAlgorithm * routingAlgorithm =
			new rinad::DijkstraAlgorithm();
	std::list<rinad::FlowStateObject *> fsos;

	std::list<rinad::RoutingTableEntry *> rtable =
			routingAlgorithm->computeRoutingTable(fsos, 1);

	if (rtable.size() != 0) {
		result = -1;
	}

	delete routingAlgorithm;
	return result;
}

int getRoutingTable_LinearGraphNumberOfEntries_2() {
	int result = 0;

	std::list<rinad::FlowStateObject *> objects;
	rinad::FlowStateObject fso1 = rinad::FlowStateObject(1, 2, 1, true, 1, 1);
	rinad::FlowStateObject fso2 = rinad::FlowStateObject(2, 1, 1, true, 1, 1);
	rinad::FlowStateObject fso3 = rinad::FlowStateObject(2, 3, 1, true, 1, 1);
	rinad::FlowStateObject fso4 = rinad::FlowStateObject(3, 2, 1, true, 1, 1);
	objects.push_back(&fso1);
	objects.push_back(&fso2);
	objects.push_back(&fso3);
	objects.push_back(&fso4);
	rinad::IRoutingAlgorithm * routingAlgorithm =
			new rinad::DijkstraAlgorithm();

	std::list<rinad::RoutingTableEntry *> rtable =
			routingAlgorithm->computeRoutingTable(objects, 1);

	if (rtable.size() != 2) {
		result = -1;
	}

	delete routingAlgorithm;
	return result;
}

int getRoutingTable_StateFalseNoEntries_True() {
	int result = 0;

	std::list<rinad::FlowStateObject *> objects;
	rinad::FlowStateObject fso1 = rinad::FlowStateObject(1, 2, 1, true, 1, 1);
	rinad::FlowStateObject fso2 = rinad::FlowStateObject(2, 1, 1, false, 1, 1);
	objects.push_back(&fso1);
	objects.push_back(&fso2);
	rinad::IRoutingAlgorithm * routingAlgorithm =
			new rinad::DijkstraAlgorithm();

	std::list<rinad::RoutingTableEntry *> rtable =
			routingAlgorithm->computeRoutingTable(objects, 1);

	if (rtable.size() != 0) {
		result = -1;
	}

	delete routingAlgorithm;
	return result;
}

int getRoutingTable_MultiGraphEntries_True() {
	int result = 0;

	std::list<rinad::FlowStateObject *> objects;
	rinad::FlowStateObject fso1 = rinad::FlowStateObject(1, 2, 1, true, 1, 1);
	rinad::FlowStateObject fso2 = rinad::FlowStateObject(2, 1, 1, true, 1, 1);
	rinad::FlowStateObject fso3 = rinad::FlowStateObject(1, 3, 1, true, 1, 1);
	rinad::FlowStateObject fso4 = rinad::FlowStateObject(3, 1, 1, true, 1, 1);
	rinad::FlowStateObject fso5 = rinad::FlowStateObject(2, 3, 1, true, 1, 1);
	rinad::FlowStateObject fso6 = rinad::FlowStateObject(3, 2, 1, true, 1, 1);
	objects.push_back(&fso1);
	objects.push_back(&fso2);
	objects.push_back(&fso3);
	objects.push_back(&fso4);
	objects.push_back(&fso5);
	objects.push_back(&fso6);
	rinad::IRoutingAlgorithm * routingAlgorithm =
			new rinad::DijkstraAlgorithm();

	std::list<rinad::RoutingTableEntry *> rtable =
			routingAlgorithm->computeRoutingTable(objects, 1);

	if (rtable.size() != 2) {
		result = -1;
	}

	delete routingAlgorithm;
	return result;
}

int test_dijkstra() {
	int result = 0;

	result = getRoutingTable_NoFSO_size0();
	if (result < 0) {
		LOG_ERR("getPDUTForwardingTable_NoFSO_size0 test failed");
		return result;
	}
	LOG_INFO("getPDUTForwardingTable_NoFSO_size0 test passed");

	result = getRoutingTable_LinearGraphNumberOfEntries_2();
	if (result < 0) {
		LOG_ERR("getPDUTForwardingTable_LinearGraphNumberOfEntries_2 test failed");
		return result;
	}
	LOG_INFO("getPDUTForwardingTable_LinearGraphNumberOfEntries_2 test passed");

	result = getRoutingTable_StateFalseNoEntries_True();
	if (result < 0) {
		LOG_ERR("getPDUTForwardingTable_StateFalseNoEntries_True test failed");
		return result;
	}
	LOG_INFO("getPDUTForwardingTable_StateFalseNoEntries_True test passed");

	result = getRoutingTable_MultiGraphEntries_True();
	if (result < 0) {
		LOG_ERR("getPDUTForwardingTable_MultiGraphEntries_True test failed");
		return result;
	}
	LOG_INFO("getPDUTForwardingTable_MultiGraphEntries_True test passed");

	return result;
}

int main()
{
	int result = 0;

	result = test_flow_state_object_db();
	if (result < 0) {
		LOG_ERR("test_flow_state_object_db tests failed");
		return result;
	}
	LOG_INFO("test_flow_state_object_db tests passed");

	result = test_graph();
	if (result < 0) {
		LOG_ERR("test_graph tests failed");
		return result;
	}
	LOG_INFO("test_graph tests passed");

	result = test_dijkstra();
	if (result < 0) {
		LOG_ERR("test_dijkstra tests failed");
		return result;
	}
	LOG_INFO("test_dijkstra tests passed");

	return 0;
}

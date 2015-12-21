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

#include <iostream>

#define IPCP_MODULE "lsr-tests"
#include "../../ipcp-logging.h"

#include "routing-ps.h"

int ipcp_id = 1;

/*
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
		LOG_IPCP_DBG("%p", object_value);
		return 0;
	}
};

class FakeRIBDaemon: public rinad::IPCPRIBDaemon {
public:
	void set_application_process(rina::ApplicationProcess * ap) {
		ipc_process_ = dynamic_cast<rinad::IPCProcess *>(ap);
	}
	void set_dif_configuration(const rina::DIFConfiguration& dif_configuration) {
		LOG_IPCP_DBG("DIF Configuration set: %u", dif_configuration.address_);
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
		LOG_IPCP_DBG("Removing object with name %s", objectName.c_str());
	}
        void sendMessageSpecific(bool useAddress, const rina::CDAPMessage & cdapMessage, int sessionId,
                        unsigned int address, rina::ICDAPResponseMessageHandler * cdapMessageHandler) {
        }
	void sendMessages(const std::list<const rina::CDAPMessage*>& cdapMessages,
				const rina::IUpdateStrategy& updateStrategy){
		LOG_IPCP_DBG("%d, %p", cdapMessages.size(), &updateStrategy);
	}
	void sendMessage(const rina::CDAPMessage & cdapMessage,
	            int sessionId, rina::ICDAPResponseMessageHandler * cdapMessageHandler){
		if (!cdapMessageHandler)  {
			return;
		}
		LOG_IPCP_DBG("%d, %d ", cdapMessage.op_code_, sessionId);
	}
	void sendMessageToAddress(const rina::CDAPMessage & cdapMessage,
	            int sessionId, unsigned int address,
	            rina::ICDAPResponseMessageHandler * cdapMessageHandler){
		if (!cdapMessageHandler) {
			return;
		}
		LOG_IPCP_DBG("%d, %d, %ud", cdapMessage.op_code_, sessionId, address);
	}
	void cdapMessageDelivered(char* message, int length, int portId){
		if (!message) {
			return;
		}

		LOG_IPCP_DBG("Message delivered: %d, %d", length, portId);
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
		LOG_IPCP_DBG("Event: %d", event.eventType);
	}
	std::list<rina::BaseRIBObject *> getRIBObjects() {
		std::list<rina::BaseRIBObject *> result;
		return result;
	}
	void openApplicationConnection(
			const rina::AuthPolicy &policy, const std::string &dest_ae_inst,
			const std::string &dest_ae_name, const std::string &dest_ap_inst,
			const std::string &dest_ap_name, const std::string &src_ae_inst,
			const std::string &src_ae_name, const std::string &src_ap_inst,
			const std::string &src_ap_name, const rina::RemoteProcessId& remote_id) {
	}
	void closeApplicationConnection(const rina::RemoteProcessId& remote_id,
				rina::ICDAPResponseMessageHandler * response_handler) {
	}
	void remoteCreateObject(const std::string& object_class, const std::string& object_name,
				rina::RIBObjectValue& object_value, int scope, const rina::RemoteProcessId& remote_id,
				rina::ICDAPResponseMessageHandler * response_handler) {
	}
	void remoteDeleteObject(const std::string& object_class, const std::string& object_name,
				int scope, const rina::RemoteProcessId& remote_id,
				rina::ICDAPResponseMessageHandler * response_handler) {
	}
	void remoteReadObject(const std::string& object_class, const std::string& object_name,
				int scope, const rina::RemoteProcessId& remote_id,
				rina::ICDAPResponseMessageHandler * response_handler) {
	}
	void remoteWriteObject(const std::string& object_class, const std::string& object_name,
				rina::RIBObjectValue& object_value, int scope, const rina::RemoteProcessId& remote_id,
				rina::ICDAPResponseMessageHandler * response_handler) {
	}
	void remoteStartObject(const std::string& object_class, const std::string& object_name,
				rina::RIBObjectValue& object_value, int scope, const rina::RemoteProcessId& remote_id,
				rina::ICDAPResponseMessageHandler * response_handler) {
	}
	void remoteStopObject(const std::string& object_class, const std::string& object_name,
				rina::RIBObjectValue& object_value, int scope, const rina::RemoteProcessId& remote_id,
				rina::ICDAPResponseMessageHandler * response_handler) {
	}
	void openApplicationConnectionResponse(
				const rina::AuthPolicy &policy, const std::string &dest_ae_inst,
				const std::string &dest_ae_name, const std::string &dest_ap_inst, const std::string &dest_ap_name,
				int result, const std::string &result_reason, const std::string &src_ae_inst,
				const std::string &src_ae_name, const std::string &src_ap_inst, const std::string &src_ap_name,
				int invoke_id, const rina::RemoteProcessId& remote_id) {
	}
	void closeApplicationConnectionResponse(int result, const std::string result_reason,
				int invoke_id, const rina::RemoteProcessId& remote_id) {
	}
	void remoteCreateObjectResponse(const std::string& object_class, const std::string& object_name,
				rina::RIBObjectValue& object_value, int result, const std::string result_reason, int invoke_id,
				const rina::RemoteProcessId& remote_id) {
	}
	void remoteDeleteObjectResponse(const std::string& object_class, const std::string& object_name,
			int result, const std::string result_reason, int invoke_id,
			const rina::RemoteProcessId& remote_id) {
	}
	void remoteReadObjectResponse(const std::string& object_class, const std::string& object_name,
				rina::RIBObjectValue& object_value, int result, const std::string result_reason,
				bool read_incomplete, int invoke_id, const rina::RemoteProcessId& remote_id) {
	}
	void remoteWriteObjectResponse(const std::string& object_class, const std::string& object_name,
			int result, const std::string result_reason, int invoke_id,
			const rina::RemoteProcessId& remote_id) {
	}
	void remoteStartObjectResponse(const std::string& object_class, const std::string& object_name,
				rina::RIBObjectValue& object_value, int result, const std::string result_reason, int invoke_id,
				const rina::RemoteProcessId& remote_id) {
	}
	void remoteStopObjectResponse(const std::string& object_class, const std::string& object_name,
				rina::RIBObjectValue& object_value, int result, const std::string result_reason, int invoke_id,
				const rina::RemoteProcessId& remote_id) {
	}

	void generateCDAPResponse(int invoke_id,
				  rina::CDAPSessionDescriptor * cdapSessDescr,
				  rina::CDAPMessage::Opcode opcode,
				  const std::string& obj_class,
				  const std::string& obj_name,
				  rina::RIBObjectValue& robject_value) {
	};

private:
	void operationCalled(const std::string& objectClass,
	                                 const std::string& objectName,
	                                 const void* objectValue) {
		if (!objectValue){
			return;
		}
		LOG_IPCP_DBG("operation called, %s, %s", objectClass.c_str(),
				objectName.c_str());
	}
	rinad::IPCProcess * ipc_process_;
};

class FakeNamespaceManager: public rinad::INamespaceManager {
public:
	void set_application_process(rina::ApplicationProcess * ap) {
		ipc_process_ = dynamic_cast<rinad::IPCProcess *>(ap);
	}
	void set_dif_configuration(const rina::DIFConfiguration& dif_configuration) {
		LOG_IPCP_DBG("DIF Configuration set: %u", dif_configuration.address_);
	}
	unsigned int getDFTNextHop(const rina::ApplicationProcessNamingInformation& apNamingInfo) {
		return 0;
	}
	unsigned short getRegIPCProcessId(const rina::ApplicationProcessNamingInformation& apNamingInfo) {
		return 0;
	}
	void addDFTEntry(rina::DirectoryForwardingTableEntry * entry){
	}
	rina::DirectoryForwardingTableEntry * getDFTEntry(
				const rina::ApplicationProcessNamingInformation& apNamingInfo){
		return 0;
	}
	void removeDFTEntry(const rina::ApplicationProcessNamingInformation& apNamingInfo){
	}
	void processApplicationRegistrationRequestEvent(
				const rina::ApplicationRegistrationRequestEvent& event){
	}
	void processApplicationUnregistrationRequestEvent(
				const rina::ApplicationUnregistrationRequestEvent& event){
	}
	bool isValidAddress(unsigned int address, const std::string& ipcp_name,
				const std::string& ipcp_instance){
		return true;
	}
	unsigned int getValidAddress(const std::string& ipcp_name,
					const std::string& ipcp_instance) {
		return 0;
	}
	unsigned int getAdressByname(const rina::ApplicationProcessNamingInformation& name) {
		if (name.processName.compare("") == 0) {
			throw rina::Exception();
		}
		return 0;
	}

	 rina::ApplicationRegistrationInformation
		get_reg_app_info(const rina::ApplicationProcessNamingInformation name) {
		return rina::ApplicationRegistrationInformation();
	}

private:
	rinad::IPCProcess * ipc_process_;

};

class FakeIPCProcess: public rinad::IPCProcess {
public:
	FakeIPCProcess() : IPCProcess("test", "1") {
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
		throw rina::Exception();
	}
	void set_dif_information(const rina::DIFInformation& dif_information) {
		dif_information_ = dif_information;
	}
	const std::list<rina::Neighbor *> get_neighbors() const {
		return neighbors_;
	}

    rina::IPolicySet * psCreate(const std::string& component,
                                 const std::string& name,
                                 rina::ApplicationEntity * context) {
    	return 0;
    }

    int psDestroy(const std::string& component,
                                        const std::string& name,
                                        rina::IPolicySet * instance) {
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
		LOG_IPCP_ERR("addObjectToGroup_NoObjectCheckModified_False test failed");
		return result;
	}
	LOG_IPCP_INFO("addObjectToGroup_NoObjectCheckModified_False test passed");

	result = addObjectToGroup_AddObjectCheckModified_True();
	if (result < 0) {
		LOG_IPCP_ERR("addObjectToGroup_AddObjectCheckModified_True test failed");
		return result;
	}
	LOG_IPCP_INFO("addObjectToGroup_AddObjectCheckModified_True test passed");

	result = incrementAge_AddObjectCheckModified_False();
	if (result < 0) {
		LOG_IPCP_ERR("addObjectToGroup_AddObjectCheckModified_True test failed");
		return result;
	}
	LOG_IPCP_INFO("incrementAge_AddObjectCheckModified_False test passed");

	return result;
}*/

int Graph_EmptyGraph_Empty() {
	std::list<rinad::FlowStateObject> objects;
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
	std::list<rinad::FlowStateObject> objects;
	rinad::FlowStateObject fso1(1, 2, 1, true, 1, 1);
	rinad::FlowStateObject fso2(2, 1, 1, true, 1, 1);
	objects.push_back(fso1);
	objects.push_back(fso2);
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
	std::list<rinad::FlowStateObject> objects;
	rinad::FlowStateObject fso1(1, 2, 1, true, 1, 1);
	rinad::FlowStateObject fso2(2, 1, 1, false, 1, 1);
	objects.push_back(fso1);
	objects.push_back(fso2);
	rinad::Graph g = rinad::Graph(objects);

	if (g.edges_.size() != 0) {
		return -1;
	}

	return 0;
}

int Graph_NotConnected_True() {
	std::list<rinad::FlowStateObject> objects;
	rinad::FlowStateObject fso1(1, 2, 1, true, 1, 1);
	rinad::FlowStateObject fso2(2, 3, 1, false, 1, 1);
	rinad::FlowStateObject fso3(3, 2, 1, false, 1, 1);
	objects.push_back(fso1);
	objects.push_back(fso2);
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
	std::list<rinad::FlowStateObject> objects;
	rinad::FlowStateObject fso1(1, 2, 1, true, 1, 1);
	objects.push_back(fso1);
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
	std::list<rinad::FlowStateObject> objects;
	rinad::FlowStateObject fso1(1, 2, 1, true, 1, 1);
	rinad::FlowStateObject fso2(2, 1, 1, true, 1, 1);
	rinad::FlowStateObject fso3(1, 3, 1, true, 1, 1);
	rinad::FlowStateObject fso4(3, 1, 1, true, 1, 1);
	rinad::FlowStateObject fso5(2, 3, 1, true, 1, 1);
	rinad::FlowStateObject fso6(3, 2, 1, true, 1, 1);
	objects.push_back(fso1);
	objects.push_back(fso2);
	objects.push_back(fso3);
	objects.push_back(fso4);
	objects.push_back(fso5);
	objects.push_back(fso6);
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
		LOG_IPCP_ERR("Graph_EmptyGraph_Empty test failed");
		return result;
	}
	LOG_IPCP_INFO("Graph_EmptyGraph_Empty test passed");

	result = Graph_Contruct2Nodes_True();
	if (result < 0) {
		LOG_IPCP_ERR("Graph_Contruct2Nodes_True test failed");
		return result;
	}
	LOG_IPCP_INFO("Graph_Contruct2Nodes_True test passed");

	result = Graph_StateFalseisEmpty_True();
	if (result < 0) {
		LOG_IPCP_ERR("Graph_StateFalseisEmpty_True test failed");
		return result;
	}
	LOG_IPCP_INFO("Graph_StateFalseisEmpty_True test passed");

	result = Graph_NotConnected_True();
	if (result < 0) {
		LOG_IPCP_ERR("Graph_NotConnected_True test failed");
		return result;
	}
	LOG_IPCP_INFO("Graph_NotConnected_True test passed");

	result = Graph_ContructNoBiderectionalFlow_False();
	if (result < 0) {
		LOG_IPCP_ERR("Graph_ContructNoBiderectionalFlow_False test failed");
		return result;
	}
	LOG_IPCP_INFO("Graph_ContructNoBiderectionalFlow_False test passed");

	result = Graph_ContructTriangleGraph_True();
	if (result < 0) {
		LOG_IPCP_ERR("Graph_ContructTriangleGraph_True test failed");
		return result;
	}
	LOG_IPCP_INFO("Graph_ContructTriangleGraph_True test passed");

	return result;
}

int getRoutingTable_NoFSO_size0() {
	std::list<rinad::FlowStateObject> fsos;
	std::list<rina::RoutingTableEntry *> rtable;
	rinad::IRoutingAlgorithm * routingAlgorithm;
	int result = 0;

	routingAlgorithm = new rinad::DijkstraAlgorithm();

	rtable = routingAlgorithm->computeRoutingTable(rinad::Graph(fsos),
						       fsos, 1);
	if (rtable.size() != 0) {
		result = -1;
	}

	delete routingAlgorithm;

	return result;
}

int getRoutingTable_LinearGraphNumberOfEntries_2() {
	std::list<rinad::FlowStateObject> objects;
	rinad::FlowStateObject fso1(1, 2, 1, true, 1, 1);
	rinad::FlowStateObject fso2(2, 1, 1, true, 1, 1);
	rinad::FlowStateObject fso3(2, 3, 1, true, 1, 1);
	rinad::FlowStateObject fso4(3, 2, 1, true, 1, 1);
	rinad::IRoutingAlgorithm * routingAlgorithm;
	std::list<rina::RoutingTableEntry *> rtable;
	routingAlgorithm = new rinad::DijkstraAlgorithm();
	int result = 0;

	objects.push_back(fso1);
	objects.push_back(fso2);
	objects.push_back(fso3);
	objects.push_back(fso4);

	rtable = routingAlgorithm->computeRoutingTable(rinad::Graph(objects),
						       objects, 1);
	if (rtable.size() != 2) {
		result = -1;
	}

	delete routingAlgorithm;
	return result;
}

int getRoutingTable_StateFalseNoEntries_True() {
	std::list<rinad::FlowStateObject> objects;
	rinad::FlowStateObject fso1(1, 2, 1, true, 1, 1);
	rinad::FlowStateObject fso2(2, 1, 1, false, 1, 1);
	rinad::IRoutingAlgorithm * routingAlgorithm;
	std::list<rina::RoutingTableEntry *> rtable;
	int result = 0;

	routingAlgorithm = new rinad::DijkstraAlgorithm();

	objects.push_back(fso1);
	objects.push_back(fso2);

	rtable = routingAlgorithm->computeRoutingTable(rinad::Graph(objects),
						       objects, 1);
	if (rtable.size() != 0) {
		result = -1;
	}

	delete routingAlgorithm;
	return result;
}

int getRoutingTable_MultiGraphEntries_True() {
	std::list<rinad::FlowStateObject> objects;
	rinad::FlowStateObject fso1(1, 2, 1, true, 1, 1);
	rinad::FlowStateObject fso2(2, 1, 1, true, 1, 1);
	rinad::FlowStateObject fso3(1, 3, 1, true, 1, 1);
	rinad::FlowStateObject fso4(3, 1, 1, true, 1, 1);
	rinad::FlowStateObject fso5(2, 3, 1, true, 1, 1);
	rinad::FlowStateObject fso6(3, 2, 1, true, 1, 1);
	rinad::IRoutingAlgorithm * routingAlgorithm;
	std::list<rina::RoutingTableEntry *> rtable;
	int result = 0;

	routingAlgorithm = new rinad::DijkstraAlgorithm();

	objects.push_back(fso1);
	objects.push_back(fso2);
	objects.push_back(fso3);
	objects.push_back(fso4);
	objects.push_back(fso5);
	objects.push_back(fso6);

	rtable = routingAlgorithm->computeRoutingTable(rinad::Graph(objects),
						       objects, 1);
	if (rtable.size() != 2) {
		result = -1;
	}

	delete routingAlgorithm;
	return result;
}

int getRoutingTable_MoreGraphEntries_True(bool lfa) {
	std::list<rinad::FlowStateObject> objects;
	rinad::IRoutingAlgorithm * routingAlgorithm;
	std::list<rina::RoutingTableEntry *> rtable;
	std::vector<unsigned int> exp_nhops;
	rinad::IResiliencyAlgorithm *resalg;
	int result = 0;

	routingAlgorithm = new rinad::DijkstraAlgorithm();

	objects.push_back(rinad::FlowStateObject(1, 2, 1, true, 1, 1));
	objects.push_back(rinad::FlowStateObject(2, 1, 1, true, 1, 1));
	objects.push_back(rinad::FlowStateObject(1, 4, 1, true, 1, 1));
	objects.push_back(rinad::FlowStateObject(4, 1, 1, true, 1, 1));
	objects.push_back(rinad::FlowStateObject(1, 3, 1, true, 1, 1));
	objects.push_back(rinad::FlowStateObject(3, 1, 1, true, 1, 1));
	objects.push_back(rinad::FlowStateObject(2, 5, 1, true, 1, 1));
	objects.push_back(rinad::FlowStateObject(5, 2, 1, true, 1, 1));
	objects.push_back(rinad::FlowStateObject(4, 5, 1, true, 1, 1));
	objects.push_back(rinad::FlowStateObject(5, 4, 1, true, 1, 1));
	objects.push_back(rinad::FlowStateObject(4, 6, 1, true, 1, 1));
	objects.push_back(rinad::FlowStateObject(6, 4, 1, true, 1, 1));
	objects.push_back(rinad::FlowStateObject(5, 7, 1, true, 1, 1));
	objects.push_back(rinad::FlowStateObject(7, 5, 1, true, 1, 1));
	objects.push_back(rinad::FlowStateObject(6, 7, 1, true, 1, 1));
	objects.push_back(rinad::FlowStateObject(7, 6, 1, true, 1, 1));
	objects.push_back(rinad::FlowStateObject(3, 7, 1, true, 1, 1));
	objects.push_back(rinad::FlowStateObject(7, 3, 1, true, 1, 1));

	exp_nhops.resize(8);
	exp_nhops[0] = exp_nhops[1] = 1;  // Not meaningful
	exp_nhops[2] = 2;
	exp_nhops[3] = 3;
	exp_nhops[4] = 4;
	exp_nhops[5] = 2;
	exp_nhops[6] = 4;
	exp_nhops[7] = 3;

	rinad::Graph graph(objects);

	rtable = routingAlgorithm->computeRoutingTable(graph, objects, 1);

	for (std::list<rina::RoutingTableEntry *>::iterator
			rit = rtable.begin(); rit != rtable.end(); rit++) {
		const rina::RoutingTableEntry& e = **rit;
		bool ok = false;

		std::cout << "Dest: " << e.address << ", Cost: " << e.cost <<
				", NextHopsAlts: {";

		if (e.address < 1 || e.address > 7) {
			std::cout << std::endl;
			result = -1;
			break;
		}

		for (std::list<rina::NHopAltList>::const_iterator
			altl = e.nextHopAddresses.begin();
				altl != e.nextHopAddresses.end(); altl++) {

			std::cout << "[";
			for (std::list<unsigned int>::const_iterator
				lit = altl->alts.begin();
					lit != altl->alts.end(); lit++) {
				std::cout << *lit << ", ";
				if (*lit == exp_nhops[e.address]) {
					ok = true;
				}
			}
			std::cout << "] ";
		}

		if (!ok) {
			std::cout << std::endl;
			result = -1;
			break;
		}

		std::cout << "}" << std::endl;
	}

	if (lfa) {
		resalg = new rinad::LoopFreeAlternateAlgorithm(*routingAlgorithm);
		resalg->fortifyRoutingTable(graph, 1, rtable);
	}

	delete routingAlgorithm;
	return result;
}

int test_dijkstra() {
	int result = 0;

	result = getRoutingTable_NoFSO_size0();
	if (result < 0) {
		LOG_IPCP_ERR("getPDUTForwardingTable_NoFSO_size0 test failed");
		return result;
	}
	LOG_IPCP_INFO("getPDUTForwardingTable_NoFSO_size0 test passed");

	result = getRoutingTable_LinearGraphNumberOfEntries_2();
	if (result < 0) {
		LOG_IPCP_ERR("getPDUTForwardingTable_LinearGraphNumberOfEntries_2 test failed");
		return result;
	}
	LOG_IPCP_INFO("getPDUTForwardingTable_LinearGraphNumberOfEntries_2 test passed");

	result = getRoutingTable_StateFalseNoEntries_True();
	if (result < 0) {
		LOG_IPCP_ERR("getPDUTForwardingTable_StateFalseNoEntries_True test failed");
		return result;
	}
	LOG_IPCP_INFO("getPDUTForwardingTable_StateFalseNoEntries_True test passed");

	result = getRoutingTable_MultiGraphEntries_True();
	if (result < 0) {
		LOG_IPCP_ERR("getPDUTForwardingTable_MultiGraphEntries_True test failed");
		return result;
	}
	LOG_IPCP_INFO("getPDUTForwardingTable_MultiGraphEntries_True test passed");

	result = getRoutingTable_MoreGraphEntries_True(false);
	if (result < 0) {
		LOG_IPCP_ERR("getPDUTForwardingTable_MoreGraphEntries_True test failed");
		return result;
	}
	LOG_IPCP_INFO("getPDUTForwardingTable_MoreGraphEntries_True test passed");

	result = getRoutingTable_MoreGraphEntries_True(true);
	if (result < 0) {
		LOG_IPCP_ERR("getPDUTForwardingTable_MoreGraphEntriesLFA_True test failed");
		return result;
	}
	LOG_IPCP_INFO("getPDUTForwardingTable_MoreGraphEntriesLFA_True test passed");

	return result;
}

int getRoutingTable_MultipathGraphRoutesTo4_2() {
	int result = 0;

	/*          node2
	*      /               \
	* node1                node4
	*      \               /
	*          node3
	*/
	
	std::list<rinad::FlowStateObject > objects;

	rinad::FlowStateObject fso1 = rinad::FlowStateObject(1, 2, 1, true, 1, 1);
	rinad::FlowStateObject fso2 = rinad::FlowStateObject(2, 1, 1, true, 1, 1);
	rinad::FlowStateObject fso3 = rinad::FlowStateObject(1, 3, 1, true, 1, 1);
	rinad::FlowStateObject fso4 = rinad::FlowStateObject(3, 1, 1, true, 1, 1);
	rinad::FlowStateObject fso5 = rinad::FlowStateObject(4, 2, 1, true, 1, 1);
	rinad::FlowStateObject fso6 = rinad::FlowStateObject(2, 4, 1, true, 1, 1);
	rinad::FlowStateObject fso7 = rinad::FlowStateObject(4, 3, 1, true, 1, 1);
	rinad::FlowStateObject fso8 = rinad::FlowStateObject(3, 4, 1, true, 1, 1);
	objects.push_back(fso1);
	objects.push_back(fso2);
	objects.push_back(fso3);
	objects.push_back(fso4);
	objects.push_back(fso5);
	objects.push_back(fso6);
	objects.push_back(fso7);
	objects.push_back(fso8);

	rinad::IRoutingAlgorithm * routingAlgorithm =
	    new rinad::ECMPDijkstraAlgorithm();

	rinad::Graph graph(objects);

	std::list<rina::RoutingTableEntry *> rtable =
	    routingAlgorithm->computeRoutingTable(graph, objects, 1);

	if (rtable.size() != 3) {
		result = -1;
	}

	std::list<rina::RoutingTableEntry *>::iterator it;
	for (it = rtable.begin(); it != rtable.end(); ++it) {
		const rina::RoutingTableEntry& e = **it;
		if (e.address == 4) {
			if (e.nextHopAddresses.size() != 2) {
				result = -1;
			}
		}
		
		std::cout << "To address: " << e.address << ", ";
		std::cout << "Next hops: ";

		for (std::list<rina::NHopAltList>::const_iterator
			altl = e.nextHopAddresses.begin();
				altl != e.nextHopAddresses.end(); altl++) {
			
			std::cout << "[";
			for (std::list<unsigned int>::const_iterator
				lit = altl->alts.begin();
					lit != altl->alts.end(); lit++) {
				std::cout << *lit;
			}
			std::cout << "] ";
	
		}
		std::cout << std::endl;
	}

	delete routingAlgorithm;
	return result;
}

int getRoutingTable_MultipathGraphRoutesTest() {
	int result = 0;

	/*         -- node2 --
	*      /2      |       \1
	* node1        |1         node4
	*      \1      |       /2
	*     |    -- node3 --   |
	*      \1	       /2
	*	   -- node5 --
	*/

	std::list<rinad::FlowStateObject> objects;
	rinad::FlowStateObject fso1 = rinad::FlowStateObject(1, 2, 2, true, 1, 1);
	rinad::FlowStateObject fso2 = rinad::FlowStateObject(2, 1, 2, true, 1, 1);
	rinad::FlowStateObject fso3 = rinad::FlowStateObject(1, 3, 1, true, 1, 1);
	rinad::FlowStateObject fso4 = rinad::FlowStateObject(3, 1, 1, true, 1, 1);
	rinad::FlowStateObject fso5 = rinad::FlowStateObject(2, 3, 1, true, 1, 1);
	rinad::FlowStateObject fso6 = rinad::FlowStateObject(3, 2, 1, true, 1, 1);
	rinad::FlowStateObject fso7 = rinad::FlowStateObject(4, 2, 1, true, 1, 1);
	rinad::FlowStateObject fso8 = rinad::FlowStateObject(2, 4, 1, true, 1, 1);
	rinad::FlowStateObject fso9 = rinad::FlowStateObject(4, 3, 2, true, 1, 1);
	rinad::FlowStateObject fso10 = rinad::FlowStateObject(3, 4, 2, true, 1, 1);
	rinad::FlowStateObject fso11 = rinad::FlowStateObject(1, 5, 1, true, 1, 1);
	rinad::FlowStateObject fso12 = rinad::FlowStateObject(5, 4, 2, true, 1, 1);
	rinad::FlowStateObject fso13 = rinad::FlowStateObject(5, 1, 1, true, 1, 1);
	rinad::FlowStateObject fso14 = rinad::FlowStateObject(4, 5, 2, true, 1, 1);
	objects.push_back(fso1);
	objects.push_back(fso2);
	objects.push_back(fso3);
	objects.push_back(fso4);
	objects.push_back(fso5);
	objects.push_back(fso6);
	objects.push_back(fso7);
	objects.push_back(fso8);
	objects.push_back(fso9);
	objects.push_back(fso10);
	objects.push_back(fso11);
	objects.push_back(fso12);
	objects.push_back(fso13);
	objects.push_back(fso14);

	rinad::IRoutingAlgorithm * routingAlgorithm =
	    new rinad::ECMPDijkstraAlgorithm();

	rinad::Graph graph(objects);

	std::list<rina::RoutingTableEntry *> rtable =
	    routingAlgorithm->computeRoutingTable(graph, objects, 1);


	if (rtable.size() != 4) {
		result = -1;
	}

	std::list<rina::RoutingTableEntry *>::iterator it;
	for (it = rtable.begin(); it != rtable.end(); ++it) {
		const rina::RoutingTableEntry& e = **it;
		if (e.address == 4) {
		    if (e.nextHopAddresses.size() != 3) {
			result = -1;
		    }
		}
	
		std::cout << "To address: " << e.address << ", ";
		std::cout << "Next hops: ";
		
		for (std::list<rina::NHopAltList>::const_iterator
			altl = e.nextHopAddresses.begin();
				altl != e.nextHopAddresses.end(); altl++) {
			
			std::cout << "[";
			for (std::list<unsigned int>::const_iterator
				lit = altl->alts.begin();
					lit != altl->alts.end(); lit++) {
				std::cout << *lit;
			}
			std::cout << "] ";
	
		}
		std::cout << std::endl;
	}

	delete routingAlgorithm;
	return result;
}

int test_mp_dijkstra() {
	int result = 0;

	result = getRoutingTable_MultipathGraphRoutesTo4_2();
	if (result < 0) {
		LOG_IPCP_ERR("getRoutingTable_MultipathGraphRoutesTo4_2 test failed");
		return result;
	}
	LOG_IPCP_INFO("getRoutingTable_MultipathGraphRoutesTo4_2 test passed");

//	result = getRoutingTable_MultipathGraphCost2();
//	if (result < 0) {
//		LOG_IPCP_ERR("getRoutingTable_MultipathGraphCost2 test failed");
//		return result;
//	}
//	LOG_IPCP_INFO("getRoutingTable_MultipathGraphCost2 test passed");
//
	result = getRoutingTable_MultipathGraphRoutesTest();
	if (result < 0) {
		LOG_IPCP_ERR("getRoutingTable_MultipathGraphMultipleLinkCosts test failed");
		return result;
	}
	LOG_IPCP_INFO("getRoutingTable_MultipathGraphMultipleLinkCosts test passed");
	return result;
}

int main()
{
	int result = 0;

	/*result = test_flow_state_object_db();
	if (result < 0) {
		LOG_IPCP_ERR("test_flow_state_object_db tests failed");
		return result;
	}
	LOG_IPCP_INFO("test_flow_state_object_db tests passed");*/

	result = test_graph();
	if (result < 0) {
		LOG_IPCP_ERR("test_graph tests failed");
		return result;
	}
	LOG_IPCP_INFO("test_graph tests passed");

	result = test_dijkstra();
	if (result < 0) {
		LOG_IPCP_ERR("test_dijkstra tests failed");
		return result;
	}
	LOG_IPCP_INFO("test_dijkstra tests passed");

	result = test_mp_dijkstra();
	if (result < 0) {
		LOG_IPCP_ERR("test_mp_dijkstra tests failed");
		return result;
	}
	LOG_IPCP_INFO("test_mp_dijkstra tests passed");
	return 0;
}

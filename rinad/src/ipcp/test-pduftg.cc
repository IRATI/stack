//
// test-pduftg
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

#define RINA_PREFIX "pduft-generator-tests"

#include <librina/logs.h>

#include "ipcp/pduft-generator.h"

class FakeEncoder: public rinad::EncoderInterface {
public:
	const rina::SerializedObject* encode(const void* object) {
		if (!object) {
			return 0;
		}
		rina::SerializedObject * result;
		result = new rina::SerializedObject(0, 0);
		return result;
	}

	void* decode(const rina::SerializedObject &serialized_object) const {
		LOG_DBG("%d", serialized_object.size_);
		return 0;
	}
};

class FakeRIBDaemon: public rinad::IRIBDaemon {
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
	void addRIBObject(rinad::BaseRIBObject * ribObject){
		if (!ribObject) {
			return;
		}
	}
	void removeRIBObject(rinad::BaseRIBObject * ribObject) {
		if (!ribObject) {
			return;
		}
	}
	void removeRIBObject(const std::string& objectName){
		LOG_DBG("Removing object with name %s", objectName.c_str());
	}
	void sendMessages(const std::list<const rina::CDAPMessage*>& cdapMessages,
				const rinad::IUpdateStrategy& updateStrategy){
		LOG_DBG("%d, %p", cdapMessages.size(), &updateStrategy);
	}
	void sendMessage(const rina::CDAPMessage & cdapMessage,
	            int sessionId, rinad::ICDAPResponseMessageHandler * cdapMessageHandler){
		if (!cdapMessageHandler)  {
			return;
		}
		LOG_DBG("%d, %d ", cdapMessage.op_code_, sessionId);
	}
	void sendMessageToAddress(const rina::CDAPMessage & cdapMessage,
	            int sessionId, unsigned int address,
	            rinad::ICDAPResponseMessageHandler * cdapMessageHandler){
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
	             const void* objectValue, const rinad::NotificationPolicy * notificationPolicy){
		operationCalled(objectClass, objectName, objectValue);
		if (!notificationPolicy) {
			return;
		}
	}
	void deleteObject(const std::string& objectClass, const std::string& objectName,
	             const void* objectValue, const rinad::NotificationPolicy * notificationPolicy) {
		operationCalled(objectClass, objectName, objectValue);
		if (!notificationPolicy) {
			return;
		}
	}
	rinad::BaseRIBObject * readObject(const std::string& objectClass,
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
		LOG_DBG("Event: %d", event.getType());
	}
	std::list<rinad::BaseRIBObject *> getRIBObjects() {
		std::list<rinad::BaseRIBObject *> result;
		return result;
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

class FakeIPCProcess: public rinad::IPCProcess {
public:
	FakeIPCProcess() {
		encoder_ = new rinad::Encoder();
		encoder_->addEncoder(rinad::EncoderConstants::FLOW_STATE_OBJECT_RIB_OBJECT_CLASS,
				new FakeEncoder());
		rib_daemon_ = new FakeRIBDaemon();
		state_= rinad::INITIALIZED;
		address_ = 0;
	}
	~FakeIPCProcess(){
		delete encoder_;
		delete rib_daemon_;
	}
	unsigned short get_id() {
		return 0;
	}
	rinad::IDelimiter* get_delimiter() {
		return 0;
	}
	rinad::Encoder* get_encoder() {
		return encoder_;
	}
	rina::CDAPSessionManagerInterface* get_cdap_session_manager() {
		return 0;
	}
	rinad::IEnrollmentTask* get_enrollment_task() {
		return 0;
	}
	rinad::IFlowAllocator* get_flow_allocator() {
		return 0;
	}
	rinad::INamespaceManager* get_namespace_manager() {
		return 0;
	}
	rinad::IResourceAllocator* get_resource_allocator() {
		return 0;
	}
	rinad::IRIBDaemon* get_rib_daemon() {
		return rib_daemon_;
	}
	unsigned int get_address() {
		return address_;
	}
	void set_address(unsigned int address){
		address_ = address;
	}
	const rina::ApplicationProcessNamingInformation& get_name() const {
		return name_;
	}
	const rinad::IPCProcessOperationalState& get_operational_state() const {
		return state_;
	}
	void set_operational_state(const rinad::IPCProcessOperationalState& operational_state){
		state_ = operational_state;
	}
	const rina::DIFInformation& get_dif_information() const {
		return dif_information_;
	}
	void set_dif_information(const rina::DIFInformation& dif_information) {
		dif_information_ = dif_information;
	}
	const std::list<rina::Neighbor>& get_neighbors() const {
		return neighbors_;
	}
	unsigned int getAdressByname(const rina::ApplicationProcessNamingInformation& name) {
		LOG_DBG("%s", name.getEncodedString().c_str());
		return 0;
	}

private:
	rinad::Encoder * encoder_;
	rinad::IRIBDaemon * rib_daemon_;
	rina::ApplicationProcessNamingInformation name_;
	rinad::IPCProcessOperationalState state_;
	rina::DIFInformation dif_information_;
	std::list<rina::Neighbor> neighbors_;
	unsigned int address_;
};

int addObjectToGroup_NoObjectCheckModified_False() {
	rinad::FlowStateDatabase fsdb = rinad::FlowStateDatabase(0,0,0);
	if (fsdb.modified_ == true) {
		return -1;
	}

	return 0;
}

int addObjectToGroup_AddObjectCheckModified_True() {
	FakeIPCProcess ipcProcess;
	rinad::FlowStateRIBObjectGroup group = rinad::FlowStateRIBObjectGroup(&ipcProcess, 0);
	rinad::FlowStateDatabase fsdb = rinad::FlowStateDatabase(ipcProcess.get_encoder(),
			&group,0);

	fsdb.addObjectToGroup(1, 1, 1, 1);
	if (fsdb.modified_ == false) {
		return -1;
	}

	return 0;
}

int test_flow_state_object_db () {
	int result = 0;

	result = addObjectToGroup_NoObjectCheckModified_False();
	if (result < 0) {
		return result;
	}

	result = addObjectToGroup_AddObjectCheckModified_True();
	if (result < 0) {
		return result;
	}

	return result;
}

int main()
{
	int result = 0;


	result = test_flow_state_object_db();
	if (result < 0) {
		return result;
	}

	return 0;
}

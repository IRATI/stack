//
// Test-CDAP
//
//	  Bernat Gaston			<bernat.gaston@i2cat.net>
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
#include <iostream>
#include "librina/cdap.h"
#include "librina/ipc-process.h"
#include "librina/common.h"

using namespace rina;

bool m_Connect(CDAPSessionManagerInterface &session_manager,
		int port_A, int port_B) {
	AuthValue auth_value;
	const CDAPMessage *sent_message, *recevied_message;
	const SerializedMessage *serialized_message;
	bool assert = false;

	// M_CONNECT Message
	sent_message = session_manager.getOpenConnectionRequestMessage(port_A,
			CDAPMessage::AUTH_NONE, auth_value, "1", "B instance", "1", "B",
			"1", "A instance", "1", "A");
	serialized_message = session_manager.encodeNextMessageToBeSent(*sent_message, port_A);
	session_manager.messageSent(*sent_message, port_A);
	recevied_message = session_manager.messageReceived(*serialized_message, port_B);
	if (sent_message->to_string() == recevied_message->to_string()) {
		assert = true;
	}

	delete sent_message;
	sent_message = 0;
	delete recevied_message;
	recevied_message = 0;
	delete serialized_message;
	serialized_message = 0;

	return assert;
}

int m_Connect_R(CDAPSessionManagerInterface &session_manager,
		int port_A, int port_B) {
	AuthValue auth_value;
	const CDAPMessage *sent_message, *recevied_message;
	const SerializedMessage *serialized_message;
	int assert = -1;

	// M_CONNECT_R message
	sent_message = session_manager.getOpenConnectionResponseMessage(
			CDAPMessage::AUTH_NONE, auth_value, "1", "A instance", "1", "A", 1,
			"OK", "1", "B instance", "1", "B", 1);
	serialized_message = session_manager.encodeNextMessageToBeSent(*sent_message, port_B);
	session_manager.messageSent(*sent_message, port_B);
	try {
		recevied_message = session_manager.messageReceived(*serialized_message, port_A);
		if (sent_message->to_string() == recevied_message->to_string()) {
			assert = 1;
			delete recevied_message;
			recevied_message = 0;
		}
	} catch (CDAPException &e) {
		assert = 1;
	}

	delete sent_message;
	sent_message = 0;
	delete serialized_message;
	serialized_message = 0;

	return assert;
}
bool connect(CDAPSessionManagerInterface &session_manager, int port_A,
		int port_B) {
	bool bool_assert = true;
	int int_assert = 0;

	session_manager.createCDAPSession(port_A);
	session_manager.createCDAPSession(port_B);

	bool_assert = bool_assert && m_Connect(session_manager, port_A, port_B);
	int_assert = m_Connect_R(session_manager, port_A, port_B);
	if(int_assert == 0)
		bool_assert = bool_assert && true;

	return bool_assert;
}

bool connectTimeout(CDAPSessionManagerInterface &session_manager, int port_A,
		int port_B) {
	bool bool_assert = true;
	int int_assert = 0;
	Sleep sleep;

	session_manager.createCDAPSession(port_A);
	session_manager.createCDAPSession(port_B);

	bool_assert = bool_assert && m_Connect(session_manager, port_A, port_B);
	sleep.sleepForSec(3);
	int_assert = m_Connect_R(session_manager, port_A, port_B);
	if(int_assert == 1)
		bool_assert = bool_assert && true;
	else
		bool_assert = bool_assert && false;

	return bool_assert;
}

int main() {
	bool result = true;
	WireMessageProviderFactory wire_factory;
	CDAPSessionManagerFactory cdap_manager_factory;
	long timeout = 2000;
	CDAPSessionManagerInterface *session_manager;
	bool connection_response;

	// TEST 1
	session_manager = cdap_manager_factory.createCDAPSessionManager(&wire_factory,
					timeout);
	connection_response = connect(*session_manager, 1, 2);
	if (connection_response)
		std::cout << "connect test passed" << std::endl;
	else {
		std::cout << "connect test failed" << std::endl;
		result = false;
	}

	delete session_manager;
	session_manager = 0;

	// TEST 2
	session_manager = cdap_manager_factory.createCDAPSessionManager(&wire_factory,
					timeout);
	connection_response = connectTimeout(*session_manager, 1, 2);
	if (connection_response)
		std::cout << "connectTimeout test passed" << std::endl;
	else {
		std::cout << "connectTimeout test failed" << std::endl;
		result = false;
	}

	delete session_manager;
	session_manager = 0;

	// TEST 3
	session_manager = cdap_manager_factory.createCDAPSessionManager(&wire_factory,
					timeout);
	connection_response = connect(*session_manager, 1, 2);
	if (connection_response)
		std::cout << "connectTimeout test passed" << std::endl;
	else {
		std::cout << "connectTimeout test failed" << std::endl;
		result = false;
	}

	delete session_manager;
	session_manager = 0;

	if (result)
		return 0;
	else
		return -1;
}

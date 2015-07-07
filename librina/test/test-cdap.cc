//
// Test-CDAP
//
//	  Bernat Gaston			<bernat.gaston@i2cat.net>
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
		int port_A, int port_B, int &invoke_id) {
	AuthPolicy auth_policy;
	const CDAPMessage *sent_message, *recevied_message;
	const SerializedObject *serialized_message;
	bool assert = false;

	// M_CONNECT Message
	sent_message = session_manager.getOpenConnectionRequestMessage(port_A,
			auth_policy, "1", "B instance", "1", "B",
			"1", "A instance", "1", "A");
	invoke_id = sent_message->get_invoke_id();
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
		int port_A, int port_B, int invoke_id) {
	AuthPolicy auth_policy;
	const CDAPMessage *sent_message, *recevied_message;
	const SerializedObject *serialized_message;
	int assert = -1;

	// M_CONNECT_R message
	sent_message = session_manager.getOpenConnectionResponseMessage(
			auth_policy, "1", "A instance", "1", "A", 1,
			"OK", "1", "B instance", "1", "B", invoke_id);
	serialized_message = session_manager.encodeNextMessageToBeSent(*sent_message, port_B);
	session_manager.messageSent(*sent_message, port_B);
	try {
		recevied_message = session_manager.messageReceived(*serialized_message, port_A);
		if (sent_message->to_string() == recevied_message->to_string()) {
			assert = 0;
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

bool m_Release(CDAPSessionManagerInterface &session_manager,
		int port_A, int port_B, int invoke_id) {
	const CDAPMessage *sent_message, *recevied_message;
	const SerializedObject *serialized_message;
	bool assert = false;

	// M_RELEASE Message
	sent_message = session_manager.getReleaseConnectionRequestMessage(port_A, CDAPMessage::NONE_FLAGS, invoke_id);
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

int m_Release_R(CDAPSessionManagerInterface &session_manager,
		int port_A, int port_B, int invoke_id) {
	const CDAPMessage *sent_message, *recevied_message;
	const SerializedObject *serialized_message;
	int assert = -1;

	// M_RELEASE_R message
	sent_message = session_manager.getReleaseConnectionResponseMessage(CDAPMessage::NONE_FLAGS, 1, "ok", invoke_id);
	serialized_message = session_manager.encodeNextMessageToBeSent(*sent_message, port_B);
	session_manager.messageSent(*sent_message, port_B);
	try {
		recevied_message = session_manager.messageReceived(*serialized_message, port_A);
		if (sent_message->to_string() == recevied_message->to_string()) {
			assert = 0;
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
		int port_B, int &invoke_id) {
	bool bool_assert = true;
	int int_assert = 0;

	session_manager.createCDAPSession(port_A);

	bool_assert = bool_assert && m_Connect(session_manager, port_A, port_B, invoke_id);
	int_assert = m_Connect_R(session_manager, port_A, port_B, invoke_id);
	if(int_assert != 0)
		bool_assert = false;
	return bool_assert;
}

bool connectTimeout(CDAPSessionManagerInterface &session_manager, int port_A,
		int port_B, int invoke_id) {
	bool bool_assert = true;
	int int_assert = 0;
	Sleep sleep;

	session_manager.createCDAPSession(port_A);

	bool_assert = bool_assert && m_Connect(session_manager, port_A, port_B, invoke_id);
	sleep.sleepForSec(3);
	int_assert = m_Connect_R(session_manager, port_A, port_B, invoke_id);
	if(int_assert != 1)
		bool_assert = false;

	return bool_assert;
}

bool release(CDAPSessionManagerInterface &session_manager, int port_A,
		int port_B, int invoke_id) {
	bool bool_assert = true;
	int int_assert = 0;

	bool_assert = bool_assert && m_Release(session_manager, port_A, port_B, invoke_id);
	int_assert = m_Release_R(session_manager, port_A, port_B, invoke_id);
	if(int_assert != 0)
		bool_assert = false;

	return bool_assert;
}

bool releaseTimeout(CDAPSessionManagerInterface &session_manager, int port_A,
		int port_B, int invoke_id) {
	bool bool_assert = true;
	int int_assert = 0;
	Sleep sleep;

	bool_assert = bool_assert && m_Release(session_manager, port_A, port_B, invoke_id);
	sleep.sleepForSec(4);
	int_assert = m_Release_R(session_manager, port_A, port_B, invoke_id);
	if(int_assert != 1)
		bool_assert = false;
	return bool_assert;
}

int main() {
	bool result = true;
	WireMessageProviderFactory wire_factory;
	CDAPSessionManagerFactory cdap_manager_factory;
	long timeout = 2000;
	CDAPSessionManagerInterface *session_manager;
	int invoke_id = 0;

	// TEST 1
	std::cout<<std::endl <<	"//////////////////////////////////////" << std::endl <<
							"/////////// TEST 1 : connect /////////" << std::endl <<
							"//////////////////////////////////////" << std::endl;
	session_manager = cdap_manager_factory.createCDAPSessionManager(&wire_factory,
					timeout);
	if (connect(*session_manager, 1, 2, invoke_id))
		std::cout << "connect test passed" << std::endl;
	else {
		std::cout << "connect test failed" << std::endl;
		result = false;
	}
	delete session_manager;
	session_manager = 0;

	// TEST 2
	std::cout<<std::endl <<	"//////////////////////////////////////" << std::endl <<
							"////// TEST 2 : connectTimeout ///////" << std::endl <<
							"//////////////////////////////////////" << std::endl;
	session_manager = cdap_manager_factory.createCDAPSessionManager(&wire_factory,
					timeout);
	if (connectTimeout(*session_manager, 1, 2, invoke_id))
		std::cout << "connectTimeout test passed" << std::endl;
	else {
		std::cout << "connectTimeout test failed" << std::endl;
		result = false;
	}
	delete session_manager;
	session_manager = 0;

	// TEST 3
	std::cout<<std::endl <<	"//////////////////////////////////////" << std::endl <<
							"////////// TEST 3 : release //////////" << std::endl <<
							"//////////////////////////////////////" << std::endl;
	session_manager = cdap_manager_factory.createCDAPSessionManager(&wire_factory,
					timeout);
	if (!connect(*session_manager, 1, 2, invoke_id))
		result = false;

	if(release(*session_manager, 1, 2, invoke_id)) {
		std::cout << "Release test passed" << std::endl;
	} else {
		std::cout << "Release test failed" << std::endl;
		result = false;
	}
	delete session_manager;
	session_manager = 0;

	// TEST 4
	std::cout<<std::endl <<	"//////////////////////////////////////" << std::endl <<
							"////// TEST 4 : releaseTimeout ///////" << std::endl <<
							"//////////////////////////////////////" << std::endl;
	session_manager = cdap_manager_factory.createCDAPSessionManager(&wire_factory,
					timeout);
	if (!connect(*session_manager, 1, 2, invoke_id))
		result = false;

	if(releaseTimeout(*session_manager, 1, 2, invoke_id)) {
		std::cout << "ReleaseTimeout test passed" << std::endl;
	} else {
		std::cout << "ReleaseTimeout test failed" << std::endl;
		result = false;
	}
	delete session_manager;
	session_manager = 0;

	if (result)
		return 0;
	else
		return -1;
}

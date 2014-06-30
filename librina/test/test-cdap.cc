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

using namespace rina;

bool connection(CDAPSessionManagerInterface &session_manager, int A_port_id, int B_port_id) {
	AuthValue auth_value;
	bool assert1 = false, assert2 = false;
	const CDAPMessage *sent_message, *recevied_message;
	const SerializedMessage *serialized_message;

	CDAPSessionInterface *session_A = session_manager.createCDAPSession(A_port_id);
	CDAPSessionInterface *session_B = session_manager.createCDAPSession(B_port_id);

	// M_CONNECT Message
	sent_message =
			session_manager.getOpenConnectionRequestMessage(A_port_id,
					CDAPMessage::AUTH_NONE, auth_value, "1", "B instance",
					"1", "B", "1", "A instance", "1", "A");
	serialized_message = session_A->encodeNextMessageToBeSent(
			*sent_message);
	session_A->messageSent(*sent_message);
	recevied_message = session_B->messageReceived(
			*serialized_message);
	if (sent_message->to_string() == recevied_message->to_string())	{
		std::cout<<"M_CONNECT assert passed"<< std::endl;
		assert1 = true;
	}

	delete sent_message;
	sent_message = 0;
	delete recevied_message;
	recevied_message = 0;
	delete serialized_message;
	serialized_message = 0;

	// M_CONNECT_R message
	sent_message =
			session_manager.getOpenConnectionResponseMessage(
					CDAPMessage::AUTH_NONE, auth_value, "1", "A instance",
					"1", "A", 1, "OK", "1", "B instance", "1", "B", 1);
	serialized_message = session_B->encodeNextMessageToBeSent(
			*sent_message);
	session_B->messageSent(*sent_message);
	try{
		recevied_message = session_A->messageReceived(
				*serialized_message);
		if (sent_message->to_string() == recevied_message->to_string())	{
			std::cout<<"M_CONNECT_R assert passed"<< std::endl;
			assert2 = true;
		}
	}
	catch(CDAPException &e)	{
		std::cout<<"CDAP Exception"<<std::endl;
		std::cout<<"OpCode: " << sent_message->get_op_code() << std::endl;
	}
	delete sent_message;
	sent_message = 0;
	delete recevied_message;
	recevied_message = 0;
	delete serialized_message;
	serialized_message = 0;

	return assert1 && assert2;
}

int main() {
	bool result = true;
	WireMessageProviderFactory wire_factory;
	CDAPSessionManagerFactory cdap_manager_factory;
	long timeout = 2000;
	CDAPSessionManagerInterface *session_manager =
			cdap_manager_factory.createCDAPSessionManager(&wire_factory,
					timeout);
	bool connection_response = connection(*session_manager, 1, 2);
	std::cout<<"Connection response: " << connection_response <<std::endl;
	if (!connection_response){
		std::cout<<"Connection test failed"<<std::endl;
		result = false;
	}

	delete session_manager;
	session_manager = 0;

	if (result)
		return 0;
	else
		return -1;
}

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

bool conection(CDAPSessionManagerInterface &session_manager, int sen_port_id, int rec_port_id) {
	AuthValue auth_value;
	bool assert1 = false, assert2 = false;
	const CDAPMessage *sent_message, *recevied_message;
	const SerializedMessage *serialized_message;

	std::cout<<"Creating first session"<< std::endl;
	CDAPSessionInterface *sending_session = session_manager.createCDAPSession(sen_port_id);
	std::cout<<"Creating second session"<< std::endl;
	CDAPSessionInterface *receving_session = session_manager.createCDAPSession(rec_port_id);

	// M_CONNECT Message
	sent_message =
			session_manager.getOpenConnectionRequestMessage(sen_port_id,
					CDAPMessage::AUTH_NONE, auth_value, "1", "B instance",
					"1", "B", "1", "A instance", "1", "A");
	serialized_message = sending_session->encodeNextMessageToBeSent(
			*sent_message);
	sending_session->messageSent(*sent_message);
	recevied_message = receving_session->messageReceived(
			*serialized_message);
	if (sent_message->to_string() == recevied_message->to_string())	{
		assert1 = true;
	}

	delete sent_message;
	delete recevied_message;

	std::cout<< "DEBUG 1" << std::endl;
	// M_CONNECT_R message
	sent_message =
			session_manager.getOpenConnectionResponseMessage(
					CDAPMessage::AUTH_NONE, auth_value, "1", "A instance",
					"1", "A", 1, "OK", "1", "B instance", "1", "B", 1);
	std::cout<< "DEBUG 2" << std::endl;
	serialized_message = sending_session->encodeNextMessageToBeSent(
			*sent_message);
	std::cout<< "DEBUG 3" << std::endl;
	sending_session->messageSent(*sent_message);
	std::cout<< "DEBUG 4" << std::endl;
	recevied_message = receving_session->messageReceived(
			*serialized_message);
	std::cout<< "DEBUG 5" << std::endl;
	if (sent_message->to_string() == recevied_message->to_string())	{
		assert2 = true;
	}
	std::cout<< "DEBUG 6" << std::endl;
	delete sent_message;
	delete recevied_message;

	// FINALLY
	delete sending_session;
	delete receving_session;

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

	if (!conection(*session_manager, 1, 2)){
		std::cout<<"Test1 Failed"<<std::endl;
		result = false;
	}

	if (result)
		return 0;
	else
		return -1;
}

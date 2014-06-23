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

bool test1(CDAPSessionManagerInterface &session_manager, int sen_port_id, int rec_port_id) {
	AuthValue auth_value;
	bool assert = false;

	CDAPSessionInterface *sending_session = session_manager.createCDAPSession(sen_port_id);
	CDAPSessionInterface *receving_session = session_manager.createCDAPSession(rec_port_id);

	const CDAPMessage *sent_message =
			session_manager.getOpenConnectionRequestMessage(sen_port_id,
					CDAPMessage::AUTH_NONE, auth_value, "1", "dest instance",
					"1", "dest", "1", "src instance", "1", "src");
	const SerializedMessage *serialized_message = sending_session->encodeNextMessageToBeSent(
			*sent_message);
	std::cout<<"Adeu"<<std::endl;

	sending_session->messageSent(*sent_message);

	std::cout<<"Adeu"<<std::endl;

	const CDAPMessage *recevied_message = receving_session->messageReceived(
			*serialized_message);
	std::cout<<"Adeu"<<std::endl;

	if (sent_message->to_string() == recevied_message->to_string())	{
		assert = true;
	}

	delete sent_message;
	delete recevied_message;
	delete sending_session;
	delete receving_session;

	return assert;
}

int main() {
	bool result = true;
	WireMessageProviderFactory wire_factory;
	CDAPSessionManagerFactory cdap_manager_factory;
	long timeout = 2000;
	CDAPSessionManagerInterface *session_manager =
			cdap_manager_factory.createCDAPSessionManager(&wire_factory,
					timeout);

	if (!test1(*session_manager, 1, 2)){
		std::cout<<"Test1 Failed"<<std::endl;
		result = false;
	}

	if (result)
		return 0;
	else
		return -1;
}

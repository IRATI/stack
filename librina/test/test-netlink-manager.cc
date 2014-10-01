//
// Test netlink manager
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

#include <iostream>

#include "netlink-manager.h"

using namespace rina;

int main() {
	std::cout << "TESTING LIBRINA-NETLINK-MANAGER\n";

	/* Test User-space to User-space communication */
	NetlinkManager source = NetlinkManager(30);
	NetlinkManager destination = NetlinkManager(31);

	ApplicationProcessNamingInformation sourceName;
	sourceName.processName = "/apps/source";
	sourceName.processInstance = "12";
	sourceName.entityName = "database";
	sourceName.entityInstance = "12";

	ApplicationProcessNamingInformation destName;
	destName.processName = "/apps/dest";
	destName.processInstance = "12345";
	destName.entityName = "printer";
	destName.entityInstance = "12623456";

	FlowSpecification flowSpec;

	ApplicationProcessNamingInformation difName;
	difName.processName = "/difs/test.DIF";

	DIFInformation difInformation;
	difInformation.set_dif_type("shim-dummy");
	difInformation.set_dif_name(difName);

	AppAllocateFlowRequestMessage message;
	message.setDestPortId(31);
	message.setSourceAppName(sourceName);
	message.setDestAppName(destName);
	message.setFlowSpecification(flowSpec);
	message.setRequestMessage(true);
	message.setSequenceNumber(source.getSequenceNumber());
	message.setSourceIpcProcessId(25);
	message.setDestIpcProcessId(38);
	try{
		source.sendMessage(&message);
	}catch(NetlinkException &e){
		std::cout<<"Exception: "<<e.what()<<std::endl;
		return -1;
	}

	AppAllocateFlowRequestMessage * result;
	result = dynamic_cast<AppAllocateFlowRequestMessage *>(destination
			.getMessage());
	std::cout << "Received message from " << result->getSourcePortId()
			<< " with sequence number " << result->getSequenceNumber()
			<< " source IPC Process id "<< result->getSourceIpcProcessId()
			<< " destination IPC Process id "<< result->getDestIpcProcessId()
			<< "\n";
	std::cout << "Source application process name: "
			<< result->getSourceAppName().processName << "\n";
	std::cout << "Destination application process name: "
			<< result->getDestAppName().processName << "\n";
	std::cout << "In order delivery requested: "
			<< result->getFlowSpecification().orderedDelivery << "\n";
	delete result;
}

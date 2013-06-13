//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#include <iostream>
#include "librina-netlink-manager.h"
#include "librina-common.h"

using namespace rina;

int main(int argc, char * argv[]) {
	std::cout << "TESTING LIBRINA-NETLINK-MANAGER\n";

	NetlinkManager * source = new NetlinkManager(4);
	NetlinkManager * destination = new NetlinkManager(5);
	NetlinkManager * source2 = new NetlinkManager(6);

	ApplicationProcessNamingInformation * sourceName = new ApplicationProcessNamingInformation();
	sourceName->setProcessName("/apps/source");
	sourceName->setProcessInstance("12");
	sourceName->setEntityName("database");
	sourceName->setEntityInstance("12");
	source->sendMessage(*sourceName, 5);

	ApplicationProcessNamingInformation * destName = new ApplicationProcessNamingInformation();
	destName->setProcessName("/apps/dest");
	destName->setProcessInstance("12345");
	destName->setEntityName("printer");
	destName->setEntityInstance("12623456");
	source2->sendMessage(*destName, 5);

	ApplicationProcessNamingInformation *  result;
	result = destination->getMessage();
	std::cout<<"Received message!!! \n";
	std::cout<< result->toString() << "\n";
	delete result;
	result = destination->getMessage();
	std::cout<<"Received message!!! \n";
	std::cout<< result->toString() << "\n";
	delete result;

	delete source;
	delete destination;
	delete source2;
	delete sourceName;
	delete destName;
}

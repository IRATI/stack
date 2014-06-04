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
#include <dirent.h>
#include <errno.h>
#include <iostream>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sstream>
#include <cstring>
#include <stdio.h>

#include "rina-syscalls.h"

using namespace rina;


char * stringToCharArray(std::string s){
	char * result = new char[s.size()+1];
	result[s.size()]=0;
	memcpy(result, s.c_str(), s.size());
	return result;
}

char * intToCharArray(int i){
	std::stringstream strs;
	strs << i;
	return stringToCharArray(strs.str());
}

int main(int argc, char * argv[]) {
	std::cout << "TESTING RINA SYSCALLS\n";
	int result = 0;
	int portId1 = 0;
	int portId2 = 0;
	char * sdu = new char[50];

	//Create an IPC Process
	ApplicationProcessNamingInformation * ipcProcessName =
			new ApplicationProcessNamingInformation();
	ipcProcessName->setProcessName("/ipcprocesses/Barcelona/i2CAT");
	ipcProcessName->setProcessInstance("1");
	result = syscallCreateIPCProcess(
			*ipcProcessName, 1, "shim-dummy");
	std::cout<<"Called IPC Process create system call with result "
			<<result<<std::endl;

	//Write management sdu
	result = syscallWriteManagementSDU(1, sdu, 0, 12, 50);
	std::cout<<"Called write management SDU system call with result "
	                <<result<<std::endl;

	//Destroy an IPC Process
	result = syscallDestroyIPCProcess(1);
	std::cout<<"Called IPC Process destroy system call with result "
			<<result<<std::endl;

	//Write SDU (will fail)
	result = syscallWriteSDU(25, sdu, 50);
	std::cout<<"Called write SDU system call with result "
				<<result<<std::endl;

	//Read SDU (will fail)
	result = syscallReadSDU(25, sdu, 50);
	std::cout<<"Called read SDU system call with result "
			<<result<<std::endl;

	//Write management sdu (will fail)
	result = syscallWriteManagementSDU(1, sdu, 12, 0, 50);
	std::cout<<"Called write management SDU system call with result "
	                <<result<<std::endl;

	//Read management sdu (will fail)
	result = syscallReadManagementSDU(1, sdu, &portId1, 50);
	std::cout<<"Called read management SDU system call with result "
	                        <<result<<std::endl;

        //Allocate port-id
	portId1 = syscallAllocatePortId(5, *ipcProcessName);
	std::cout<<"Allocated port id: "<<portId1<<std::endl;
	portId2 = syscallAllocatePortId(1, *ipcProcessName);
	std::cout<<"Allocated port id: "<<portId2<<std::endl;

	//Deallocate port-id
	result = syscallDeallocatePortId(portId1);
	std::cout<<"Deallocate port id result: "<<result<<std::endl;
	result = syscallDeallocatePortId(portId2);
	std::cout<<"Deallocate port id result: "<<result<<std::endl;
	result = syscallDeallocatePortId(34);
	std::cout<<"Deallocate port id result: "<<result<<std::endl;

	delete sdu;
        delete ipcProcessName;
}

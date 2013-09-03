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

#include "rina-syscalls.h"

using namespace rina;

int main(int argc, char * argv[]) {
	std::cout << "TESTING RINA SYSCALLS\n";
	int result = 0;

	//Create an IPC Process
	ApplicationProcessNamingInformation * ipcProcessName =
			new ApplicationProcessNamingInformation();
	ipcProcessName->setProcessName("/ipcprocesses/Barcelona/i2CAT");
	ipcProcessName->setProcessInstance("1");
	result = syscallCreateIPCProcess(
			*ipcProcessName, 1, "shim-dummy");
	std::cout<<"Called IPC Process create system call with result "
			<<result<<std::endl;

	//Destroy an IPC Process
	result = syscallDestroyIPCProcess(1);
	std::cout<<"Called IPC Process destroy system call with result "
			<<result<<std::endl;

	//Write SDU (will fail)
	char * sdu = new char[50];
	result = syscallWriteSDU(25, sdu, 50);
	std::cout<<"Called write SDU system call with result "
				<<result<<std::endl;

	//Read SDU (will fail)
	result = syscallReadSDU(25, sdu, 50);
	std::cout<<"Called read SDU system call with result "
			<<result<<std::endl;

	delete ipcProcessName;

}

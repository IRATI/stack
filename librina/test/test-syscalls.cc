//
// Test syscalls
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

int main() {
	std::cout << "TESTING RINA SYSCALLS\n";
	int result = 0;

	//Create an IPC Process
	ApplicationProcessNamingInformation * ipcProcessName =
			new ApplicationProcessNamingInformation();
	ipcProcessName->processName = "/ipcprocesses/Barcelona/i2CAT";
	ipcProcessName->processInstance = "1";
	result = syscallCreateIPCProcess(
			*ipcProcessName, 1, "shim-dummy");
	std::cout<<"Called IPC Process create system call with result "
			<<result<<std::endl;

	//Destroy an IPC Process
	result = syscallDestroyIPCProcess(1);
	std::cout<<"Called IPC Process destroy system call with result "
			<<result<<std::endl;

        delete ipcProcessName;
}

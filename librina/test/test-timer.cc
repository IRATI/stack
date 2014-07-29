//
// Test 03
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

#include "librina/timer.h"

using namespace rina;

class HelloWorldTimerTask: public TimerTask {
public:
	void run() {
		std::cout << "Hello world" <<std::endl;
	};
};

int main()
{
	Timer * timer = new Timer();
	timer->scheduleTask(new HelloWorldTimerTask(), 100);

	Sleep sleep;
	sleep.sleepForMili(2000);

	delete timer;

	sleep.sleepForMili(1000);
}

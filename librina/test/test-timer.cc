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
	HelloWorldTimerTask(){
		check_ = false;
	};
	void run() {
		std::cout << "Hello world" <<std::endl;
		check_ = true;
	};

	std::string name() const {
		return "Hello world";
	}
bool check_;
};
class GoodbyeWorldTimerTask: public TimerTask {
public:
	GoodbyeWorldTimerTask(){
		check_ = false;
	};
	void run() {
		std::cout << "Goodbye world" <<std::endl;
		check_ = true;
	};

	std::string name() const {
		return "Goodbye world";
	}

	bool check_;
};

int main()
{
	bool result = true;
	std::cout<<std::endl <<	"//////////////////////////////////////" << std::endl <<
							"//// test-timer TEST 1 : one timer ///" << std::endl <<
							"//////////////////////////////////////" << std::endl;
	Timer * timer = new Timer();

	HelloWorldTimerTask *hello = new HelloWorldTimerTask();
	timer->scheduleTask(hello, 100);

	Sleep sleep;
	sleep.sleepForMili(1000);

	if (!hello->check_){
		result = false;
		std::cout<< "TEST 1 FAILED"<<std::endl;
	}

	delete timer;

	std::cout<<std::endl <<	"////////////////////////////////////////////////" << std::endl <<
							"//// test-timer TEST 2 : two timer not equal ///" << std::endl <<
							"////////////////////////////////////////////////" << std::endl;
	timer = new Timer();

	hello = new HelloWorldTimerTask();
	GoodbyeWorldTimerTask *bye = new GoodbyeWorldTimerTask();
	timer->scheduleTask(hello, 100);
	sleep.sleepForMili(500);
	timer->scheduleTask(bye, 100);

	sleep.sleepForMili(1000);

	if (!hello->check_){
		result = false;
		std::cout<< "TEST 2 FAILED"<<std::endl;
	}

	delete timer;

	std::cout<<std::endl <<	"//////////////////////////////////////" << std::endl <<
							"/ test-timer TEST 3 : two timer equal/" << std::endl <<
							"//////////////////////////////////////" << std::endl;
	timer = new Timer();

	hello = new HelloWorldTimerTask();
	bye = new GoodbyeWorldTimerTask();
	timer->scheduleTask(hello, 100);
	timer->scheduleTask(bye, 100);

	sleep.sleepForMili(1000);

	if (!hello->check_){
		result = false;
		std::cout<< "TEST 3 FAILED"<<std::endl;
	}

	delete timer;

	std::cout<<std::endl <<	"////////////////////////////////////////////////////" << std::endl <<
							"/ test-timer TEST 4 : Cancel tasks of the same time/" << std::endl <<
							"////////////////////////////////////////////////////" << std::endl;
	timer = new Timer();

	hello = new HelloWorldTimerTask();
	bye = new GoodbyeWorldTimerTask();
	timer->scheduleTask(hello, 100);
	timer->scheduleTask(bye, 100);

	timer->cancelTask(hello);
	sleep.sleepForMili(1000);

	if (!bye->check_){
		result = false;
		std::cout<< "TEST 4 FAILED"<<std::endl;
	}

	delete timer;

	if (result) {
		std::cout<<std::endl <<	"//////////////////////////////////////" << std::endl <<
								"//////////////////////////////////////" << std::endl <<
								"////////// TIMER TESTS PASSED ////////" << std::endl <<
								"//////////////////////////////////////" << std::endl <<
								"//////////////////////////////////////" << std::endl;
		return 0;
	}
	else
		return -1;
}

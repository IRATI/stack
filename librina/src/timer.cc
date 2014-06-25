//
// Timer facilities
//
//    Bernat Gaston         <bernat.gaston@i2cat.net>
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

#include <cerrno>

#define RINA_PREFIX "timer"

#include "librina/logs.h"
#include "librina/timer.h"

namespace rina {

// CLASS LockableMap
LockableMap::LockableMap() : Lockable() {
}
LockableMap::~LockableMap() throw() {
	LOG_DBG("Lockable Map destroyer");
}
void LockableMap::insert(std::pair<double, TimerTask*> pair) {
	lock();
	tasks_.insert( pair );
	unlock();
}
void LockableMap::clear() {
	lock();
	for (std::map<double, TimerTask*>::iterator iter = tasks_.begin(); iter != tasks_.end(); ++iter)
	{
		LOG_DBG("Erasing task");
		delete iter->second;
		tasks_.erase(iter);
	}
	if (tasks_.empty())
		LOG_DBG("Clear on Lockable map: Tasks are empty");
	else
		LOG_DBG("ALARM!!! Clear on Lockable map: Tasks are not empty");
	unlock();
}
void LockableMap::runTasks() {
	std::clock_t now;
	lock();
	LOG_DBG("Run tasks: There are %d", tasks_.size());
	now = std::clock();
	for (std::map<double, TimerTask*>::iterator iter = tasks_.begin();
			iter != tasks_.upper_bound(now); ++iter)
	{
		LOG_DBG("Running task");
		iter->second->run();
		delete iter->second;
	}
	tasks_.erase(tasks_.begin(), tasks_.upper_bound(now));
	unlock();
}
void LockableMap::cancelTask(TimerTask *task) {
	lock();
	for (std::map<double, TimerTask*>::iterator iter = tasks_.begin();
			iter != tasks_.end(); ++iter)
	{
		if (iter->second == task)
		{
			delete iter->second;
			tasks_.erase(iter);
		}
	}
	unlock();
}

// CLASS Timer
void* doWorkTimers(void *arg) {
	LockableMap *lockableMap = (LockableMap*) arg;
	while(true)
	{
		lockableMap->runTasks();
	}
	return (void *) lockableMap;
}

Timer::Timer() {
	ThreadAttributes threadAttributes;
	thread_ = new Thread(&threadAttributes, &doWorkTimers, (void *) &lockableMap_);
}
Timer::~Timer() {
	LOG_DBG("Timer destroyer called");
	lockableMap_.clear();
	LOG_DBG("Lockable Map cleared");
	delete thread_;
	LOG_DBG("thread deleted");
}
void Timer::scheduleTask(TimerTask* task, double delay_ms) {
	std::clock_t now = std::clock();
	double executeTime = now + delay_ms;
	lockableMap_.insert(std::pair<double, TimerTask*>(executeTime, task));
}
void Timer::cancelTask(TimerTask* task) {
	lockableMap_.cancelTask(task);
}
void Timer::clear() {
	lockableMap_.clear();
}

}


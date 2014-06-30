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
#include <sys/select.h>
#include <sys/time.h>

namespace rina {

// CLASS LockableMap
void* doWorkTask(void *arg) {
	TimerTask *timer_task = (TimerTask*) arg;
	timer_task->run();
	delete timer_task;
	timer_task = 0;
	return (void *) 0;
}

TaskScheduler::TaskScheduler() : Lockable() {
}
TaskScheduler::~TaskScheduler() throw() {
}
void TaskScheduler::insert(double time, TimerTask* timer_task) {
	lock();
	std::pair<double, TimerTask*> pair (time, timer_task);
	tasks_.insert( pair );
	unlock();
}
void TaskScheduler::runTasks() {
	std::clock_t now;
	lock();
	now = std::clock();
	for (std::map<double, TimerTask*>::iterator iter = tasks_.begin();
		iter != tasks_.upper_bound(now); ++iter) {
		ThreadAttributes threadAttributes;
		Thread *t = new Thread(&threadAttributes, &doWorkTask, (void *) iter->second);
		LOG_DBG("Thread with ID %d started ", t);
		delete t;
		t = 0;
		tasks_.erase(iter);
	}
	unlock();
}
void TaskScheduler::cancelTask(TimerTask *task) {
	lock();
	for (std::map<double, TimerTask* >::iterator iter = tasks_.begin();
			iter != tasks_.end(); ++iter) {
		if (iter->second== task)	{
			delete iter->second;
			iter->second = 0;
			tasks_.erase(iter);
		}
	}
	unlock();
}

// CLASS Timer
void* doWorkTimer(void *arg) {
	Timer *timer = (Timer*) arg;
	//timeval timeout;
	//timeout.tv_sec = 0;
	//timeout.tv_usec = 500;
	while(timer->is_continue())
	{
		//select(0, NULL, NULL, NULL, &timeout);
		timer->get_task_scheduler()->runTasks();
	}
	return (void *) 0;
}

Timer::Timer() {
	ThreadAttributes threadAttributes;
	continue_lock_.lock();
	continue_ = true;
	continue_lock_.unlock();
	task_scheduler =  new TaskScheduler();
	thread_ = new Thread(&threadAttributes, &doWorkTimer, (void *) this);
	LOG_DBG("Timer with ID %d started", thread_);
}
Timer::~Timer() {
	cancel();
	delete task_scheduler;
	task_scheduler = 0;
	delete thread_;
	thread_ = 0;
}
void Timer::scheduleTask(TimerTask* task, double delay_ms) {
	std::clock_t now = std::clock();
	double executeTime = now + delay_ms;
	task_scheduler->insert(executeTime, task);
}
void Timer::cancelTask(TimerTask* task) {
	task_scheduler->cancelTask(task);
}
void Timer::cancel() {
	continue_lock_.lock();
	continue_ = false;
	continue_lock_.unlock();
	void *r;
	LOG_DBG("Waiting for the timer %d to join", thread_);
	thread_->join(&r);
	LOG_DBG("Timer with ID %d ended", thread_);
}
TaskScheduler* Timer::get_task_scheduler() const {
	return task_scheduler;
}
bool Timer::is_continue(){
	bool result;
	continue_lock_.lock();
	result = continue_;
	continue_lock_.unlock();
	return result;
}
}


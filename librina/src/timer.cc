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
#include "librina/common.h"

namespace rina {

// CLASS Time
Time::Time() {
	gettimeofday(&time_, 0);
}
Time::Time(timeval t) {
	time_ = t;
}
int Time::get_current_time_in_ms() const {
	return get_time_seconds()*1000 + get_only_milliseconds();
}
int Time::get_time_seconds() const {
	return (int)time_.tv_sec;
}
int Time::get_only_milliseconds() const {
	return (int)(time_.tv_usec/1000);
}
bool Time::operator<(const Time &other) const {
	if ((int)time_.tv_sec != (int)other.get_time_seconds())
		return (int)time_.tv_sec < (int)other.get_time_seconds();
	else
		return (int)time_.tv_usec < (int)other.get_only_milliseconds();
}
void Time::set_timeval(timeval t) {
	time_ = t;
}

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
void TaskScheduler::insert(Time time, TimerTask* timer_task) {
	lock();
	std::pair<Time, TimerTask*> pair (time, timer_task);
	tasks_.insert( pair );
	unlock();
}
void TaskScheduler::runTasks() {
	lock();
	Time now;
	for (std::map<Time, TimerTask*>::iterator iter = tasks_.begin();
		iter != tasks_.upper_bound(now); ++iter) {
		ThreadAttributes threadAttributes;
		Thread *t = new Thread(&threadAttributes, &doWorkTask, (void *) iter->second);
		delete t;
		t = 0;
		tasks_.erase(iter);
	}
	unlock();
}
void TaskScheduler::cancelTask(TimerTask *task) {
	lock();
	for (std::map<Time, TimerTask* >::iterator iter = tasks_.begin();
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
	Sleep sleep;
	while(timer->is_continue())
	{
		sleep.sleepForMili(500);
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
void Timer::scheduleTask(TimerTask* task, long delay_ms) {
	Time executeTime;
	timeval t;
	t.tv_sec = executeTime.get_time_seconds() + (delay_ms / 1000);
	t.tv_usec = executeTime.get_only_milliseconds() + ((delay_ms % 1000) * 1000);
	executeTime.set_timeval(t);
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


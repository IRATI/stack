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

/*
 * concurrency.cc
 *
 *  Created on: 18/06/2013
 *      Author: francescosalvestrini, eduardgrasa
 */

#include "concurrency.h"
#include <errno.h>

namespace rina {

/* CLASS CONCURRENT EXCEPTION */
const std::string ConcurrentException::error_initialize_thread_attributes =
		"Cannot initialize thread attribues";
const std::string ConcurrentException::error_destroy_thread_attributes =
		"Cannot destroy thread attribues";
const std::string ConcurrentException::error_set_thread_attributes =
		"Cannot set thread attribues";
const std::string ConcurrentException::error_get_thread_attributes =
		"Cannot get thread attribues";
const std::string ConcurrentException::error_create_thread =
		"Cannot create thread";
const std::string ConcurrentException::error_join_thread =
		"Cannot join thread";
const std::string ConcurrentException::error_detach_thread =
		"Cannot detach thread";
const std::string ConcurrentException::error_initialize_mutex_attributes =
		"Cannot initialize mutex attribues";
const std::string ConcurrentException::error_set_mutex_attributes =
		"Cannot set mutex attribues";
const std::string ConcurrentException::error_initialize_mutex =
		"Cannot initialize mutex";
const std::string ConcurrentException::error_destroy_mutex_attributes =
		"Cannot destroy mutex attribues";
const std::string ConcurrentException::error_destroy_mutex =
		"Cannot destroy mutex";
const std::string ConcurrentException::error_lock_mutex = "Cannot lock mutex";
const std::string ConcurrentException::error_unlock_mutex =
		"Cannot unlock mutex";
const std::string ConcurrentException::error_invalid_mutex = "Invalid mutex";
const std::string ConcurrentException::error_deadlock = "Deadlock";
const std::string ConcurrentException::error_initialize_rw_lock_attributes =
		"Cannot initialize read_write lock attribues";
const std::string ConcurrentException::error_set_rw_lock_attributes =
		"Cannot set read_write lock attributes";
const std::string ConcurrentException::error_initialize_rw_lock =
		"Cannot initialize read_write lock";
const std::string ConcurrentException::error_destroy_rw_lock_attributes =
		"Cannot destroy read_write lock attribues";
const std::string ConcurrentException::error_write_lock_rw_lock =
		"Cannot get write lock";
const std::string ConcurrentException::error_read_lock_rw_lock =
		"Cannot get read lock";
const std::string ConcurrentException::error_unlock_rw_lock =
		"Cannot unlock read_write lock";
const std::string ConcurrentException::error_invalid_rw_lock =
		"Invalid read_write lock";
const std::string ConcurrentException::error_destroy_rw_lock =
		"Cannot destroy read_write lock";

/* CLASS THREAD ATTRIBUTES */
ThreadAttributes::ThreadAttributes() {
	if (pthread_attr_init(&thread_attr_)) {
		LOG_CRIT(
				ConcurrentException::error_initialize_thread_attributes.c_str());
		throw ConcurrentException(
				ConcurrentException::error_initialize_thread_attributes);
	}
}

ThreadAttributes::~ThreadAttributes() throw () {
	if (pthread_attr_destroy(&thread_attr_)) {
		LOG_CRIT(ConcurrentException::error_destroy_thread_attributes.c_str());
	}
}

pthread_attr_t * ThreadAttributes::getThreadAttributes(){
	return &thread_attr_;
}

bool ThreadAttributes::isJoinable() {
	int dettachState = 0;
	if (pthread_attr_getdetachstate(&thread_attr_, &dettachState)) {
		LOG_CRIT(ConcurrentException::error_get_thread_attributes.c_str());
		throw ConcurrentException(
				ConcurrentException::error_get_thread_attributes);
	}

	if (dettachState == PTHREAD_CREATE_JOINABLE) {
		return true;
	} else {
		return false;
	}
}

void ThreadAttributes::setDetachState(int detachState) {
	if (pthread_attr_setdetachstate(&thread_attr_, detachState)) {
		LOG_CRIT(ConcurrentException::error_set_thread_attributes.c_str());
		throw ConcurrentException(
				ConcurrentException::error_set_thread_attributes);
	}
}

void ThreadAttributes::setJoinable() {
	this->setDetachState(PTHREAD_CREATE_JOINABLE);
}

bool ThreadAttributes::isDettached() {
	return !(this->isJoinable());
}

void ThreadAttributes::setDettached() {
	this->setDetachState(PTHREAD_CREATE_DETACHED);
}

bool ThreadAttributes::isSystemScope() {
	int scope = 0;
	if (pthread_attr_getscope(&thread_attr_, &scope)) {
		LOG_CRIT(ConcurrentException::error_get_thread_attributes.c_str());
		throw ConcurrentException(
				ConcurrentException::error_get_thread_attributes);
	}

	if (scope == PTHREAD_SCOPE_SYSTEM) {
		return true;
	} else {
		return false;
	}
}

void ThreadAttributes::setScope(int scope) {
	if (pthread_attr_setscope(&thread_attr_, scope)) {
		LOG_CRIT(ConcurrentException::error_set_thread_attributes.c_str());
		throw ConcurrentException(
				ConcurrentException::error_set_thread_attributes);
	}
}

void ThreadAttributes::setSystemScope() {
	this->setScope(PTHREAD_SCOPE_SYSTEM);
}

bool ThreadAttributes::isProcessScope() {
	return (!this->isSystemScope());
}

void ThreadAttributes::setProcessScope() {
	this->setScope(PTHREAD_SCOPE_PROCESS);
}

bool ThreadAttributes::isInheritedScheduling() {
	int inheritedScheduling = 0;
	if (pthread_attr_getinheritsched(&thread_attr_, &inheritedScheduling)) {
		LOG_CRIT(ConcurrentException::error_get_thread_attributes.c_str());
		throw ConcurrentException(
				ConcurrentException::error_get_thread_attributes);
	}

	if (inheritedScheduling == PTHREAD_INHERIT_SCHED) {
		return true;
	} else {
		return false;
	}
}

void ThreadAttributes::setInheritedScheduling(int inheritedScheduling) {
	if (pthread_attr_setinheritsched(&thread_attr_, inheritedScheduling)) {
		LOG_CRIT(ConcurrentException::error_set_thread_attributes.c_str());
		throw ConcurrentException(
				ConcurrentException::error_set_thread_attributes);
	}
}

void ThreadAttributes::setInheritedScheduling() {
	this->setInheritedScheduling(PTHREAD_INHERIT_SCHED);
}

bool ThreadAttributes::isExplicitScheduling() {
	return (!this->isInheritedScheduling());
}

void ThreadAttributes::setExplicitScheduling() {
	this->setInheritedScheduling(PTHREAD_EXPLICIT_SCHED);
}

void ThreadAttributes::getSchedulingPolicy(int * schedulingPolicy) {
	if (pthread_attr_getschedpolicy(&thread_attr_, schedulingPolicy)) {
		LOG_CRIT(ConcurrentException::error_get_thread_attributes.c_str());
		throw ConcurrentException(
				ConcurrentException::error_get_thread_attributes);
	}
}

bool ThreadAttributes::isFifoSchedulingPolicy() {
	int schedulingPolicy = 0;
	this->getSchedulingPolicy(&schedulingPolicy);
	if (schedulingPolicy == SCHED_FIFO) {
		return true;
	} else {
		return false;
	}
}

void ThreadAttributes::setSchedulingPolicy(int schedulingPolicy) {
	if (pthread_attr_setschedpolicy(&thread_attr_, schedulingPolicy)) {
		LOG_CRIT(ConcurrentException::error_set_thread_attributes.c_str());
		throw ConcurrentException(
				ConcurrentException::error_set_thread_attributes);
	}
}

void ThreadAttributes::setFifoSchedulingPolicy() {
	this->setSchedulingPolicy(SCHED_FIFO);
}

bool ThreadAttributes::isRRSchedulingPolicy() {
	int schedulingPolicy = 0;
	this->getSchedulingPolicy(&schedulingPolicy);
	if (schedulingPolicy == SCHED_RR) {
		return true;
	} else {
		return false;
	}
}

void ThreadAttributes::setRRSchedulingPolicy() {
	this->setSchedulingPolicy(SCHED_RR);
}

bool ThreadAttributes::isOtherSchedulingPolicy() {
	int schedulingPolicy = 0;
	this->getSchedulingPolicy(&schedulingPolicy);
	if (schedulingPolicy == SCHED_OTHER) {
		return true;
	} else {
		return false;
	}
}

void ThreadAttributes::setOtherSchedulingPolicy() {
	this->setSchedulingPolicy(SCHED_OTHER);
}

/* CLASS THREAD */
Thread::Thread(ThreadAttributes * threadAttributes,
		void *(*startFunction)(void *), void * arg) {
	if (pthread_create(&thread_id_, threadAttributes->getThreadAttributes(),
			startFunction, arg)) {
		LOG_CRIT(ConcurrentException::error_create_thread.c_str());
		throw ConcurrentException(ConcurrentException::error_create_thread);
	}
}

Thread::Thread(pthread_t thread_id_){
	this->thread_id_ = thread_id_;
}

Thread::~Thread() throw () {
}

pthread_t Thread::getThreadType() const{
	return thread_id_;
}

void Thread::join(void ** status){
	if(pthread_join(thread_id_, status)){
		LOG_CRIT(ConcurrentException::error_join_thread.c_str());
		throw ConcurrentException(ConcurrentException::error_join_thread);
	}
}

void Thread::detach(){
	if(pthread_detach(thread_id_)){
		LOG_CRIT(ConcurrentException::error_detach_thread.c_str());
		throw ConcurrentException(ConcurrentException::error_detach_thread);
	}
}

void Thread::exit(void * status){
	pthread_exit(status);
}

Thread * Thread::self(){
	Thread * myself;
	myself = new Thread(pthread_self());
	return myself;
}

int Thread::getConcurrency(){
	return pthread_getconcurrency();
}

void Thread::setConcurrency(int concurrency){
	pthread_setconcurrency(concurrency);
}

bool Thread::operator==(const Thread &other) const{
	if (pthread_equal(thread_id_, other.getThreadType())){
		return true;
	}else{
		return false;
	}
}

bool Thread::operator!=(const Thread &other) const {
	return !(*this == other);
}

/* CLASS LOCKABLE*/
Lockable::Lockable() {
	if (pthread_mutexattr_init(&mutex_attr_)) {
		LOG_CRIT(
				ConcurrentException::error_initialize_mutex_attributes.c_str());
		throw ConcurrentException(
				ConcurrentException::error_initialize_mutex_attributes);
	}

#ifdef _DEBUG
	if (pthread_mutexattr_settype(&mutex_attr_,
					PTHREAD_MUTEX_ERRORCHECK)) {
		LOG_CRIT(ConcurrentException::error_set_mutex_attributes.c_str());
		throw ConcurrentException(ConcurrentException::error_set_mutex_attributes);
	}
#else
	if (pthread_mutexattr_settype(&mutex_attr_, PTHREAD_MUTEX_NORMAL)) {
		LOG_CRIT(ConcurrentException::error_set_mutex_attributes.c_str());
		throw ConcurrentException(
				ConcurrentException::error_set_mutex_attributes);
	}
#endif

	if (pthread_mutex_init(&mutex_, &mutex_attr_)) {
		LOG_CRIT(ConcurrentException::error_initialize_mutex.c_str());
		throw ConcurrentException(ConcurrentException::error_initialize_mutex);
	}
	if (pthread_mutexattr_destroy(&mutex_attr_)) {
		LOG_CRIT(ConcurrentException::error_destroy_mutex_attributes.c_str());
		throw ConcurrentException(
				ConcurrentException::error_destroy_mutex_attributes);
	}

	LOG_DBG("Lockable created successfully");
}

Lockable::~Lockable() throw () {
	if (pthread_mutex_destroy(&mutex_)) {
		LOG_CRIT(ConcurrentException::error_destroy_mutex.c_str());
	}
}

void Lockable::lock() {
	if (pthread_mutex_lock(&mutex_)) {
		LOG_CRIT(ConcurrentException::error_lock_mutex.c_str());
		throw ConcurrentException(ConcurrentException::error_lock_mutex);
	}
}

bool Lockable::trylock() {
	int result = pthread_mutex_trylock(&mutex_);
	switch (result) {
	case 0: {
		return true;
	}
	case EINVAL: {
		LOG_CRIT(ConcurrentException::error_invalid_mutex.c_str());
		throw ConcurrentException(ConcurrentException::error_invalid_mutex);
	}
	case EDEADLK: {
		LOG_CRIT(ConcurrentException::error_deadlock.c_str());
		throw ConcurrentException(ConcurrentException::error_deadlock);
	}
	default: {
		return false;
	}
	}
}

void Lockable::unlock() {
	if (pthread_mutex_unlock(&mutex_)) {
		LOG_CRIT(ConcurrentException::error_unlock_mutex.c_str());
		throw ConcurrentException(ConcurrentException::error_unlock_mutex);
	}
}

pthread_mutex_t * Lockable::getMutex(){
	return &mutex_;
}

/* CLASS READ WRITE LOCKABLE */
ReadWriteLockable::ReadWriteLockable() {
	if (pthread_rwlockattr_init(&rwlock_attr_)) {
		LOG_CRIT(
				ConcurrentException::error_initialize_rw_lock_attributes.c_str());
		throw ConcurrentException(
				ConcurrentException::error_initialize_rw_lock_attributes);
	}

	if (pthread_rwlockattr_setpshared(&rwlock_attr_, PTHREAD_PROCESS_PRIVATE)) {
		LOG_CRIT(ConcurrentException::error_set_rw_lock_attributes.c_str());
		throw ConcurrentException(
				ConcurrentException::error_set_rw_lock_attributes);
	}

	if (pthread_rwlock_init(&rwlock_, &rwlock_attr_)) {
		LOG_CRIT(ConcurrentException::error_initialize_rw_lock.c_str());
		throw ConcurrentException(
				ConcurrentException::error_initialize_rw_lock);
	}

	if (pthread_rwlockattr_destroy(&rwlock_attr_)) {
		LOG_CRIT(ConcurrentException::error_destroy_rw_lock_attributes.c_str());
		throw ConcurrentException(
				ConcurrentException::error_destroy_rw_lock_attributes);
	}

	LOG_DBG("Read/Write Lockable created successfully");
}

ReadWriteLockable::~ReadWriteLockable() throw () {
	if (pthread_rwlock_destroy(&rwlock_)) {
		LOG_CRIT(ConcurrentException::error_destroy_rw_lock.c_str());
	}
}

void ReadWriteLockable::writelock() {
	if (pthread_rwlock_wrlock(&rwlock_)) {
		LOG_CRIT(ConcurrentException::error_write_lock_rw_lock.c_str());
		throw ConcurrentException(
				ConcurrentException::error_write_lock_rw_lock);
	}
}

bool ReadWriteLockable::trywritelock() {
	int result = pthread_rwlock_trywrlock(&rwlock_);
	switch (result) {
	case 0: {
		return true;
	}
	case EBUSY: {
		return false;
	}
	default: {
		LOG_CRIT(ConcurrentException::error_write_lock_rw_lock.c_str());
		throw ConcurrentException(
				ConcurrentException::error_write_lock_rw_lock);
	}
	}
}

void ReadWriteLockable::readlock() {
	if (pthread_rwlock_rdlock(&rwlock_)) {
		LOG_CRIT(ConcurrentException::error_read_lock_rw_lock.c_str());
		throw ConcurrentException(ConcurrentException::error_read_lock_rw_lock);
	}
}

bool ReadWriteLockable::tryreadlock() {
	int result = pthread_rwlock_tryrdlock(&rwlock_);
	switch (result) {
	case 0: {
		return true;
	}
	case EBUSY: {
		return false;
	}
	default: {
		LOG_CRIT(ConcurrentException::error_read_lock_rw_lock.c_str());
		throw ConcurrentException(ConcurrentException::error_read_lock_rw_lock);
	}
	}
}

void ReadWriteLockable::unlock() {
	if (pthread_rwlock_unlock(&rwlock_)) {
		LOG_CRIT(ConcurrentException::error_unlock_rw_lock.c_str());
		throw ConcurrentException(ConcurrentException::error_unlock_rw_lock);
	}
}

pthread_rwlock_t * ReadWriteLockable::getReadWriteLock(){
	return &rwlock_;
}

}


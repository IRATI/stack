//
// Concurrency facilities
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

#include <cerrno>
#include <pthread.h>
#include <unistd.h>

#define RINA_PREFIX "librina.concurrency"

#include "librina/concurrency.h"
#include "librina/logs.h"

namespace rina {

/* Timed wait constants */
#define CONCURRENCY_1S_TO_NS 1000000000

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
const std::string ConcurrentException::error_initialize_cond_attributes =
		"Cannot initialize condition variable attribues";
const std::string ConcurrentException::error_set_cond_attributes =
		"Cannot set condition variable attributes";
const std::string ConcurrentException::error_initialize_cond =
		"Cannot initialize condition variable";
const std::string ConcurrentException::error_destroy_cond_attributes =
		"Cannot destroy condition variable attribues";
const std::string ConcurrentException::error_destroy_cond =
		"Cannot destroy condition variable";
const std::string ConcurrentException::error_signal_cond =
		"Cannot signal condition variable";
const std::string ConcurrentException::error_broadcast_cond =
		"Cannot broadcast condition variable";
const std::string ConcurrentException::error_wait_cond =
		"Cannot wait condition variable";
const std::string ConcurrentException::error_timeout =
		"Timeout";

/* CLASS THREAD */
Thread::Thread(void *(*startFunction)(void *), void * arg,
	       const std::string & name, bool detached)
{
	start_function = startFunction;
	start_arg = arg;
	tname = name;
	is_detach = detached;
	thread_id_ = 0;
}

Thread::Thread(pthread_t tid)
{
	thread_id_ = tid;
	start_arg = 0;
	is_detach = false;
	start_function = 0;
}

Thread::~Thread() throw ()
{
}

void Thread::start()
{
	if (pthread_create(&thread_id_, NULL, start_function, start_arg)) {
		LOG_CRIT("%s (%s). Errno: %d, %s:",
				ConcurrentException::error_create_thread.c_str(),
				tname.c_str(),
				errno, strerror(errno));
		throw ConcurrentException(ConcurrentException::error_create_thread);
	}

	if (tname != "")
		pthread_setname_np(thread_id_, tname.c_str());

	if (is_detach) {
		if (pthread_detach(thread_id_)) {
			LOG_CRIT("Error detaching thread (%d): %s",
					errno, strerror(errno));
			throw ConcurrentException(ConcurrentException::error_create_thread);
		}
	}
}

pthread_t Thread::getThreadType() const{
	return thread_id_;
}

void Thread::join(void ** status){
	int error = pthread_join(thread_id_, status);
	if(error != 0){
		LOG_CRIT("%s. Error is: %d",
				ConcurrentException::error_join_thread.c_str(),
				error);
		throw ConcurrentException(ConcurrentException::error_join_thread);
	}
}

void Thread::detach(){
	if(pthread_detach(thread_id_)){
		LOG_CRIT("%s", ConcurrentException::error_detach_thread.c_str());
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

/* Class SimpleThread */
void * do_simple_thread_work(void * arg)
{
	SimpleThread * simple_thread = (SimpleThread *) arg;
	if (!simple_thread) {
		LOG_ERR("Bogus simple thread passed");
		return (void *) -1;
	}

	return reinterpret_cast<void *>(simple_thread->run());
}

SimpleThread::SimpleThread(const std::string& name, bool detach) :
		Thread(do_simple_thread_work, (void *) this, name, detach)
{
}

SimpleThread::~SimpleThread() throw()
{
}

/* CLASS LOCKABLE*/
Lockable::Lockable() {
	if (pthread_mutexattr_init(&mutex_attr_)) {
		LOG_CRIT("%s", ConcurrentException::error_initialize_mutex_attributes.c_str());
		throw ConcurrentException(
				ConcurrentException::error_initialize_mutex_attributes);
	}

#ifdef _DEBUG
	if (pthread_mutexattr_settype(&mutex_attr_,
					PTHREAD_MUTEX_ERRORCHECK)) {
		throw ConcurrentException(ConcurrentException::error_set_mutex_attributes);
	}
#else
	if (pthread_mutexattr_settype(&mutex_attr_, PTHREAD_MUTEX_NORMAL)) {
		throw ConcurrentException(
				ConcurrentException::error_set_mutex_attributes);
	}
#endif

	if (pthread_mutex_init(&mutex_, &mutex_attr_)) {
		throw ConcurrentException(ConcurrentException::error_initialize_mutex);
	}
	if (pthread_mutexattr_destroy(&mutex_attr_)) {
		throw ConcurrentException(
				ConcurrentException::error_destroy_mutex_attributes);
	}
}

Lockable::~Lockable() throw () {
	if (pthread_mutex_destroy(&mutex_)) {
		LOG_CRIT("%s", ConcurrentException::error_destroy_mutex.c_str());
	}
}

void Lockable::lock() {
	if (pthread_mutex_lock(&mutex_)) {
		LOG_CRIT("%s", ConcurrentException::error_lock_mutex.c_str());
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
		LOG_CRIT("%s", ConcurrentException::error_invalid_mutex.c_str());
		throw ConcurrentException(ConcurrentException::error_invalid_mutex);
	}
	case EDEADLK: {
		LOG_CRIT("%s", ConcurrentException::error_deadlock.c_str());
		throw ConcurrentException(ConcurrentException::error_deadlock);
	}
	default: {
		return false;
	}
	}
}

void Lockable::unlock() {
	if (pthread_mutex_unlock(&mutex_)) {
		LOG_CRIT("%s", ConcurrentException::error_unlock_mutex.c_str());
		throw ConcurrentException(ConcurrentException::error_unlock_mutex);
	}
}

pthread_mutex_t * Lockable::getMutex(){
	return &mutex_;
}

/* CLASS READ WRITE LOCKABLE */
ReadWriteLockable::ReadWriteLockable() {
	if (pthread_rwlockattr_init(&rwlock_attr_)) {
		LOG_CRIT("%s", ConcurrentException::error_initialize_rw_lock_attributes.c_str());
		throw ConcurrentException(
				ConcurrentException::error_initialize_rw_lock_attributes);
	}

	if (pthread_rwlockattr_setpshared(&rwlock_attr_, PTHREAD_PROCESS_PRIVATE)) {
		LOG_CRIT("%s", ConcurrentException::error_set_rw_lock_attributes.c_str());
		throw ConcurrentException(
				ConcurrentException::error_set_rw_lock_attributes);
	}

	if (pthread_rwlock_init(&rwlock_, &rwlock_attr_)) {
		LOG_CRIT("%s", ConcurrentException::error_initialize_rw_lock.c_str());
		throw ConcurrentException(
				ConcurrentException::error_initialize_rw_lock);
	}

	if (pthread_rwlockattr_destroy(&rwlock_attr_)) {
		LOG_CRIT("%s", ConcurrentException::error_destroy_rw_lock_attributes.c_str());
		throw ConcurrentException(
				ConcurrentException::error_destroy_rw_lock_attributes);
	}
}

ReadWriteLockable::~ReadWriteLockable() throw () {
	if (pthread_rwlock_destroy(&rwlock_)) {
		LOG_CRIT("%s", ConcurrentException::error_destroy_rw_lock.c_str());
	}
}

void ReadWriteLockable::writelock() {
	if (pthread_rwlock_wrlock(&rwlock_)) {
		LOG_CRIT("%s", ConcurrentException::error_write_lock_rw_lock.c_str());
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
		LOG_CRIT("%s", ConcurrentException::error_write_lock_rw_lock.c_str());
		throw ConcurrentException(
				ConcurrentException::error_write_lock_rw_lock);
	}
	}
}

void ReadWriteLockable::readlock() {
	if (pthread_rwlock_rdlock(&rwlock_)) {
		LOG_CRIT("%s", ConcurrentException::error_read_lock_rw_lock.c_str());
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
		LOG_CRIT("%s", ConcurrentException::error_read_lock_rw_lock.c_str());
		throw ConcurrentException(ConcurrentException::error_read_lock_rw_lock);
	}
	}
}

void ReadWriteLockable::unlock() {
	if (pthread_rwlock_unlock(&rwlock_)) {
		LOG_CRIT("%s", ConcurrentException::error_unlock_rw_lock.c_str());
		throw ConcurrentException(ConcurrentException::error_unlock_rw_lock);
	}
}

pthread_rwlock_t * ReadWriteLockable::getReadWriteLock(){
	return &rwlock_;
}

/* CLASS CONDITION VARIABLE */
ConditionVariable::ConditionVariable():Lockable() {
	if (pthread_condattr_init(&cond_attr_)) {
		LOG_CRIT("%s", ConcurrentException::error_initialize_cond_attributes.c_str());
		throw ConcurrentException(
				ConcurrentException::error_initialize_cond_attributes);
	}

	if (pthread_condattr_setpshared(&cond_attr_, PTHREAD_PROCESS_PRIVATE)) {
		LOG_CRIT("%s", ConcurrentException::error_set_cond_attributes.c_str());
		throw ConcurrentException(
				ConcurrentException::error_set_cond_attributes);
	}

	if (pthread_cond_init(&cond_, &cond_attr_)) {
		LOG_CRIT("%s", ConcurrentException::error_initialize_cond.c_str());
		throw ConcurrentException(
				ConcurrentException::error_initialize_cond);
	}

	if (pthread_condattr_destroy(&cond_attr_)) {
		LOG_CRIT("%s", ConcurrentException::error_destroy_cond_attributes.c_str());
		throw ConcurrentException(
				ConcurrentException::error_destroy_cond_attributes);
	}
}

ConditionVariable::~ConditionVariable() throw () {
	if (pthread_cond_destroy(&cond_)) {
		LOG_CRIT("%s", ConcurrentException::error_destroy_cond.c_str());
	}
}

void ConditionVariable::signal(){
	if (pthread_cond_signal(&cond_)){
		LOG_CRIT("%s", ConcurrentException::error_signal_cond.c_str());
		throw ConcurrentException(
				ConcurrentException::error_signal_cond);
	}
}

void ConditionVariable::broadcast(){
	if (pthread_cond_broadcast(&cond_)){
		LOG_CRIT("%s", ConcurrentException::error_broadcast_cond.c_str());
		throw ConcurrentException(
				ConcurrentException::error_broadcast_cond);
	}
}

void ConditionVariable::doWait(){
	if (pthread_cond_wait(&cond_, getMutex())){
		LOG_CRIT("%s", ConcurrentException::error_wait_cond.c_str());
		throw ConcurrentException(
				ConcurrentException::error_wait_cond);
	}
}

void ConditionVariable::timedwait(long seconds, long nanoseconds){
	timespec waitTime;

	//Prepare timespec struct
	clock_gettime(CLOCK_REALTIME, &waitTime);
	waitTime.tv_nsec += nanoseconds;

	//Make sure it does not overflow
	while(waitTime.tv_nsec >= CONCURRENCY_1S_TO_NS){
		waitTime.tv_sec++;
		waitTime.tv_nsec -= CONCURRENCY_1S_TO_NS;
	}
	waitTime.tv_sec += seconds;

	int response = pthread_cond_timedwait(&cond_, getMutex(), &waitTime);
	if (response == 0){
		return;
	}

	if (response == ETIMEDOUT){
		throw ConcurrentException(
				ConcurrentException::error_timeout);
	}

	LOG_CRIT("%s", ConcurrentException::error_wait_cond.c_str());
	throw ConcurrentException(
			ConcurrentException::error_wait_cond);
}

// Class Sleep
bool Sleep::sleep(int sec, int milisec) {
	return usleep(sec * 1000000 + milisec * 1000);
}
bool Sleep::sleepForMili(int milisec) {
	return usleep(milisec * 1000);
}
bool Sleep::sleepForSec(int sec) {
	return usleep(sec * 1000000);
}

}

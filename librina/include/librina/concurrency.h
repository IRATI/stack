/*
 * Concurrency
 *
 *    Eduard Grasa          <eduard.grasa@i2cat.net>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

#ifndef LIBRINA_CONCURRENCY_H_
#define LIBRINA_CONCURRENCY_H_

#ifdef __cplusplus

#include <pthread.h>

#include <list>
#include <map>
#include <string>
#include <stdexcept>

#include "librina/patterns.h"

namespace rina {

class ConcurrentException : public Exception {
public:
        ConcurrentException() { }
ConcurrentException(const std::string & s) : Exception(s.c_str()) { }

        static const std::string error_initialize_thread_attributes;
        static const std::string error_destroy_thread_attributes;
        static const std::string error_set_thread_attributes;
        static const std::string error_get_thread_attributes;
        static const std::string error_create_thread;
        static const std::string error_join_thread;
        static const std::string error_detach_thread;
        static const std::string error_initialize_mutex_attributes;
        static const std::string error_set_mutex_attributes;
        static const std::string error_initialize_mutex;
        static const std::string error_destroy_mutex_attributes;
        static const std::string error_destroy_mutex;
        static const std::string error_lock_mutex;
        static const std::string error_unlock_mutex;
        static const std::string error_deadlock;
        static const std::string error_invalid_mutex;
        static const std::string error_initialize_rw_lock_attributes;
        static const std::string error_set_rw_lock_attributes;
        static const std::string error_initialize_rw_lock;
        static const std::string error_destroy_rw_lock_attributes;
        static const std::string error_write_lock_rw_lock;
        static const std::string error_read_lock_rw_lock;
        static const std::string error_unlock_rw_lock;
        static const std::string error_invalid_rw_lock;
        static const std::string error_destroy_rw_lock;
        static const std::string error_initialize_cond_attributes;
        static const std::string error_set_cond_attributes;
        static const std::string error_initialize_cond;
        static const std::string error_destroy_cond_attributes;
        static const std::string error_destroy_cond;
        static const std::string error_signal_cond;
        static const std::string error_broadcast_cond;
        static const std::string error_wait_cond;
        static const std::string error_timeout;
};

/**
 * Wraps a Thread as provided by the pthreads library
 */
class Thread : public NonCopyable {
public:
        /** Calls pthreads.create to create a new thread */
        Thread(void *(* startFunction)(void *), void * arg,
               const std::string & name, bool detached);
        virtual ~Thread() throw();

        void start();
        pthread_t getThreadType() const;
        void join(void ** status);
        void detach();
        static void exit(void * status);
        static Thread * self();
        static int getConcurrency();
        static void setConcurrency(int concurrency);
        bool operator==(const Thread &other) const;
        bool operator!=(const Thread &other) const;

private:
        Thread(pthread_t thread_id_);
        pthread_t thread_id_;
        void *(*start_function)(void *);
        void * start_arg;
        std::string tname;
        bool is_detach;
};

/// A Simple thread that performs all its work in the run method
class SimpleThread : public Thread {
public:
	SimpleThread(const std::string& name, bool detached);
	virtual ~SimpleThread() throw();
	///Subclasses must override this method in order for the
	///to do something useful
	///@return 0 if everything is ok, -1 otherwise
	virtual int run() = 0;
};

/**
 * Wraps a Mutex as provided by the pthreads library
 */
class Lockable : public NonCopyable {
public:
        Lockable();
        virtual ~Lockable() throw();

        virtual void lock();
        virtual bool trylock();
        virtual void unlock();
        pthread_mutex_t * getMutex();

private:
        pthread_mutex_t     mutex_;
        pthread_mutexattr_t mutex_attr_;
};

/**
 * Wraps a Read/Write Lock as provided by the pthreads library
 */
class ReadWriteLockable : public NonCopyable {
public:
        ReadWriteLockable();
        virtual ~ReadWriteLockable() throw();

        virtual void writelock();
        virtual bool trywritelock();
        virtual void readlock();
        virtual bool tryreadlock();
        virtual void unlock();
        pthread_rwlock_t * getReadWriteLock();

private:
        pthread_rwlock_t  rwlock_;
        pthread_rwlockattr_t rwlock_attr_;
};

/**
* Scoped lock (RAI)
*/
class ScopedLock {
public:
ScopedLock(Lockable & guarded, bool lock=true) :
        guarded_(guarded)
        { if(lock) guarded_.lock(); }

        virtual ~ScopedLock() throw() {
                try {
                        guarded_.unlock();
                } catch (std::exception & e) {
                }
        }

private:
        Lockable & guarded_;
};

/**
* Read scoped lock (RAII)
*/
class ReadScopedLock {
public:
ReadScopedLock(ReadWriteLockable & rwlock, bool lock=true) :
        rwlock_(rwlock)
        { if(lock) rwlock_.readlock(); }

        virtual ~ReadScopedLock() throw() {
                try {
                        rwlock_.unlock();
                } catch (std::exception & e) {
                }
        }

private:
	ReadWriteLockable& rwlock_;
};

/**
* Write scoped lock (RAII)
*/
class WriteScopedLock {
public:
WriteScopedLock(ReadWriteLockable & rwlock, bool lock=true) :
        rwlock_(rwlock)
        { if(lock) rwlock_.writelock(); }

        virtual ~WriteScopedLock() throw() {
                try {
                        rwlock_.unlock();
                } catch (std::exception & e) {
                }
        }

private:
	ReadWriteLockable& rwlock_;
};


/**
 * Wraps a Condition Variable as provided by the pthreads library
 */
class ConditionVariable : public Lockable {
public:
        ConditionVariable();
        virtual ~ConditionVariable() throw();

        virtual void signal();
        virtual void broadcast();
        virtual void doWait();
        virtual void timedwait(long seconds, long nanoseconds);

private:
        pthread_cond_t     cond_;
        pthread_condattr_t cond_attr_;
};


/**
 * A queue that provides a blocking behaviour when trying to take
 * an element when the queue is empty
 */
template <class T> class BlockingFIFOQueue: public ConditionVariable {
public:
BlockingFIFOQueue():ConditionVariable() { };
        ~BlockingFIFOQueue() throw() { };

        /** Insert an element at the end of the queue */
        void put(T * element) {
                lock();

                queue.push_back(element);
                if (queue.size() == 1) {
                        broadcast();
                }

                unlock();
        }

        /**
         * Get the element at the begining of the queue. If the queue is
         * empty, this operation will block until there is at least one
         * element
         */
        T * take() {
                T* result;

                lock();

                while(queue.size() == 0) {
                        doWait();
                }

                result = queue.front();
                queue.pop_front();

                unlock();

                return result;
        }

        /**
         * Take an element from the beginig of the queue, waiting maximum
         * the specified seconds and miliseconds
         * @param seconds
         * @param nanoseconds
         * @return
         */

        T* timedtake(long seconds, long nanoseconds) {
                T* result = 0;

                lock();
                if (queue.size() == 0) {
                        try {
                                timedwait(seconds, nanoseconds);
                        } catch(ConcurrentException &e) {
                        }
                }

                if (queue.size() > 0) {
                        result = queue.front();
                        queue.pop_front();
                }

                unlock();

                return result;
        }

        /**
         * Get the element at the begining of the queue. If the queue is
         * empty it will return a NULL pointer.
         */
        T * poll() {
                T * result;

                lock();
                if (queue.size() == 0) {
                        result = 0;
                } else {
                        result = queue.front();
                        queue.pop_front();
                }
                unlock();

                return result;
        }

        /**
         * Get the element at the begining of the queue. It will not remove
         * the item from the queue. If the queue is
         * empty it will return a NULL pointer.
         */
        T * peek() {
                T * result;

                lock();
                if (queue.size() == 0) {
                        result = 0;
                } else {
                        result = queue.front();
                }
                unlock();

                return result;
        }

private:
        std::list<T*> queue;
};

/// Wrapper to sleep a thread
class Sleep{
public:
	bool sleep(int sec, int milisec);
	bool sleepForMili(int milisec);
	bool sleepForSec(int sec);
};

/// A map of pointers that provides thread-safe put, find and erase operations
template <class K, class T> class ThreadSafeMapOfPointers {
public:
        ThreadSafeMapOfPointers() {
        	lock_ = new Lockable();
        };
        ~ThreadSafeMapOfPointers() throw() {
        	if (lock_)
        		delete lock_;
        };

        /// Insert element T* at position K
        /// @param key
        /// @param value
        void put(K key, T* element) {
                rina::ScopedLock g(*lock_);
                map[key] = element;
        }

        /// Get element at position K (returns 0 if k is
        /// unknown)
        /// @param key
        /// @return value
        T* find(K key) {
                typename std::map<K, T*>::iterator iterator;
                T* result;

                rina::ScopedLock g(*lock_);
                iterator = map.find(key);
                if (iterator == map.end()) {
                        result = 0;
                } else {
                        result = iterator->second;
                }

                return result;
        }

        /// Remove element at position K (returns 0 if k
        /// is unknown, or the erased element othewise
        /// @param key
        /// @return value
        T* erase(K key) {
                typename std::map<K, T*>::iterator iterator;
                T* result;

                rina::ScopedLock g(*lock_);
                iterator = map.find(key);
                if (iterator == map.end()) {
                        result = 0;
                } else {
                        result = iterator->second;
                        map.erase(key);
                }

                return result;
        }

        /// Returns the list of entries in the map
        std::list<T*> getEntries() const {
                typename std::map<K, T*>::const_iterator iterator;
                std::list<T*> result;

                rina::ScopedLock g(*lock_);
                for(iterator = map.begin();
                                iterator != map.end(); ++iterator){
                        result.push_back(iterator->second);
                }

                return result;
        }

        /// Returns a copy of the entries in the map
        std::list<T> getCopyofentries() const {
                typename std::map<K, T*>::const_iterator iterator;
                std::list<T> result;

                rina::ScopedLock g(*lock_);
                for(iterator = map.begin();
                                iterator != map.end(); ++iterator){
                        result.push_back(*iterator->second);
                }

                return result;
        }

        std::list<K> getKeys() const {
        	typename std::map<K, T*>::const_iterator iterator;
        	std::list<K> result;

        	rina::ScopedLock g(*lock_);
        	for(iterator = map.begin();
        			iterator != map.end(); ++iterator) {
        		result.push_back(iterator->first);
        	}

        	return result;
        }

        /// Delete all the values of the map
        void deleteValues() {
                typename std::map<K, T*>::const_iterator iterator;

                rina::ScopedLock g(*lock_);
                for(iterator = map.begin();
                                iterator != map.end(); ++iterator){
                		if (iterator->second) {
                				delete iterator->second;
                		}
                }
        }

private:
        std::map<K, T*> map;
        rina::Lockable * lock_;
};

}

#endif

#endif

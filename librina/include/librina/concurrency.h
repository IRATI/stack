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
 * Wraps pthread_attr_t
 */
class ThreadAttributes : public NonCopyable {
public:
        ThreadAttributes();
        virtual ~ThreadAttributes() throw();

        pthread_attr_t * getThreadAttributes();
        bool isJoinable();
        void setJoinable();
        bool isDettached();
        void setDettached();
        bool isSystemScope();
        void setSystemScope();
        bool isProcessScope();
        void setProcessScope();
        bool isInheritedScheduling();
        void setInheritedScheduling();
        bool isExplicitScheduling();
        void setExplicitScheduling();
        bool isFifoSchedulingPolicy();
        void setFifoSchedulingPolicy();
        bool isRRSchedulingPolicy();
        void setRRSchedulingPolicy();
        bool isOtherSchedulingPolicy();
        void setOtherSchedulingPolicy();
private:
        pthread_attr_t thread_attr_;
        void setDetachState(int detachState);
        void setScope(int scope);
        void setInheritedScheduling(int inheritedScheduling);
        void getSchedulingPolicy(int * schedulingPolicy);
        void setSchedulingPolicy(int schedulingPolicy);
};

/**
 * Wraps a Thread as provided by the pthreads library
 */
class Thread : public NonCopyable {
public:
        /** Calls pthreads.create to create a new thread */
        Thread(ThreadAttributes * threadAttributes,
               void *(* startFunction)(void *), void * arg);
        virtual ~Thread() throw();

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

class AccessGuard {
public:
AccessGuard(Lockable & guarded) :
        guarded_(guarded)
        { guarded_.lock(); }

        virtual ~AccessGuard() throw() {
                try {
                        guarded_.unlock();
                } catch (std::exception & e) {
                }
        }

private:
        Lockable & guarded_;
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

private:
        std::list<T*> queue;
};

/// A map of pointers that provides thread-safe put, find and erase operations
template <class K, class T> class ThreadSafeMapOfPointers: public Lockable {
public:
	ThreadSafeMapOfPointers():Lockable() {};
	~ThreadSafeMapOfPointers() throw() {};

	/// Insert element T* at position K
	/// @param key
	/// @param value
	void put(K key, T* element) {
		lock();
		map[key] = element;
		unlock();
	}

	/// Get element at position K (returns 0 if k is
	/// unknown)
	/// @param key
	/// @return value
	T* find(K key) {
		typename std::map<K, T*>::iterator iterator;
		T* result;

		lock();
		iterator = map.find(key);
		if (iterator == map.end()) {
			result = 0;
		} else {
			result = iterator->second;
		}
		unlock();

		return result;
	}

	/// Remove element at position K (returns 0 if k
	/// is unknown, or the erased element othewise
	/// @param key
	/// @return value
	T* erase(K key) {
		typename std::map<K, T*>::iterator iterator;
		T* result;

		lock();
		iterator = map.find(key);
		if (iterator == map.end()) {
			result = 0;
		} else {
			result = iterator->second;
			map.erase(key);
		}
		unlock();

		return result;
	}

	/// Returns the list of entries in the map
	std::list<T*> getEntries() {
		typename std::map<K, T*>::iterator iterator;
		std::list<T*> result;

		lock();
		for(iterator = map.begin();
				iterator != map.end(); ++iterator){
			result.push_back(iterator->second);
		}
		unlock();

		return result;
	}

	/// Delete all the values of the map
	void deleteValues() {
		typename std::map<K, T*>::iterator iterator;

		lock();
		for(iterator = map.begin();
				iterator != map.end(); ++iterator){
			delete iterator->second;
		}
		unlock();
	}

private:
	std::map<K, T*> map;
};

}

#endif

#endif

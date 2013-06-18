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
 * concurrency.h
 *
 *  Created on: 18/06/2013
 *      Author: francescosalvestrini, eduardgrasa
 */

#ifndef CONCURRENCY_H_
#define CONCURRENCY_H_

#include "patterns.h"
#define RINA_PREFIX "concurrency"
#include "logs.h"
#include <pthread.h>

namespace rina{

class LockableException : public Exception {
public:
	LockableException() { }
	LockableException(const std::string & s) : Exception(s) { }
	static const std::string error_initialize_mutex_attributes;
	static const std::string error_set_mutex_attributes;
	static const std::string error_initialize_mutex;
	static const std::string error_destroy_mutex_attributes;
	static const std::string error_destroy_mutex;
	static const std::string error_lock_mutex;
	static const std::string error_unlock_mutex;
};

class Lockable : public NonCopyable {
public:
	Lockable();
	virtual ~Lockable() throw();

	virtual void lock();
	virtual int trylock();
	virtual void unlock();

private:
	pthread_mutex_t     mutex_;
	pthread_mutexattr_t mutex_attr_;
};

class AccessGuard {
public:
	AccessGuard(Lockable & guarded) :
		guarded_(guarded)
{ guarded_.lock(); }

	virtual ~AccessGuard() throw()
                		{
		try {
			guarded_.unlock();
		} catch (std::exception & e) {
			LOG_CRIT("Cannot unlock guarded access");
		}
                		}

private:
	Lockable & guarded_;
};



}


#endif /* CONCURRENCY_H_ */

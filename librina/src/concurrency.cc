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

namespace rina {

/* CLASS LOCKABLE EXCEPTION */
const std::string LockableException::error_initialize_mutex_attributes =
		"Cannot initialize mutex attribues";
const std::string LockableException::error_set_mutex_attributes =
		"Cannot set mutex attribues";
const std::string LockableException::error_initialize_mutex =
		"Cannot initialize mutex";
const std::string LockableException::error_destroy_mutex_attributes =
		"Cannot destroy mutex attribues";
const std::string LockableException::error_destroy_mutex =
		"Cannot destroy mutex";
const std::string LockableException::error_lock_mutex =
		"Cannot lock mutex";
const std::string LockableException::error_unlock_mutex =
		"Cannot unlock mutex";

/* CLASS LOCKABLE*/
Lockable::Lockable() {
	if (pthread_mutexattr_init(&mutex_attr_)) {
		LOG_CRIT(LockableException::error_initialize_mutex_attributes.c_str());
		throw LockableException(
				LockableException::error_initialize_mutex_attributes);
	}

#ifdef _DEBUG
	if (pthread_mutexattr_settype(&mutex_attr_,
					PTHREAD_MUTEX_ERRORCHECK)) {
		LOG_CRIT(LockableException::error_set_mutex_attributes.c_str());
		throw LockableException(LockableException::error_set_mutex_attributes);
	}
#else
	if (pthread_mutexattr_settype(&mutex_attr_, PTHREAD_MUTEX_NORMAL)) {
		throw LockableException(LockableException::error_set_mutex_attributes);
	}
#endif

	if (pthread_mutex_init(&mutex_, &mutex_attr_)) {
		LOG_CRIT(LockableException::error_initialize_mutex.c_str());
		throw LockableException(LockableException::error_initialize_mutex);
	}
	if (pthread_mutexattr_destroy(&mutex_attr_)) {
		LOG_CRIT(LockableException::error_destroy_mutex_attributes.c_str());
		throw LockableException(LockableException::error_destroy_mutex_attributes);
	}

	LOG_DBG("Lockable created successfully");
}

Lockable::~Lockable() throw () {
	if (pthread_mutex_destroy(&mutex_)) {
		LOG_CRIT(LockableException::error_destroy_mutex.c_str());
	}
}

void Lockable::lock() {
	if (pthread_mutex_lock(&mutex_)) {
		LOG_CRIT(LockableException::error_lock_mutex.c_str());
		throw LockableException(LockableException::error_lock_mutex);
	}
}

int Lockable::trylock(){
	return pthread_mutex_trylock(&mutex_);
}

void Lockable::unlock() {
	if (pthread_mutex_lock(&mutex_)) {
		LOG_CRIT(LockableException::error_unlock_mutex.c_str());
		throw LockableException(LockableException::error_unlock_mutex );
	}
}

}


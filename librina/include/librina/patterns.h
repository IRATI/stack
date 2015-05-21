/*
 * Patterns
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

#ifndef LIBRINA_PATTERNS_H
#define LIBRINA_PATTERNS_H

/**
* @file patterns.h
*
* @brief Utility design pattern classes
*/


#ifdef __cplusplus

#include "exceptions.h"

/**
* Non copyable object base class
*/
class NonCopyable {
public:
        NonCopyable()  { }
        ~NonCopyable() { }

private:
        NonCopyable(const NonCopyable &);
        const NonCopyable & operator =(const NonCopyable &);
};

/**
* Singleton pattern base class
*/
template<typename TYPE>
class Singleton : public NonCopyable {

public:
        Singleton()  { }
        ~Singleton() { ___fini___(); }

        TYPE * operator->() {
                ___init___();
                return instance_;
        }

private:
	static TYPE * instance_;

	inline void ___init___() {
		if (!instance_) {
			instance_ = new TYPE();
		}
	}

	inline void ___fini___() {
		if (instance_) {
			delete instance_;
			instance_ = NULL;
		}
	}
};

template<typename TYPE> TYPE * Singleton<TYPE>::instance_ = NULL;

#endif

#endif

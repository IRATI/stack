/*
 * Concurrency utils for IPCP and IPCM
 *
 *    Eduard GRASA <eduard.grasa@i2cat.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef RINAD_CONCURRENCY_H
#define RINAD_CONCURRENCY_H

#ifdef __cplusplus

#include <list>
#include <map>

#include <librina/concurrency.h>

/// A map of pointers that provides thread-safe put, find and erase operations
template <class K, class T> class ThreadSafeMapOfPointers: public rina::Lockable {
public:
	ThreadSafeMapOfPointers():rina::Lockable() {};
	~ThreadSafeMapOfPointers() throw() {};

	/// Insert element T* at position K
	/// @param key
	/// @param value
	void put(K key, T* element) {
		rina::AccessGuard(*this);
		map[key] = element;
	}

	/// Get element at position K (returns 0 if k is
	/// unknown)
	/// @param key
	/// @return value
	T* find(K key) {
		typename std::map<K, T*>::iterator iterator;
		T* result;

		rina::AccessGuard(*this);
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

		rina::AccessGuard(*this);
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
	std::list<T*> getEntries() {
		typename std::map<K, T*>::iterator iterator;
		std::list<T*> result;

		rina::AccessGuard(*this);
		for(iterator = map.begin();
				iterator != map.end(); ++iterator){
			result.push_back(iterator->second);
		}

		return result;
	}

	/// Delete all the values of the map
	void deleteValues() {
		typename std::map<K, T*>::iterator iterator;

		rina::AccessGuard(*this);
		for(iterator = map.begin();
				iterator != map.end(); ++iterator){
			delete iterator->second;
		}
	}

private:
	std::map<K, T*> map;
};


#endif

#endif

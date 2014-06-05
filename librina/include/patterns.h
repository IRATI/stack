//
// Patterns
//
//    Eduard Grasa          <eduard.grasa@i2cat.net>
//    Francesco Salvestrini <f.salvestrini@nextworks.it>
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

#ifndef LIBRINA_PATTERNS_H
#define LIBRINA_PATTERNS_H

#ifdef __cplusplus

#include "exceptions.h"

class NonCopyable {
public:
        NonCopyable()  { }
        ~NonCopyable() { }

private:
        NonCopyable(const NonCopyable &);
        const NonCopyable & operator =(const NonCopyable &);
};

template<typename TYPE> class Singleton : public NonCopyable {

public:
        Singleton()  { }
        ~Singleton() { fini(); }

        TYPE * operator->() {
                init();
                return instance_;
        }

private:
        static TYPE * instance_;

        void init() {
        	if (!instance_) {
        		instance_ = new TYPE();
        	}
        }

        void fini() {
                if (instance_) {
                        delete instance_;
                        instance_ = 0;
                }
        }
};

template<typename TYPE> TYPE * Singleton<TYPE>::instance_ = 0;

#endif

#endif

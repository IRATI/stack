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

template<typename TYPE> class singleton : public NonCopyable {
public:
        singleton()  { }
        ~singleton() { fini(); }

        TYPE * operator->() {
                init();
                return instance_;
        }

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

private:
        static TYPE * instance_;
};

template<typename TYPE> TYPE * singleton<TYPE>::instance_ = 0;

class LockableException : public Exception {
public:
        LockableException() { }
        LockableException(const std::string & s) : Exception(s) { }
};

class Lockable : public NonCopyable {
public:
        Lockable();
        virtual ~Lockable() throw();

        virtual void lock();
        virtual void unlock();

private:
        // FIXME: Add proper mutexes here
        //pthread_mutex_t     mutex_;
        //pthread_mutexattr_t mutex_attr_;
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
                        LCRT("Cannot unlock guarded access");
                }
        }

private:
        Lockable & guarded_;
};

#endif

#endif

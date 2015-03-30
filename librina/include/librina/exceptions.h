/*
 * Exceptions
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

#ifndef LIBRINA_EXCEPTIONS_H
#define LIBRINA_EXCEPTIONS_H

/**
* @file exceptions.h
*
* @brief A set of classes to simplify exception handling
*/


#ifdef __cplusplus

#include <stdexcept>

/**
* Exception base class
*/
namespace rina {

class Exception : public std::exception {
public:
        Exception() { }
        Exception(const char* s) { description_ = s; }

        virtual ~Exception() throw() { }

        virtual const char * what() const throw();

private:
        std::string description_;
};

/**
* Utility MACRO to define a (simple) derived exception class
*/
#define DECLARE_EXCEPTION_SUBCLASS(TYPE)\
class TYPE : public rina::Exception {\
public:\
        TYPE () { };\
        TYPE (const char* s) : Exception(s) {};\
        virtual ~ TYPE() throw() { };\
}

}//namespace rina

#endif //__cplusplus

#endif //LIBRINA_EXCEPTIONS_H

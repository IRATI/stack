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

#ifdef __cplusplus

#include <stdexcept>

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

}

#endif

#endif

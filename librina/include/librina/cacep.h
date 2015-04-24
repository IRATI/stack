/*
 * Common Application Connection Establishment phase
 *
 *    Eduard Grasa          <eduard.grasa@i2cat.net>
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

#ifndef LIBRINA_CACEP_H
#define LIBRINA_CACEP_H

#ifdef __cplusplus

#include "librina/application.h"
#include "librina/cdap.h"

namespace rina {

class IAuthPolicySet : IPolicySet {
public:
	virtual ~IAuthPolicySet() {};
};

class CACEPHandler {
public:
        virtual ~CACEPHandler(){};

        /// A remote IPC process Connect request has been received.
        /// @param invoke_id the id of the connect message
        /// @param session_descriptor
        virtual void connect(int invoke_id,
                        rina::CDAPSessionDescriptor * session_descriptor) = 0;

        /// A remote IPC process Connect response has been received.
        /// @param result
        /// @param result_reason
        /// @param session_descriptor
        virtual void connectResponse(int result, const std::string& result_reason,
                        rina::CDAPSessionDescriptor * session_descriptor) = 0;

        /// A remote IPC process Release request has been received.
        /// @param invoke_id the id of the release message
        /// @param session_descriptor
        virtual void release(int invoke_id,
                        rina::CDAPSessionDescriptor * session_descriptor) = 0;

        /// A remote IPC process Release response has been received.
        /// @param result
        /// @param result_reason
        /// @param session_descriptor
        virtual void releaseResponse(int result, const std::string& result_reason,
                        rina::CDAPSessionDescriptor * session_descriptor) = 0;

        /// The authentication policy set
        IAuthPolicySet * ps;
};


}

#endif

#endif

/*
 * RIB and CDAP common structures
 *
 *    Bernat Gaston <bernat.gaston@i2cat.net>
 *
 * This library is free software{} you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation{} either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY{} without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library{} if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

#ifndef CDAP_RIB_STRUCTURES_H_
#define CDAP_RIB_STRUCTURES_H_

#include <string>
#include <list>

namespace rina {
namespace cdap_rib {

typedef struct SerializedObject {
	int size_;
	void* message_;
} ser_obj_t;

typedef struct cdap_params {
	long timeout_;
	bool is_IPCP_;
} cdap_params_t;

/// Authentication information
typedef struct auth_policy {
	/// Policy name
	std::string name;
	/// Supported versions
	std::list<std::string> versions;
	/// Policy-specific options, encoded in a char array
	SerializedObject options;
} auth_policy_t;

// End-point information
typedef struct ep_info {
	/// Application-Entity-Instance-Id (string), optional, not validated by CDAP.
	std::string ae_inst_;
	/// Application-Entity-Name (string), mandatory (optional for the response).
	std::string ae_name_;
	/// Application-Process-Instance-Id (string), optional, not validated by CDAP.
	std::string ap_inst_;
	/// Application-Process-Name (string), mandatory (optional for the response).
	std::string ap_name_;
} ep_info_t;


typedef struct flags {
	enum Flags {
		NONE_FLAGS,
		F_SYNC,
		F_RD_INCOMPLETE
	};
	/// flags (enm, int32), conditional, may be required by CDAP.
	/// set_ of Boolean values that modify the meaning of a
	/// message in a uniform way when true.
	Flags flags_;
} flags_t;

typedef struct object_info {
	/// ObjectClass (string). Identifies the object class definition of the
	/// addressed object.
	std::string class_;
	/// ObjectInstance (int64). Object instance uniquely identifies a single object
	/// with a specific ObjectClass and ObjectName in an application's RIB. Either
	/// the ObjectClass and ObjectName or this value may be used, if the ObjectInstance
	/// value is known. If a class and name are supplied in an operation,
	/// an ObjectInstance value may be returned, and that may be used in future operations
	/// in lieu of obj_class and obj_name for the duration of this connection.
	long inst_;
	/// ObjectName (string). Identifies a named object that the operation is
	/// to be applied to. Object names identify a unique object of the designated
	/// ObjectClass within an application.
	std::string name_;
	/// ObjectValueInterface (ObjectValueInterface). The value of the object.
	ser_obj_t value_;
	/// Result (int32). Mandatory in the responses, forbidden in the requests
	/// The result of an operation, indicating its success (which has the value zero,
	/// the default for this field), partial success in the case of
	/// synchronized operations, or reason for failure
} obj_info_t;

typedef struct filtering_info {
	/// Filter (bytes). Filter predicate function to be used to determine whether an operation
	/// is to be applied to the designated object (s).
	char* filter_;
	/// Scope (int32). Specifies the depth of the object tree at
	/// the destination application to which an operation (subject to filtering)
	/// is to apply (if missing or present and having the value 0, the default,
	/// only the target_ed application's objects are addressed)
	int scope_;
} filt_info_t;

typedef enum{
	/// The operation was successful
	CDAP_SUCCESS = 0,
	/// The operation has not been been completed yet
	CDAP_PENDING = 1,

	/* List of errors */

	/// Unknown error
	CDAP_ERROR = -1,
	/// Object does not exist
	CDAP_INVALID_OBJ = -2,
	/// Object class does not match
	CDAP_INVALID_OBJ_CLASS = -3,
	/// Operation not supported in this object
	CDAP_OP_NOT_SUPPORTED = -4,
	/// Error serializing/deserializing object value
	CDAP_SER_DES_ERRORSUPPORTED = -5,
	/// 
}res_code_t;

typedef struct result_info {
	/// Result code of the operation
	res_code_t code_;

	/// Result-Reason (string), optional in the responses, forbidden in the requests
	/// Additional explanation of the result_
	std::string reason_;
} res_info_t;

typedef struct version_info {
	/// Version (int32). Mandatory in connect request and response, optional otherwise.
	/// Version of RIB and object set_ to use in the conversation. Note that the
	/// abstract syntax refers to the CDAP message syntax, while version refers to the
	/// version of the AE RIB objects, their values, vocabulary of object id's, and
	/// related behaviors that are subject to change over time. See text for details
	/// of use.
	long version_;

} vers_info_t;

typedef struct connection_handler {
	int port_;
	ep_info_t src_;
	ep_info_t dest_;
	auth_policy_t auth_;
	vers_info_t version_;
} con_handle_t;

} //cdap_rib namespace
} //rina namespace
#endif /* CDAP_RIB_STRUCTURES_H_ */

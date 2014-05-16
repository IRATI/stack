/*
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

#include "librina-cdap.h"
#include "exceptions.h"

namespace rina {

/* CLASS AuthValue      */

AuthValue::AuthValue() {
	auth_other = NULL;
}

std::string AuthValue::get_auth_name() const {
	return auth_name;
}

void AuthValue::set_auth_name(std::string arg0) {
	auth_name = arg0;
}

std::string AuthValue::get_auth_password() const {
	return auth_password;
}

void AuthValue::set_auth_password(std::string arg0) {
	auth_password = arg0;
}

char* AuthValue::get_auth_other() const {
	return auth_other;
}

void AuthValue::set_auth_other(char* arg0) {
	auth_other = arg0;
}

/* CLASS ObjectValue */
ObjectValue::ObjectValue() {
	intval = 0;
	sintval = 0;
	int64val = 0;
	sint64val = 0;
	byteval = NULL;
	floatval = 0;
	doubleval = 0;
	booleanval = false;
}

int ObjectValue::get_intval() const {
	return intval;
}

void ObjectValue::set_intval(int arg0) {
	intval = arg0;
}

int ObjectValue::get_sintval() const {
	return sintval;
}

void ObjectValue::set_sintval(int arg0) {
	sintval = arg0;
}

long ObjectValue::get_int64val() const {
	return int64val;
}

void ObjectValue::set_int64val(long arg0) {
	int64val = arg0;
}

long ObjectValue::get_sint64val() const {
	return sint64val;
}

void ObjectValue::set_sint64val(long arg0) {
	sint64val = arg0;
}

std::string ObjectValue::get_strval() const {
	return strval;
}

void ObjectValue::set_strval(std::string arg0) {
	strval = arg0;
}

char* ObjectValue::get_byteval() const {
	return byteval;
}

void ObjectValue::set_byteval(char arg0[]) {
	byteval = arg0;
}

int ObjectValue::get_floatval() const {
	return floatval;
}

void ObjectValue::set_floatval(int arg0) {
	floatval = arg0;
}

long ObjectValue::get_doubleval() const {
	return doubleval;
}

void ObjectValue::set_doubleval(long arg0) {
	doubleval = arg0;
}

void ObjectValue::set_booleanval(bool arg0) {
	booleanval = arg0;
}

bool ObjectValue::is_booleanval() const {
	return booleanval;

}

bool ObjectValue::operator==(const ObjectValue &other) const {
	if (intval == other.get_intval() && sintval == other.get_sintval()
			&& int64val == other.get_int64val()
			&& sint64val == other.get_sint64val()
			&& byteval == other.get_byteval()
			&& floatval == other.get_floatval()
			&& doubleval == other.get_doubleval()
			&& booleanval == other.is_booleanval())
		return true;
	else
		return false;

}
/* CLASS CDAPMessageValidator */
void CDAPMessageValidator::validate(const CDAPMessage &message)
		throw (CDAPException) {
	validateAbsSyntax(message);
	validateAuthMech(message);
	validateAuthValue(message);
	validateDestAEInst(message);
	validateDestAEName(message);
	validateDestApInst(message);
	validateDestApName(message);
	validateFilter(message);
	validateInvokeID(message);
	validateObjClass(message);
	validateObjInst(message);
	validateObjName(message);
	validateObjValue(message);
	validateOpcode(message);
	validateResult(message);
	validateResultReason(message);
	validateScope(message);
	validateSrcAEInst(message);
	validateSrcAEName(message);
	validateSrcApInst(message);
	validateSrcApName(message);
	validateVersion(message);
}

void CDAPMessageValidator::validateAbsSyntax(const CDAPMessage &message)
		throw (CDAPException) {
	if (message.get_abs_syntax() == 0) {
		if (message.get_op_code() == CDAPMessage::M_CONNECT
				|| message.get_op_code() == CDAPMessage::M_CONNECT_R) {
			throw CDAPException(
					std::string(
							"AbsSyntax must be set for M_CONNECT and M_CONNECT_R messages"),
					message);
		}
	} else {
		if ((message.get_op_code() != CDAPMessage::M_CONNECT)
				&& (message.get_op_code() != CDAPMessage::M_CONNECT_R)) {
			throw CDAPException(
					"AbsSyntax can only be set for M_CONNECT and M_CONNECT_R messages",
					message);
		}
	}
}

void CDAPMessageValidator::validateAuthMech(const CDAPMessage &message)
		throw (CDAPException) {
	if (message.get_auth_mech() != CDAPMessage::AUTH_NONE) {
		if ((message.get_op_code() != CDAPMessage::M_CONNECT)
				&& message.get_op_code() != CDAPMessage::M_CONNECT_R) {
			throw CDAPException(
					std::string(
							"AuthMech can only be set for M_CONNECT and M_CONNECT_R messages"),
					message);
		}
	}
}

void CDAPMessageValidator::validateAuthValue(const CDAPMessage &message)
		throw (CDAPException) {
	if ((message.get_op_code() != CDAPMessage::M_CONNECT)
			&& (message.get_op_code() != CDAPMessage::M_CONNECT_R)) {
		throw CDAPException(
				std::string(
						"AuthValue can only be set for M_CONNECT and M_CONNECT_R messages"),
				message);
	}
}

void CDAPMessageValidator::validateDestAEInst(const CDAPMessage &message)
		throw (CDAPException) {
	if (!message.get_dest_ae_inst().empty()) {
		if ((message.get_op_code() != CDAPMessage::M_CONNECT)
				&& (message.get_op_code() != CDAPMessage::M_CONNECT_R)) {
			throw CDAPException(
					std::string(
							"dest_ae_inst can only be set for M_CONNECT and M_CONNECT_R messages"),
					message);
		}
	}
}

void CDAPMessageValidator::validateDestAEName(const CDAPMessage &message)
		throw (CDAPException) {
	if (!message.get_dest_ae_name().empty()) {
		if ((message.get_op_code() != CDAPMessage::M_CONNECT)
				&& (message.get_op_code() != CDAPMessage::M_CONNECT_R)) {
			throw CDAPException(
					std::string(
							"DestAEName can only be set for M_CONNECT and M_CONNECT_R messages"),
					message);
		}
	}
}

void CDAPMessageValidator::validateDestApInst(const CDAPMessage &message)
		throw (CDAPException) {
	if (!message.get_dest_ap_inst().empty()) {
		if (message.get_op_code() != CDAPMessage::M_CONNECT
				&& message.get_op_code() != CDAPMessage::M_CONNECT_R) {
			throw CDAPException(
					std::string(
							"DestApInst can only be set for M_CONNECT and M_CONNECT_R messages"),
					message);
		}
	}
}

void CDAPMessageValidator::validateDestApName(const CDAPMessage &message)
		throw (CDAPException) {
	if (!message.get_dest_ap_name().empty()) {
		if (message.get_op_code() == CDAPMessage::M_CONNECT) {
			throw CDAPException(
					std::string(
							"DestApName must be set for the M_CONNECT message"),
					message);
		} else if (message.get_op_code() == CDAPMessage::M_CONNECT_R) {
			//TODO not sure what to do
		}
	} else {
		if (message.get_op_code() != CDAPMessage::M_CONNECT
				&& message.get_op_code() != CDAPMessage::M_CONNECT_R) {
			throw CDAPException(
					std::string(
							"DestApName can only be set for M_CONNECT and M_CONNECT_R messages"),
					message);
		}
	}
}

void CDAPMessageValidator::validateFilter(const CDAPMessage &message)
		throw (CDAPException) {
	if (message.get_filter() != NULL) {
		if (message.get_op_code() != CDAPMessage::M_CREATE
				&& message.get_op_code() != CDAPMessage::M_DELETE
				&& message.get_op_code() != CDAPMessage::M_READ
				&& message.get_op_code() != CDAPMessage::M_WRITE
				&& message.get_op_code() != CDAPMessage::M_START
				&& message.get_op_code() != CDAPMessage::M_STOP) {
			throw CDAPException(
					std::string(
							"The filter parameter can only be set for M_CREATE, M_DELETE, M_READ, M_WRITE, M_START or M_STOP messages"),
					message);
		}
	}
}

void CDAPMessageValidator::validateInvokeID(const CDAPMessage &message)
		throw (CDAPException) {
	if (message.get_invoke_id() == 0) {
		if (message.get_op_code() == CDAPMessage::M_CONNECT
				|| message.get_op_code() == CDAPMessage::M_CONNECT_R
				|| message.get_op_code() == CDAPMessage::M_RELEASE_R
				|| message.get_op_code() == CDAPMessage::M_CREATE_R
				|| message.get_op_code() == CDAPMessage::M_DELETE_R
				|| message.get_op_code() == CDAPMessage::M_READ_R
				|| message.get_op_code() == CDAPMessage::M_CANCELREAD
				|| message.get_op_code() == CDAPMessage::M_CANCELREAD_R
				|| message.get_op_code() == CDAPMessage::M_WRITE_R
				|| message.get_op_code() == CDAPMessage::M_START_R
				|| message.get_op_code() == CDAPMessage::M_STOP_R) {
			throw CDAPException(
					std::string("The invoke id parameter cannot be 0"),
					message);
		}
	}
}

void CDAPMessageValidator::validateObjClass(const CDAPMessage &message)
		throw (CDAPException) {
	if (!message.get_obj_class().empty()) {
		if (!message.get_obj_name().empty()) {
			throw CDAPException(
					"If the objClass parameter is set, the objName parameter also has to be set",
					message);
		}
		if (message.get_op_code() != CDAPMessage::M_CREATE
				&& message.get_op_code() != CDAPMessage::M_CREATE_R
				&& message.get_op_code() != CDAPMessage::M_DELETE
				&& message.get_op_code() != CDAPMessage::M_DELETE_R
				&& message.get_op_code() != CDAPMessage::M_READ
				&& message.get_op_code() != CDAPMessage::M_READ_R
				&& message.get_op_code() != CDAPMessage::M_WRITE
				&& message.get_op_code() != CDAPMessage::M_WRITE_R
				&& message.get_op_code() != CDAPMessage::M_START
				&& message.get_op_code() != CDAPMessage::M_STOP
				&& message.get_op_code() != CDAPMessage::M_START_R
				&& message.get_op_code() != CDAPMessage::M_STOP_R) {
			throw CDAPException(
					std::string(
							"The objClass parameter can only be set for M_CREATE, M_CREATE_R, M_DELETE, M_DELETE_R, M_READ, M_READ_R, M_WRITE, M_WRITE_R, M_START, M_STOP, M_START_R, M_STOP_R messages"),
					message);
		}
	}
}

void CDAPMessageValidator::validateObjInst(const CDAPMessage &message)
		throw (CDAPException) {
	if (message.get_obj_inst() != 0) {
		if (message.get_op_code() != CDAPMessage::M_CREATE
				&& message.get_op_code() != CDAPMessage::M_CREATE_R
				&& message.get_op_code() != CDAPMessage::M_DELETE
				&& message.get_op_code() != CDAPMessage::M_DELETE_R
				&& message.get_op_code() != CDAPMessage::M_READ
				&& message.get_op_code() != CDAPMessage::M_READ_R
				&& message.get_op_code() != CDAPMessage::M_WRITE
				&& message.get_op_code() != CDAPMessage::M_WRITE_R
				&& message.get_op_code() != CDAPMessage::M_START
				&& message.get_op_code() != CDAPMessage::M_STOP
				&& message.get_op_code() != CDAPMessage::M_START_R
				&& message.get_op_code() != CDAPMessage::M_STOP_R) {
			throw CDAPException(
					std::string(
							"The objInst parameter can only be set for M_CREATE, M_CREATE_R, M_DELETE, M_DELETE_R, M_READ, M_READ_R, M_WRITE, M_WRITE_R, M_START, M_START_R, M_STOP and M_STOP_R messages"),
					message);
		}
	}
}

void CDAPMessageValidator::validateObjName(const CDAPMessage &message)
		throw (CDAPException) {
	if (!message.get_obj_name().empty()) {
		if (message.get_obj_class().empty()) {
			throw new CDAPException(
					std::string(
							"If the objName parameter is set, the objClass parameter also has to be set"));
		}
		if (message.get_op_code() != CDAPMessage::M_CREATE
				&& message.get_op_code() != CDAPMessage::M_CREATE_R
				&& message.get_op_code() != CDAPMessage::M_DELETE
				&& message.get_op_code() != CDAPMessage::M_DELETE_R
				&& message.get_op_code() != CDAPMessage::M_READ
				&& message.get_op_code() != CDAPMessage::M_READ_R
				&& message.get_op_code() != CDAPMessage::M_WRITE
				&& message.get_op_code() != CDAPMessage::M_WRITE_R
				&& message.get_op_code() != CDAPMessage::M_START
				&& message.get_op_code() != CDAPMessage::M_STOP
				&& message.get_op_code() != CDAPMessage::M_START_R
				&& message.get_op_code() != CDAPMessage::M_STOP_R) {
			throw CDAPException(
					std::string(
							"The objName parameter can only be set for M_CREATE, M_CREATE_R, M_DELETE, M_DELETE_R, M_READ, M_READ_R, M_WRITE, M_WRITE_R, M_START, M_STOP, M_START_R and M_STOP_R messages"),
					message);
		}
	}
}

void CDAPMessageValidator::validateObjValue(const CDAPMessage &message)
		throw (CDAPException) {
	/*FIXME: See if this can be done in a better way	*/
	ObjectValue emptyObj;
	if (message.get_obj_value() == emptyObj) {
		if (message.get_op_code() == CDAPMessage::M_WRITE) {
			throw CDAPException(
					std::string(
							"The objValue parameter must be set for M_WRITE messages"),
					message);
		}
	} else {
		if (message.get_op_code() != CDAPMessage::M_CREATE
				&& message.get_op_code() != CDAPMessage::M_CREATE_R
				&& message.get_op_code() != CDAPMessage::M_READ_R
				&& message.get_op_code() != CDAPMessage::M_WRITE
				&& message.get_op_code() != CDAPMessage::M_START
				&& message.get_op_code() != CDAPMessage::M_STOP
				&& message.get_op_code() != CDAPMessage::M_START_R
				&& message.get_op_code() != CDAPMessage::M_STOP_R
				&& message.get_op_code() != CDAPMessage::M_WRITE_R
				&& message.get_op_code() != CDAPMessage::M_DELETE
				&& message.get_op_code() != CDAPMessage::M_READ) {
			throw CDAPException(
					std::string(
							"The objValue parameter can only be set for M_CREATE, M_DELETE, M_CREATE_R, M_READ_R, M_WRITE, M_START, M_START_R, M_STOP, M_STOP_R and M_WRITE_R messages"),
					message);
		}
	}
}

void CDAPMessageValidator::validateOpcode(const CDAPMessage &message)
		throw (CDAPException) {
	if (message.get_op_code() == CDAPMessage::NONE_OPCODE) {
		throw CDAPException(
				std::string("The opcode must be set for all the messages"),
				message);
	}
}

void CDAPMessageValidator::validateResult(const CDAPMessage &message)
		throw (CDAPException) {
	/* FIXME: Do something with sense */
	message.get_abs_syntax();
}

void CDAPMessageValidator::validateResultReason(const CDAPMessage &message)
		throw (CDAPException) {
	if (!message.get_result_reason().empty()) {
		if (message.get_op_code() != CDAPMessage::M_CREATE_R
				&& message.get_op_code() != CDAPMessage::M_DELETE_R
				&& message.get_op_code() != CDAPMessage::M_READ_R
				&& message.get_op_code() != CDAPMessage::M_WRITE_R
				&& message.get_op_code() != CDAPMessage::M_CONNECT_R
				&& message.get_op_code() != CDAPMessage::M_RELEASE_R
				&& message.get_op_code() != CDAPMessage::M_CANCELREAD
				&& message.get_op_code() != CDAPMessage::M_CANCELREAD_R
				&& message.get_op_code() != CDAPMessage::M_START_R
				&& message.get_op_code() != CDAPMessage::M_STOP_R) {
			throw CDAPException(
					std::string(
							"The resultReason parameter can only be set for M_CREATE_R, M_DELETE_R, M_READ_R, M_WRITE_R, M_START_R, M_STOP_R, M_CONNECT_R, M_RELEASE_R, M_CANCELREAD and M_CANCELREAD_R messages"),
					message);
		}
	}
}

void CDAPMessageValidator::validateScope(const CDAPMessage &message)
		throw (CDAPException) {
	if (message.get_scope() != 0) {
		if (message.get_op_code() != CDAPMessage::M_CREATE
				&& message.get_op_code() != CDAPMessage::M_DELETE
				&& message.get_op_code() != CDAPMessage::M_READ
				&& message.get_op_code() != CDAPMessage::M_WRITE
				&& message.get_op_code() != CDAPMessage::M_START
				&& message.get_op_code() != CDAPMessage::M_STOP) {
			throw CDAPException(
					std::string(
							"The scope parameter can only be set for M_CREATE, M_DELETE, M_READ, M_WRITE, M_START or M_STOP messages"),
					message);
		}
	}
}

void CDAPMessageValidator::validateSrcAEInst(const CDAPMessage &message)
		throw (CDAPException) {
	if (!message.get_src_ae_inst().empty()) {
		if (message.get_op_code() != CDAPMessage::M_CONNECT
				&& message.get_op_code() != CDAPMessage::M_CONNECT_R) {
			throw CDAPException(
					std::string(
							"SrcAEInst can only be set for M_CONNECT and M_CONNECT_R messages"),
					message);
		}
	}
}

void CDAPMessageValidator::validateSrcAEName(const CDAPMessage &message)
		throw (CDAPException) {
	if (!message.get_src_ae_name().empty()) {

	}
	if (message.get_op_code() != CDAPMessage::M_CONNECT
			&& message.get_op_code() != CDAPMessage::M_CONNECT_R) {
		throw CDAPException(
				std::string(
						"SrcAEName can only be set for M_CONNECT and M_CONNECT_R messages"),
				message);
	}
}

void CDAPMessageValidator::validateSrcApInst(const CDAPMessage &message)
		throw (CDAPException) {
	if (message.get_src_ap_inst().empty()) {
		if (message.get_op_code() != CDAPMessage::M_CONNECT
				&& message.get_op_code() != CDAPMessage::M_CONNECT_R) {
			throw CDAPException(
					std::string(
							"SrcApInst can only be set for M_CONNECT and M_CONNECT_R messages"),
					message);
		}
	}
}

void CDAPMessageValidator::validateSrcApName(const CDAPMessage &message)
		throw (CDAPException) {
	if (message.get_src_ap_name().empty()) {
		if (message.get_op_code() == CDAPMessage::M_CONNECT) {
			throw CDAPException(
					std::string(
							"SrcApName must be set for the M_CONNECT message"),
					message);
		} else if (message.get_op_code() == CDAPMessage::M_CONNECT_R) {
			//TODO not sure what to do
		}
	} else {
		if (message.get_op_code() != CDAPMessage::M_CONNECT
				&& message.get_op_code() != CDAPMessage::M_CONNECT_R) {
			throw CDAPException(
					std::string(
							"SrcApName can only be set for M_CONNECT and M_CONNECT_R messages"),
					message);
		}
	}
}

void CDAPMessageValidator::validateVersion(const CDAPMessage &message)
		throw (CDAPException) {
	if (message.get_version() == 0) {
		if (message.get_op_code() == CDAPMessage::M_CONNECT
				|| message.get_op_code() == CDAPMessage::M_CONNECT_R) {
			throw CDAPException(
					std::string(
							"Version must be set for M_CONNECT and M_CONNECT_R messages"),
					message);
		}
	}
}

/* CLASS CDAPMessage */
const int CDAPMessage::ABSTRACT_SYNTAX_VERSION = 0x0073;
CDAPMessage::CDAPMessage() {
	abs_syntax = 0;
	auth_mech = AUTH_NONE;
	filter = NULL;
	flags = NONE_FLAGS;
	invoke_id = 0;
	obj_inst = 0;
	op_code = NONE_OPCODE;
	result = 0;
	scope = 0;
	version = 0;
}

void CDAPMessage::getOpenConnectionRequestMessage(CDAPMessage &cdapMessage,
		AuthTypes authMech, const AuthValue &authValue, std::string dest_ae_inst,
		std::string destAEName, std::string destApInst, std::string destApName,
		std::string srcAEInst, std::string srcAEName, std::string srcApInst,
		std::string srcApName, int invokeID) throw (CDAPException) {
	cdapMessage.set_abs_syntax(ABSTRACT_SYNTAX_VERSION);
	cdapMessage.set_auth_mech(authMech);
	cdapMessage.set_auth_value(authValue);
	cdapMessage.set_dest_ae_inst(dest_ae_inst);
	cdapMessage.set_dest_ae_name(destAEName);
	cdapMessage.set_dest_ap_inst(destApInst);
	cdapMessage.set_dest_ap_inst(destApInst);
	cdapMessage.set_dest_ap_name(destApName);
	cdapMessage.set_invoke_id(invokeID);
	cdapMessage.set_op_code(M_CONNECT);
	cdapMessage.set_src_ae_inst(srcAEInst);
	cdapMessage.set_src_ae_name(srcAEName);
	cdapMessage.set_src_ap_inst(srcApInst);
	cdapMessage.set_src_ap_name(srcApName);
	cdapMessage.set_version(1);
	CDAPMessageValidator::validate(cdapMessage);

}

void CDAPMessage::getOpenConnectionResponseMessage(CDAPMessage &cdapMessage,
		AuthTypes authMech, const AuthValue &authValue, std::string dest_ae_inst,
		std::string destAEName, std::string destApInst, std::string destApName,
		int result, std::string resultReason, std::string srcAEInst,
		std::string srcAEName, std::string srcApInst, std::string srcApName,
		int invokeID) throw (CDAPException) {
	cdapMessage.set_abs_syntax(ABSTRACT_SYNTAX_VERSION);
	cdapMessage.set_auth_mech(authMech);
	cdapMessage.set_auth_value(authValue);
	cdapMessage.set_dest_ae_inst(dest_ae_inst);
	cdapMessage.set_dest_ae_name(destAEName);
	cdapMessage.set_dest_ap_inst(destApInst);
	cdapMessage.set_dest_ap_inst(destApInst);
	cdapMessage.set_dest_ap_name(destApName);
	cdapMessage.set_op_code(M_CONNECT_R);
	cdapMessage.set_invoke_id(invokeID);
	cdapMessage.set_result(result);
	cdapMessage.set_result_reason(resultReason);
	cdapMessage.set_src_ae_inst(srcAEInst);
	cdapMessage.set_src_ae_name(srcAEName);
	cdapMessage.set_src_ap_inst(srcApInst);
	cdapMessage.set_src_ap_name(srcApName);
	cdapMessage.set_version(1);
	CDAPMessageValidator::validate(cdapMessage);

}

void CDAPMessage::getReleaseConnectionRequestMessage(CDAPMessage &cdapMessage,
		Flags flags) throw (CDAPException) {
	cdapMessage.set_flags(flags);
	cdapMessage.set_op_code(M_RELEASE);
	CDAPMessageValidator::validate(cdapMessage);

}

void CDAPMessage::getReleaseConnectionResponseMessage(CDAPMessage &cdapMessage,
		Flags flags, int result, std::string resultReason, int invokeID)
				throw (CDAPException) {
	cdapMessage.set_flags(flags);
	cdapMessage.set_invoke_id(invokeID);
	cdapMessage.set_op_code(M_RELEASE_R);
	cdapMessage.set_result(result);
	cdapMessage.set_result_reason(resultReason);
	CDAPMessageValidator::validate(cdapMessage);

}

void CDAPMessage::getCreateObjectRequestMessage(CDAPMessage &cdapMessage,
		char filter[], Flags flags, std::string objClass, long objInst,
		std::string objName, const ObjectValue &objValue, int scope)
				throw (CDAPException) {
	cdapMessage.set_filter(filter);
	cdapMessage.set_flags(flags);
	cdapMessage.set_obj_class(objClass);
	cdapMessage.set_obj_inst(objInst);
	cdapMessage.set_obj_name(objName);
	cdapMessage.set_obj_value(objValue);
	cdapMessage.set_op_code(M_CREATE);
	cdapMessage.set_scope(scope);
	CDAPMessageValidator::validate(cdapMessage);

}

void CDAPMessage::getCreateObjectResponseMessage(CDAPMessage &cdapMessage,
		Flags flags, std::string objClass, long objInst, std::string objName,
		const ObjectValue &objValue, int result, std::string resultReason,
		int invokeID) throw (CDAPException) {
	cdapMessage.set_flags(flags);
	cdapMessage.set_invoke_id(invokeID);
	cdapMessage.set_obj_class(objClass);
	cdapMessage.set_obj_inst(objInst);
	cdapMessage.set_obj_name(objName);
	cdapMessage.set_obj_value(objValue);
	cdapMessage.set_op_code(M_CREATE_R);
	cdapMessage.set_result(result);
	cdapMessage.set_result_reason(resultReason);
	CDAPMessageValidator::validate(cdapMessage);

}

void CDAPMessage::getDeleteObjectRequestMessage(CDAPMessage &cdapMessage,
		char filter[], Flags flags, std::string objClass, long objInst,
		std::string objName, const ObjectValue &objectValue, int scope)
				throw (CDAPException) {
	cdapMessage.set_filter(filter);
	cdapMessage.set_flags(flags);
	cdapMessage.set_obj_class(objClass);
	cdapMessage.set_obj_inst(objInst);
	cdapMessage.set_obj_name(objName);
	cdapMessage.set_obj_value(objectValue);
	cdapMessage.set_op_code(M_DELETE);
	cdapMessage.set_scope(scope);
	CDAPMessageValidator::validate(cdapMessage);

}
void CDAPMessage::getDeleteObjectResponseMessage(CDAPMessage &cdapMessage,
		Flags flags, std::string objClass, long objInst, std::string objName,
		int result, std::string resultReason, int invokeID)
				throw (CDAPException) {
	cdapMessage.set_flags(flags);
	cdapMessage.set_invoke_id(invokeID);
	cdapMessage.set_obj_class(objClass);
	cdapMessage.set_obj_inst(objInst);
	cdapMessage.set_obj_name(objName);
	cdapMessage.set_op_code(M_DELETE_R);
	cdapMessage.set_result(result);
	cdapMessage.set_result_reason(resultReason);
	CDAPMessageValidator::validate(cdapMessage);

}
void CDAPMessage::getStartObjectRequestMessage(CDAPMessage &cdapMessage,
		char filter[], Flags flags, std::string objClass,
		const ObjectValue &objValue, long objInst, std::string objName,
		int scope) throw (CDAPException) {
	cdapMessage.set_filter(filter);
	cdapMessage.set_flags(flags);
	cdapMessage.set_obj_class(objClass);
	cdapMessage.set_obj_inst(objInst);
	cdapMessage.set_obj_name(objName);
	cdapMessage.set_obj_value(objValue);
	cdapMessage.set_op_code(M_START);
	cdapMessage.set_scope(scope);
	CDAPMessageValidator::validate(cdapMessage);

}
void CDAPMessage::getStartObjectResponseMessage(CDAPMessage &cdapMessage,
		Flags flags, int result, std::string resultReason, int invokeID)
				throw (CDAPException) {
	cdapMessage.set_flags(flags);
	cdapMessage.set_invoke_id(invokeID);
	cdapMessage.set_op_code(M_START_R);
	cdapMessage.set_result(result);
	cdapMessage.set_result_reason(resultReason);
	CDAPMessageValidator::validate(cdapMessage);

}
void CDAPMessage::getStartObjectResponseMessage(CDAPMessage &cdapMessage,
		Flags flags, std::string objectClass, const ObjectValue &objectValue,
		long objInst, std::string objName, int result, std::string resultReason,
		int invokeID) throw (CDAPException) {
	cdapMessage.set_flags(flags);
	cdapMessage.set_invoke_id(invokeID);
	cdapMessage.set_op_code(M_START_R);
	cdapMessage.set_obj_class(objectClass);
	cdapMessage.set_obj_inst(objInst);
	cdapMessage.set_obj_name(objName);
	cdapMessage.set_obj_value(objectValue);
	cdapMessage.set_result(result);
	cdapMessage.set_result_reason(resultReason);
	CDAPMessageValidator::validate(cdapMessage);
}
void CDAPMessage::getStopObjectRequestMessage(CDAPMessage &cdapMessage,
		char filter[], Flags flags, std::string objClass,
		const ObjectValue &objValue, long objInst, std::string objName,
		int scope) throw (CDAPException) {
	cdapMessage.set_filter(filter);
	cdapMessage.set_flags(flags);
	cdapMessage.set_obj_class(objClass);
	cdapMessage.set_obj_inst(objInst);
	cdapMessage.set_obj_name(objName);
	cdapMessage.set_obj_value(objValue);
	cdapMessage.set_op_code(M_STOP);
	cdapMessage.set_scope(scope);
	CDAPMessageValidator::validate(cdapMessage);
}
void CDAPMessage::getStopObjectResponseMessage(CDAPMessage &cdapMessage,
		Flags flags, int result, std::string resultReason, int invokeID)
				throw (CDAPException) {
	cdapMessage.set_flags(flags);
	cdapMessage.set_invoke_id(invokeID);
	cdapMessage.set_op_code(M_STOP_R);
	cdapMessage.set_result(result);
	cdapMessage.set_result_reason(resultReason);
	CDAPMessageValidator::validate(cdapMessage);
}
void CDAPMessage::getReadObjectRequestMessage(CDAPMessage &cdapMessage,
		char filter[], Flags flags, std::string objClass, long objInst,
		std::string objName, int scope) throw (CDAPException) {
	cdapMessage.set_filter(filter);
	cdapMessage.set_flags(flags);
	cdapMessage.set_obj_class(objClass);
	cdapMessage.set_obj_inst(objInst);
	cdapMessage.set_obj_name(objName);
	cdapMessage.set_op_code(M_READ);
	cdapMessage.set_scope(scope);
	CDAPMessageValidator::validate(cdapMessage);
}
void CDAPMessage::getReadObjectResponseMessage(CDAPMessage &cdapMessage,
		Flags flags, std::string objClass, long objInst, std::string objName,
		const ObjectValue &objValue, int result, std::string resultReason,
		int invokeID) throw (CDAPException) {
	cdapMessage.set_flags(flags);
	cdapMessage.set_invoke_id(invokeID);
	cdapMessage.set_obj_class(objClass);
	cdapMessage.set_obj_inst(objInst);
	cdapMessage.set_obj_name(objName);
	cdapMessage.set_obj_value(objValue);
	cdapMessage.set_op_code(M_READ_R);
	cdapMessage.set_result(result);
	cdapMessage.set_result_reason(resultReason);
	CDAPMessageValidator::validate(cdapMessage);
}
void CDAPMessage::getWriteObjectRequestMessage(CDAPMessage &cdapMessage,
		char filter[], Flags flags, std::string objClass, long objInst,
		const ObjectValue &objValue, std::string objName, int scope)
				throw (CDAPException) {
	cdapMessage.set_filter(filter);
	cdapMessage.set_flags(flags);
	cdapMessage.set_obj_class(objClass);
	cdapMessage.set_obj_inst(objInst);
	cdapMessage.set_obj_name(objName);
	cdapMessage.set_obj_value(objValue);
	cdapMessage.set_op_code(M_WRITE);
	cdapMessage.set_scope(scope);
	CDAPMessageValidator::validate(cdapMessage);
}
void CDAPMessage::getWriteObjectResponseMessage(CDAPMessage &cdapMessage,
		Flags flags, int result, int invokeID, std::string resultReason)
				throw (CDAPException) {
	cdapMessage.set_flags(flags);
	cdapMessage.set_op_code(M_WRITE_R);
	cdapMessage.set_result(result);
	cdapMessage.set_result_reason(resultReason);
	cdapMessage.set_invoke_id(invokeID);
	CDAPMessageValidator::validate(cdapMessage);
}
void CDAPMessage::getCancelReadRequestMessage(CDAPMessage &cdapMessage,
		Flags flags, int invokeID) throw (CDAPException) {
	cdapMessage.set_flags(flags);
	cdapMessage.set_invoke_id(invokeID);
	cdapMessage.set_op_code(M_CANCELREAD);
	CDAPMessageValidator::validate(cdapMessage);
}
void CDAPMessage::getCancelReadResponseMessage(CDAPMessage &cdapMessage,
		Flags flags, int invokeID, int result, std::string resultReason)
				throw (CDAPException) {
	cdapMessage.set_flags(flags);
	cdapMessage.set_invoke_id(invokeID);
	cdapMessage.set_op_code(M_CANCELREAD_R);
	cdapMessage.set_result(result);
	cdapMessage.set_result_reason(resultReason);
	CDAPMessageValidator::validate(cdapMessage);
}

/*	TODO: Implement these functions	*/
int CDAPMessage::get_abs_syntax() const {
	return abs_syntax;
}

void CDAPMessage::set_abs_syntax(int arg0) {
	abs_syntax = arg0;
}

CDAPMessage::AuthTypes CDAPMessage::get_auth_mech() const {
	return auth_mech;
}

void CDAPMessage::set_auth_mech(AuthTypes arg0) {
	auth_mech = arg0;
}

const AuthValue& CDAPMessage::get_auth_value() const {
	return auth_value;
}

void CDAPMessage::set_auth_value(AuthValue arg0) {
	auth_value = arg0;
}

std::string CDAPMessage::get_dest_ae_inst() const {
	return dest_ae_inst;
}

void CDAPMessage::set_dest_ae_inst(std::string arg0) {
	dest_ae_inst = arg0;
}

std::string CDAPMessage::get_dest_ae_name() const {
	return dest_ae_name;
}

void CDAPMessage::set_dest_ae_name(std::string arg0) {
	dest_ae_name = arg0;
}

std::string CDAPMessage::get_dest_ap_inst() const {
	return dest_ap_inst;
}

void CDAPMessage::set_dest_ap_inst(std::string arg0) {
	dest_ap_inst = arg0;
}

std::string CDAPMessage::get_dest_ap_name() const {
	return dest_ap_name;
}

void CDAPMessage::set_dest_ap_name(std::string arg0) {
	dest_ap_name = arg0;
}

char* CDAPMessage::get_filter() const {
	return filter;
}

void CDAPMessage::set_filter(char arg0[]) {
	filter = arg0;
}

CDAPMessage::Flags CDAPMessage::get_flags() const {
	return flags;
}

void CDAPMessage::set_flags(Flags arg0) {
	flags = arg0;
}

int CDAPMessage::get_invoke_id() const {
	return invoke_id;
}

void CDAPMessage::set_invoke_id(int arg0) {
	invoke_id =  arg0;
}

std::string CDAPMessage::get_obj_class() const {
	return obj_class;
}

void CDAPMessage::set_obj_class(std::string arg0) {
	obj_class = arg0;
}

long CDAPMessage::get_obj_inst() const {
	return obj_inst;
}

void CDAPMessage::set_obj_inst(long arg0) {
	obj_inst = arg0;
}

std::string CDAPMessage::get_obj_name() const {
	return obj_name;
}

void CDAPMessage::set_obj_name(std::string arg0) {
	obj_name = arg0;
}

const ObjectValue& CDAPMessage::get_obj_value() const {
	return obj_value;
}

void CDAPMessage::set_obj_value(const ObjectValue &arg0) {
	obj_value = arg0;
}

CDAPMessage::Opcode CDAPMessage::get_op_code() const {
	return op_code;
}

void CDAPMessage::set_op_code(Opcode arg0) {
	op_code = arg0;
}

int CDAPMessage::get_result() const {
	return result;
}

void CDAPMessage::set_result(int arg0) {
	result = arg0;
}

std::string CDAPMessage::get_result_reason() const {
	return result_reason;
}

void CDAPMessage::set_result_reason(std::string arg0) {
	result_reason = arg0;
}

int CDAPMessage::get_scope() const {
	return scope;
}

void CDAPMessage::set_scope(int arg0) {
	scope = arg0;
}

std::string CDAPMessage::get_src_ae_inst() const {
	return src_ae_inst;
}

void CDAPMessage::set_src_ae_inst(std::string arg0) {
	src_ae_inst = arg0;
}

std::string CDAPMessage::get_src_ae_name() const {
	return src_ae_name;
}

void CDAPMessage::set_src_ae_name(std::string arg0) {
	src_ae_name = arg0;
}

std::string CDAPMessage::get_src_ap_inst() const {
	return src_ap_inst;
}

void CDAPMessage::set_src_ap_inst(std::string arg0) {
	src_ap_inst = arg0;
}

std::string CDAPMessage::get_src_ap_name() const {
	return src_ap_name;
}

void CDAPMessage::set_src_ap_name(std::string arg0) {
	src_ap_name = arg0;
}

long CDAPMessage::get_version() const {
	return version;
}

void CDAPMessage::set_version(long arg0) {
	version = arg0;
}

std::string toString() {
	return std::string("return");
}
}

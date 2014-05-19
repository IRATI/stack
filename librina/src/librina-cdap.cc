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

namespace rina {

// CLASS AuthValue
AuthValue::AuthValue() {
}

// CLASS ObjectValue
ObjectValue::ObjectValue() {
	intval_ = 0;
	sintval_ = 0;
	int64val_ = 0;
	sint64val_ = 0;
	byteval_ = 0;
	floatval_ = 0;
	doubleval_ = 0;
	booleanval_ = false;
}
int ObjectValue::get_intval() const {
	return intval_;
}
int ObjectValue::get_sintval() const {
	return sintval_;
}
long ObjectValue::get_int64val() const {
	return int64val_;
}
long ObjectValue::get_sint64val() const {
	return sint64val_;
}
std::string ObjectValue::get_strval() const {
	return strval_;
}
char* ObjectValue::get_byteval() const {
	return byteval_;
}
int ObjectValue::get_floatval() const {
	return floatval_;
}
long ObjectValue::get_doubleval() const {
	return doubleval_;
}
bool ObjectValue::is_booleanval() const {
	return booleanval_;
}
bool ObjectValue::operator==(const ObjectValue &other) const {
	if (intval_ == other.get_intval() && sintval_ == other.get_sintval()
			&& int64val_ == other.get_int64val()
			&& sint64val_ == other.get_sint64val()
			&& byteval_ == other.get_byteval()
			&& floatval_ == other.get_floatval()
			&& doubleval_ == other.get_doubleval()
			&& booleanval_ == other.is_booleanval())
		return true;
	else
		return false;
}

// CLASS CDAPException
CDAPException::CDAPException() :
		Exception("CDAP message caused an Exception") {
}
CDAPException::CDAPException(std::string arg0) :
		Exception(arg0.c_str()) {
}
CDAPException::CDAPException(int arg0, std::string arg1) :
		Exception(arg1.c_str()) {
	result_ = arg0;
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
							"AbsSyntax must be set for M_CONNECT and M_CONNECT_R messages"));
		}
	} else {
		if ((message.get_op_code() != CDAPMessage::M_CONNECT)
				&& (message.get_op_code() != CDAPMessage::M_CONNECT_R)) {
			throw CDAPException(
					std::string(
							"AbsSyntax can only be set for M_CONNECT and M_CONNECT_R messages"));
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
							"AuthMech can only be set for M_CONNECT and M_CONNECT_R messages"));
		}
	}
}

void CDAPMessageValidator::validateAuthValue(const CDAPMessage &message)
		throw (CDAPException) {
	if ((message.get_op_code() != CDAPMessage::M_CONNECT)
			&& (message.get_op_code() != CDAPMessage::M_CONNECT_R)) {
		throw CDAPException(
				std::string(
						"AuthValue can only be set for M_CONNECT and M_CONNECT_R messages"));
	}
}

void CDAPMessageValidator::validateDestAEInst(const CDAPMessage &message)
		throw (CDAPException) {
	if (!message.get_dest_ae_inst().empty()) {
		if ((message.get_op_code() != CDAPMessage::M_CONNECT)
				&& (message.get_op_code() != CDAPMessage::M_CONNECT_R)) {
			throw CDAPException(
					std::string(
							"dest_ae_inst can only be set for M_CONNECT and M_CONNECT_R messages"));
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
							"DestAEName can only be set for M_CONNECT and M_CONNECT_R messages"));
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
							"DestApInst can only be set for M_CONNECT and M_CONNECT_R messages"));
		}
	}
}

void CDAPMessageValidator::validateDestApName(const CDAPMessage &message)
		throw (CDAPException) {
	if (!message.get_dest_ap_name().empty()) {
		if (message.get_op_code() == CDAPMessage::M_CONNECT) {
			throw CDAPException(
					std::string(
							"DestApName must be set for the M_CONNECT message"));
		} else if (message.get_op_code() == CDAPMessage::M_CONNECT_R) {
			//TODO not sure what to do
		}
	} else {
		if (message.get_op_code() != CDAPMessage::M_CONNECT
				&& message.get_op_code() != CDAPMessage::M_CONNECT_R) {
			throw CDAPException(
					std::string(
							"DestApName can only be set for M_CONNECT and M_CONNECT_R messages"));
		}
	}
}

void CDAPMessageValidator::validateFilter(const CDAPMessage &message)
		throw (CDAPException) {
	if (message.get_filter() != 0) {
		if (message.get_op_code() != CDAPMessage::M_CREATE
				&& message.get_op_code() != CDAPMessage::M_DELETE
				&& message.get_op_code() != CDAPMessage::M_READ
				&& message.get_op_code() != CDAPMessage::M_WRITE
				&& message.get_op_code() != CDAPMessage::M_START
				&& message.get_op_code() != CDAPMessage::M_STOP) {
			throw CDAPException(
					std::string(
							"The filter parameter can only be set for M_CREATE, M_DELETE, M_READ, M_WRITE, M_START or M_STOP messages"));
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
					std::string("The invoke id parameter cannot be 0"));
		}
	}
}

void CDAPMessageValidator::validateObjClass(const CDAPMessage &message)
		throw (CDAPException) {
	if (!message.get_obj_class().empty()) {
		if (!message.get_obj_name().empty()) {
			throw CDAPException(
					std::string(
							"If the objClass parameter is set, the objName parameter also has to be set"));
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
							"The objClass parameter can only be set for M_CREATE, M_CREATE_R, M_DELETE, M_DELETE_R, M_READ, M_READ_R, M_WRITE, M_WRITE_R, M_START, M_STOP, M_START_R, M_STOP_R messages"));
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
							"The objInst parameter can only be set for M_CREATE, M_CREATE_R, M_DELETE, M_DELETE_R, M_READ, M_READ_R, M_WRITE, M_WRITE_R, M_START, M_START_R, M_STOP and M_STOP_R messages"));
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
							"The objName parameter can only be set for M_CREATE, M_CREATE_R, M_DELETE, M_DELETE_R, M_READ, M_READ_R, M_WRITE, M_WRITE_R, M_START, M_STOP, M_START_R and M_STOP_R messages"));
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
					"The objValue parameter must be set for M_WRITE messages");
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
							"The objValue parameter can only be set for M_CREATE, M_DELETE, M_CREATE_R, M_READ_R, M_WRITE, M_START, M_START_R, M_STOP, M_STOP_R and M_WRITE_R messages"));
		}
	}
}

void CDAPMessageValidator::validateOpcode(const CDAPMessage &message)
		throw (CDAPException) {
	if (message.get_op_code() == CDAPMessage::NONE_OPCODE) {
		throw CDAPException(
				std::string("The opcode must be set for all the messages"));
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
							"The resultReason parameter can only be set for M_CREATE_R, M_DELETE_R, M_READ_R, M_WRITE_R, M_START_R, M_STOP_R, M_CONNECT_R, M_RELEASE_R, M_CANCELREAD and M_CANCELREAD_R messages"));
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
							"The scope parameter can only be set for M_CREATE, M_DELETE, M_READ, M_WRITE, M_START or M_STOP messages"));
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
							"SrcAEInst can only be set for M_CONNECT and M_CONNECT_R messages"));
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
						"SrcAEName can only be set for M_CONNECT and M_CONNECT_R messages"));
	}
}

void CDAPMessageValidator::validateSrcApInst(const CDAPMessage &message)
		throw (CDAPException) {
	if (message.get_src_ap_inst().empty()) {
		if (message.get_op_code() != CDAPMessage::M_CONNECT
				&& message.get_op_code() != CDAPMessage::M_CONNECT_R) {
			throw CDAPException(
					std::string(
							"SrcApInst can only be set for M_CONNECT and M_CONNECT_R messages"));
		}
	}
}

void CDAPMessageValidator::validateSrcApName(const CDAPMessage &message)
		throw (CDAPException) {
	if (message.get_src_ap_name().empty()) {
		if (message.get_op_code() == CDAPMessage::M_CONNECT) {
			throw CDAPException(
					std::string(
							"SrcApName must be set for the M_CONNECT message"));
		} else if (message.get_op_code() == CDAPMessage::M_CONNECT_R) {
			//TODO not sure what to do
		}
	} else {
		if (message.get_op_code() != CDAPMessage::M_CONNECT
				&& message.get_op_code() != CDAPMessage::M_CONNECT_R) {
			throw CDAPException(
					std::string(
							"SrcApName can only be set for M_CONNECT and M_CONNECT_R messages"));
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
							"Version must be set for M_CONNECT and M_CONNECT_R messages"));
		}
	}
}

/* CLASS CDAPMessage */
const int CDAPMessage::ABSTRACT_SYNTAX_VERSION = 0x0073;
CDAPMessage::CDAPMessage() {
	abs_syntax_ = 0;
	auth_mech_ = AUTH_NONE;
	filter_ = 0;
	flags_ = NONE_FLAGS;
	invoke_id_ = 0;
	obj_inst_ = 0;
	op_code_ = NONE_OPCODE;
	result_ = 0;
	scope_ = 0;
	version_ = 0;
}

void CDAPMessage::getOpenConnectionRequestMessage(CDAPMessage &cdapMessage,
		AuthTypes authMech, const AuthValue &authValue,
		const std::string &dest_ae_inst, const std::string &destAEName,
		const std::string &destApInst, const std::string &destApName,
		const std::string &srcAEInst, const std::string &srcAEName,
		const std::string &srcApInst, const std::string &srcApName,
		int invokeID) throw (CDAPException) {
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
		AuthTypes authMech, const AuthValue &authValue,
		const std::string &dest_ae_inst, const std::string &destAEName,
		const std::string &destApInst, const std::string &destApName,
		int result, const std::string &resultReason,
		const std::string &srcAEInst, const std::string &srcAEName,
		const std::string &srcApInst, const std::string &srcApName,
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
		Flags flags, int result, const std::string &resultReason, int invokeID)
				throw (CDAPException) {
	cdapMessage.set_flags(flags);
	cdapMessage.set_invoke_id(invokeID);
	cdapMessage.set_op_code(M_RELEASE_R);
	cdapMessage.set_result(result);
	cdapMessage.set_result_reason(resultReason);
	CDAPMessageValidator::validate(cdapMessage);

}

void CDAPMessage::getCreateObjectRequestMessage(CDAPMessage &cdapMessage,
		char filter[], Flags flags, const std::string &objClass, long objInst,
		const std::string &objName, const ObjectValue &objValue, int scope)
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
		Flags flags, const std::string &objClass, long objInst,
		const std::string &objName, const ObjectValue &objValue, int result,
		const std::string &resultReason, int invokeID) throw (CDAPException) {
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
		char filter[], Flags flags, const std::string &objClass, long objInst,
		const std::string &objName, const ObjectValue &objectValue, int scope)
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
		Flags flags, const std::string &objClass, long objInst,
		const std::string &objName, int result, const std::string &resultReason,
		int invokeID) throw (CDAPException) {
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
		char filter[], Flags flags, const std::string &objClass,
		const ObjectValue &objValue, long objInst, const std::string &objName,
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
		Flags flags, int result, const std::string &resultReason, int invokeID)
				throw (CDAPException) {
	cdapMessage.set_flags(flags);
	cdapMessage.set_invoke_id(invokeID);
	cdapMessage.set_op_code(M_START_R);
	cdapMessage.set_result(result);
	cdapMessage.set_result_reason(resultReason);
	CDAPMessageValidator::validate(cdapMessage);

}
void CDAPMessage::getStartObjectResponseMessage(CDAPMessage &cdapMessage,
		Flags flags, const std::string &objectClass,
		const ObjectValue &objectValue, long objInst,
		const std::string &objName, int result, const std::string &resultReason,
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
		char filter[], Flags flags, const std::string &objClass,
		const ObjectValue &objValue, long objInst, const std::string &objName,
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
		Flags flags, int result, const std::string &resultReason, int invokeID)
				throw (CDAPException) {
	cdapMessage.set_flags(flags);
	cdapMessage.set_invoke_id(invokeID);
	cdapMessage.set_op_code(M_STOP_R);
	cdapMessage.set_result(result);
	cdapMessage.set_result_reason(resultReason);
	CDAPMessageValidator::validate(cdapMessage);
}
void CDAPMessage::getReadObjectRequestMessage(CDAPMessage &cdapMessage,
		char filter[], Flags flags, const std::string &objClass, long objInst,
		const std::string &objName, int scope) throw (CDAPException) {
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
		Flags flags, const std::string &objClass, long objInst,
		const std::string &objName, const ObjectValue &objValue, int result,
		const std::string &resultReason, int invokeID) throw (CDAPException) {
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
		char filter[], Flags flags, const std::string &objClass, long objInst,
		const ObjectValue &objValue, const std::string &objName, int scope)
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
		Flags flags, int result, int invokeID, const std::string &resultReason)
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
		Flags flags, int invokeID, int result, const std::string &resultReason)
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
	return abs_syntax_;
}
void CDAPMessage::set_abs_syntax(int arg0) {
	abs_syntax_ = arg0;
}
CDAPMessage::AuthTypes CDAPMessage::get_auth_mech() const {
	return auth_mech_;
}
void CDAPMessage::set_auth_mech(AuthTypes arg0) {
	auth_mech_ = arg0;
}
const AuthValue& CDAPMessage::get_auth_value() const {
	return auth_value_;
}
void CDAPMessage::set_auth_value(AuthValue arg0) {
	auth_value_ = arg0;
}
std::string CDAPMessage::get_dest_ae_inst() const {
	return dest_ae_inst_;
}
void CDAPMessage::set_dest_ae_inst(std::string arg0) {
	dest_ae_inst_ = arg0;
}
std::string CDAPMessage::get_dest_ae_name() const {
	return dest_ae_name_;
}
void CDAPMessage::set_dest_ae_name(std::string arg0) {
	dest_ae_name_ = arg0;
}
std::string CDAPMessage::get_dest_ap_inst() const {
	return dest_ap_inst_;
}
void CDAPMessage::set_dest_ap_inst(std::string arg0) {
	dest_ap_inst_ = arg0;
}
std::string CDAPMessage::get_dest_ap_name() const {
	return dest_ap_name_;
}
void CDAPMessage::set_dest_ap_name(std::string arg0) {
	dest_ap_name_ = arg0;
}
char* CDAPMessage::get_filter() const {
	return filter_;
}
void CDAPMessage::set_filter(char arg0[]) {
	filter_ = arg0;
}
CDAPMessage::Flags CDAPMessage::get_flags() const {
	return flags_;
}
void CDAPMessage::set_flags(Flags arg0) {
	flags_ = arg0;
}
int CDAPMessage::get_invoke_id() const {
	return invoke_id_;
}
void CDAPMessage::set_invoke_id(int arg0) {
	invoke_id_ = arg0;
}
std::string CDAPMessage::get_obj_class() const {
	return obj_class_;
}
void CDAPMessage::set_obj_class(std::string arg0) {
	obj_class_ = arg0;
}
long CDAPMessage::get_obj_inst() const {
	return obj_inst_;
}
void CDAPMessage::set_obj_inst(long arg0) {
	obj_inst_ = arg0;
}
std::string CDAPMessage::get_obj_name() const {
	return obj_name_;
}
void CDAPMessage::set_obj_name(std::string arg0) {
	obj_name_ = arg0;
}
const ObjectValue& CDAPMessage::get_obj_value() const {
	return obj_value_;
}
void CDAPMessage::set_obj_value(const ObjectValue &arg0) {
	obj_value_ = arg0;
}
CDAPMessage::Opcode CDAPMessage::get_op_code() const {
	return op_code_;
}
void CDAPMessage::set_op_code(Opcode arg0) {
	op_code_ = arg0;
}
int CDAPMessage::get_result() const {
	return result_;
}
void CDAPMessage::set_result(int arg0) {
	result_ = arg0;
}
std::string CDAPMessage::get_result_reason() const {
	return result_reason_;
}
void CDAPMessage::set_result_reason(std::string arg0) {
	result_reason_ = arg0;
}
int CDAPMessage::get_scope() const {
	return scope_;
}
void CDAPMessage::set_scope(int arg0) {
	scope_ = arg0;
}
std::string CDAPMessage::get_src_ae_inst() const {
	return src_ae_inst_;
}
void CDAPMessage::set_src_ae_inst(std::string arg0) {
	src_ae_inst_ = arg0;
}
std::string CDAPMessage::get_src_ae_name() const {
	return src_ae_name_;
}
void CDAPMessage::set_src_ae_name(std::string arg0) {
	src_ae_name_ = arg0;
}
std::string CDAPMessage::get_src_ap_inst() const {
	return src_ap_inst_;
}
void CDAPMessage::set_src_ap_inst(std::string arg0) {
	src_ap_inst_ = arg0;
}
std::string CDAPMessage::get_src_ap_name() const {
	return src_ap_name_;
}

void CDAPMessage::set_src_ap_name(std::string arg0) {
	src_ap_name_ = arg0;
}
long CDAPMessage::get_version() const {
	return version_;
}
void CDAPMessage::set_version(long arg0) {
	version_ = arg0;
}
std::string toString() {
	return std::string("return");
}

/*	class CDAPSessionDescriptor	*/
CDAPSessionDescriptor::~CDAPSessionDescriptor() {
	delete ap_naming_info_;
}
const ApplicationProcessNamingInformation* CDAPSessionDescriptor::get_source_application_process_naming_info() {
	ap_naming_info_ = new ApplicationProcessNamingInformation(src_ap_name_,
			src_ap_inst_);
	if (!src_ae_name_.empty()) {
		ap_naming_info_->setEntityName(src_ae_name_);
	}

	if (!src_ae_inst_.empty()) {
		ap_naming_info_->setEntityInstance(src_ae_inst_);
	}
	return ap_naming_info_;
}
const ApplicationProcessNamingInformation* CDAPSessionDescriptor::get_destination_application_process_naming_info() {
	ap_naming_info_ = new ApplicationProcessNamingInformation(dest_ap_name_,
			dest_ap_inst_);
	if (!dest_ae_name_.empty()) {
		ap_naming_info_->setEntityName(dest_ae_name_);
	}

	if (!dest_ae_inst_.empty()) {
		ap_naming_info_->setEntityInstance(dest_ae_inst_);
	}
	return ap_naming_info_;
}

/*	CLASS RIBDaemonException	*/

RIBDaemonException::RIBDaemonException(ErrorCode arg0) :
		Exception("RIBDaemon caused an exception") {
	error_code_ = arg0;
}

RIBDaemonException::RIBDaemonException(ErrorCode arg0, const char* arg1) :
		Exception(arg1) {
	error_code_ = arg0;
}
}

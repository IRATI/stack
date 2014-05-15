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
	authOther = NULL;
}

std::string AuthValue::getAuthName() const {
	return authName;
}

void AuthValue::setAuthName(std::string arg0) {
	authName = arg0;
}

std::string AuthValue::getAuthPassword() const {
	return authPassword;
}

void AuthValue::setAuthPassword(std::string arg0) {
	authPassword = arg0;
}

char* AuthValue::getAuthOther() const {
	return authOther;
}

void AuthValue::setAuthOther(char* arg0) {
	authOther = arg0;
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

int ObjectValue::getIntval() const {
	return intval;
}

void ObjectValue::setIntval(int arg0) {
	intval = arg0;
}

int ObjectValue::getSintval() const {
	return sintval;
}

void ObjectValue::setSintval(int arg0) {
	sintval = arg0;
}

long ObjectValue::getInt64val() const {
	return int64val;
}

void ObjectValue::setInt64val(long arg0) {
	int64val = arg0;
}

long ObjectValue::getSint64val() const {
	return sint64val;
}

void ObjectValue::setSint64val(long arg0) {
	sint64val = arg0;
}

std::string ObjectValue::getStrval() const {
	return strval;
}

void ObjectValue::setStrval(std::string arg0) {
	strval = arg0;
}

char* ObjectValue::getByteval() const {
	return byteval;
}

void ObjectValue::setByteval(char arg0[]) {
	byteval = arg0;
}

int ObjectValue::getFloatval() const {
	return floatval;
}

void ObjectValue::setFloatval(int arg0) {
	floatval = arg0;
}

long ObjectValue::getDoubleval() const {
	return doubleval;
}

void ObjectValue::setDoubleval(long arg0) {
	doubleval = arg0;
}

void ObjectValue::setBooleanval(bool arg0) {
	booleanval = arg0;
}

bool ObjectValue::isBooleanval() const {
	return booleanval;

}

bool ObjectValue::operator==(const ObjectValue &other) const {
	if (intval == other.getIntval() && sintval == other.getSintval()
			&& int64val == other.getInt64val()
			&& sint64val == other.getSint64val()
			&& byteval == other.getByteval() && floatval == other.getFloatval()
			&& doubleval == other.getDoubleval()
			&& booleanval == other.isBooleanval())
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
	if (message.get_absSyntax() == 0) {
		if (message.getOpCode() == CDAPMessage::M_CONNECT
				|| message.getOpCode() == CDAPMessage::M_CONNECT_R) {
			throw CDAPException(
					std::string(
							"AbsSyntax must be set for M_CONNECT and M_CONNECT_R messages"),
					message);
		}
	} else {
		if ((message.getOpCode() != CDAPMessage::M_CONNECT)
				&& (message.getOpCode() != CDAPMessage::M_CONNECT_R)) {
			throw CDAPException(
					"AbsSyntax can only be set for M_CONNECT and M_CONNECT_R messages",
					message);
		}
	}
}

void CDAPMessageValidator::validateAuthMech(const CDAPMessage &message)
		throw (CDAPException) {
	if (message.get_authMech() != CDAPMessage::AUTH_NONE) {
		if ((message.getOpCode() != CDAPMessage::M_CONNECT)
				&& message.getOpCode() != CDAPMessage::M_CONNECT_R) {
			throw CDAPException(
					std::string(
							"AuthMech can only be set for M_CONNECT and M_CONNECT_R messages"),
					message);
		}
	}
}

void CDAPMessageValidator::validateAuthValue(const CDAPMessage &message)
		throw (CDAPException) {
	if ((message.getOpCode() != CDAPMessage::M_CONNECT)
			&& (message.getOpCode() != CDAPMessage::M_CONNECT_R)) {
		throw CDAPException(
				std::string(
						"AuthValue can only be set for M_CONNECT and M_CONNECT_R messages"),
				message);
	}
}

void CDAPMessageValidator::validateDestAEInst(const CDAPMessage &message)
		throw (CDAPException) {
	if (!message.getDestAEInst().empty()) {
		if ((message.getOpCode() != CDAPMessage::M_CONNECT)
				&& (message.getOpCode() != CDAPMessage::M_CONNECT_R)) {
			throw CDAPException(
					std::string(
							"DestAEInst can only be set for M_CONNECT and M_CONNECT_R messages"),
					message);
		}
	}
}

void CDAPMessageValidator::validateDestAEName(const CDAPMessage &message)
		throw (CDAPException) {
	if (!message.getDestAEName().empty()) {
		if ((message.getOpCode() != CDAPMessage::M_CONNECT)
				&& (message.getOpCode() != CDAPMessage::M_CONNECT_R)) {
			throw CDAPException(
					std::string(
							"DestAEName can only be set for M_CONNECT and M_CONNECT_R messages"),
					message);
		}
	}
}

void CDAPMessageValidator::validateDestApInst(const CDAPMessage &message)
		throw (CDAPException) {
	if (!message.getDestApInst().empty()) {
		if (message.getOpCode() != CDAPMessage::M_CONNECT
				&& message.getOpCode() != CDAPMessage::M_CONNECT_R) {
			throw CDAPException(
					std::string(
							"DestApInst can only be set for M_CONNECT and M_CONNECT_R messages"),
					message);
		}
	}
}

void CDAPMessageValidator::validateDestApName(const CDAPMessage &message)
		throw (CDAPException) {
	if (!message.getDestApName().empty()) {
		if (message.getOpCode() == CDAPMessage::M_CONNECT) {
			throw CDAPException(
					std::string(
							"DestApName must be set for the M_CONNECT message"),
					message);
		} else if (message.getOpCode() == CDAPMessage::M_CONNECT_R) {
			//TODO not sure what to do
		}
	} else {
		if (message.getOpCode() != CDAPMessage::M_CONNECT
				&& message.getOpCode() != CDAPMessage::M_CONNECT_R) {
			throw CDAPException(
					std::string(
							"DestApName can only be set for M_CONNECT and M_CONNECT_R messages"),
					message);
		}
	}
}

void CDAPMessageValidator::validateFilter(const CDAPMessage &message)
		throw (CDAPException) {
	if (message.getFilter() != NULL) {
		if (message.getOpCode() != CDAPMessage::M_CREATE
				&& message.getOpCode() != CDAPMessage::M_DELETE
				&& message.getOpCode() != CDAPMessage::M_READ
				&& message.getOpCode() != CDAPMessage::M_WRITE
				&& message.getOpCode() != CDAPMessage::M_START
				&& message.getOpCode() != CDAPMessage::M_STOP) {
			throw CDAPException(
					std::string(
							"The filter parameter can only be set for M_CREATE, M_DELETE, M_READ, M_WRITE, M_START or M_STOP messages"),
					message);
		}
	}
}

void CDAPMessageValidator::validateInvokeID(const CDAPMessage &message)
		throw (CDAPException) {
	if (message.getInvokeID() == 0) {
		if (message.getOpCode() == CDAPMessage::M_CONNECT
				|| message.getOpCode() == CDAPMessage::M_CONNECT_R
				|| message.getOpCode() == CDAPMessage::M_RELEASE_R
				|| message.getOpCode() == CDAPMessage::M_CREATE_R
				|| message.getOpCode() == CDAPMessage::M_DELETE_R
				|| message.getOpCode() == CDAPMessage::M_READ_R
				|| message.getOpCode() == CDAPMessage::M_CANCELREAD
				|| message.getOpCode() == CDAPMessage::M_CANCELREAD_R
				|| message.getOpCode() == CDAPMessage::M_WRITE_R
				|| message.getOpCode() == CDAPMessage::M_START_R
				|| message.getOpCode() == CDAPMessage::M_STOP_R) {
			throw CDAPException(
					std::string("The invoke id parameter cannot be 0"),
					message);
		}
	}
}

void CDAPMessageValidator::validateObjClass(const CDAPMessage &message)
		throw (CDAPException) {
	if (!message.getObjClass().empty()) {
		if (!message.getObjName().empty()) {
			throw CDAPException(
					"If the objClass parameter is set, the objName parameter also has to be set",
					message);
		}
		if (message.getOpCode() != CDAPMessage::M_CREATE
				&& message.getOpCode() != CDAPMessage::M_CREATE_R
				&& message.getOpCode() != CDAPMessage::M_DELETE
				&& message.getOpCode() != CDAPMessage::M_DELETE_R
				&& message.getOpCode() != CDAPMessage::M_READ
				&& message.getOpCode() != CDAPMessage::M_READ_R
				&& message.getOpCode() != CDAPMessage::M_WRITE
				&& message.getOpCode() != CDAPMessage::M_WRITE_R
				&& message.getOpCode() != CDAPMessage::M_START
				&& message.getOpCode() != CDAPMessage::M_STOP
				&& message.getOpCode() != CDAPMessage::M_START_R
				&& message.getOpCode() != CDAPMessage::M_STOP_R) {
			throw CDAPException(
					std::string(
							"The objClass parameter can only be set for M_CREATE, M_CREATE_R, M_DELETE, M_DELETE_R, M_READ, M_READ_R, M_WRITE, M_WRITE_R, M_START, M_STOP, M_START_R, M_STOP_R messages"),
					message);
		}
	}
}

void CDAPMessageValidator::validateObjInst(const CDAPMessage &message)
		throw (CDAPException) {
	if (message.getObjInst() != 0) {
		if (message.getOpCode() != CDAPMessage::M_CREATE
				&& message.getOpCode() != CDAPMessage::M_CREATE_R
				&& message.getOpCode() != CDAPMessage::M_DELETE
				&& message.getOpCode() != CDAPMessage::M_DELETE_R
				&& message.getOpCode() != CDAPMessage::M_READ
				&& message.getOpCode() != CDAPMessage::M_READ_R
				&& message.getOpCode() != CDAPMessage::M_WRITE
				&& message.getOpCode() != CDAPMessage::M_WRITE_R
				&& message.getOpCode() != CDAPMessage::M_START
				&& message.getOpCode() != CDAPMessage::M_STOP
				&& message.getOpCode() != CDAPMessage::M_START_R
				&& message.getOpCode() != CDAPMessage::M_STOP_R) {
			throw CDAPException(
					std::string(
							"The objInst parameter can only be set for M_CREATE, M_CREATE_R, M_DELETE, M_DELETE_R, M_READ, M_READ_R, M_WRITE, M_WRITE_R, M_START, M_START_R, M_STOP and M_STOP_R messages"),
					message);
		}
	}
}

void CDAPMessageValidator::validateObjName(const CDAPMessage &message)
		throw (CDAPException) {
	if (!message.getObjName().empty()) {
		if (message.getObjClass().empty()) {
			throw new CDAPException(
					std::string(
							"If the objName parameter is set, the objClass parameter also has to be set"));
		}
		if (message.getOpCode() != CDAPMessage::M_CREATE
				&& message.getOpCode() != CDAPMessage::M_CREATE_R
				&& message.getOpCode() != CDAPMessage::M_DELETE
				&& message.getOpCode() != CDAPMessage::M_DELETE_R
				&& message.getOpCode() != CDAPMessage::M_READ
				&& message.getOpCode() != CDAPMessage::M_READ_R
				&& message.getOpCode() != CDAPMessage::M_WRITE
				&& message.getOpCode() != CDAPMessage::M_WRITE_R
				&& message.getOpCode() != CDAPMessage::M_START
				&& message.getOpCode() != CDAPMessage::M_STOP
				&& message.getOpCode() != CDAPMessage::M_START_R
				&& message.getOpCode() != CDAPMessage::M_STOP_R) {
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
	if (message.getObjValue() == emptyObj) {
		if (message.getOpCode() == CDAPMessage::M_WRITE) {
			throw CDAPException(
					std::string(
							"The objValue parameter must be set for M_WRITE messages"),
					message);
		}
	} else {
		if (message.getOpCode() != CDAPMessage::M_CREATE
				&& message.getOpCode() != CDAPMessage::M_CREATE_R
				&& message.getOpCode() != CDAPMessage::M_READ_R
				&& message.getOpCode() != CDAPMessage::M_WRITE
				&& message.getOpCode() != CDAPMessage::M_START
				&& message.getOpCode() != CDAPMessage::M_STOP
				&& message.getOpCode() != CDAPMessage::M_START_R
				&& message.getOpCode() != CDAPMessage::M_STOP_R
				&& message.getOpCode() != CDAPMessage::M_WRITE_R
				&& message.getOpCode() != CDAPMessage::M_DELETE
				&& message.getOpCode() != CDAPMessage::M_READ) {
			throw CDAPException(
					std::string(
							"The objValue parameter can only be set for M_CREATE, M_DELETE, M_CREATE_R, M_READ_R, M_WRITE, M_START, M_START_R, M_STOP, M_STOP_R and M_WRITE_R messages"),
					message);
		}
	}
}

void CDAPMessageValidator::validateOpcode(const CDAPMessage &message)
		throw (CDAPException) {
	if (message.getOpCode() == CDAPMessage::NONE_OPCODE) {
		throw CDAPException(
				std::string("The opcode must be set for all the messages"),
				message);
	}
}

void CDAPMessageValidator::validateResult(const CDAPMessage &message)
		throw (CDAPException) {
	/* FIXME: Do something with sense */
	message.get_absSyntax();
}

void CDAPMessageValidator::validateResultReason(const CDAPMessage &message)
		throw (CDAPException) {
	if (!message.getResultReason().empty()) {
		if (message.getOpCode() != CDAPMessage::M_CREATE_R
				&& message.getOpCode() != CDAPMessage::M_DELETE_R
				&& message.getOpCode() != CDAPMessage::M_READ_R
				&& message.getOpCode() != CDAPMessage::M_WRITE_R
				&& message.getOpCode() != CDAPMessage::M_CONNECT_R
				&& message.getOpCode() != CDAPMessage::M_RELEASE_R
				&& message.getOpCode() != CDAPMessage::M_CANCELREAD
				&& message.getOpCode() != CDAPMessage::M_CANCELREAD_R
				&& message.getOpCode() != CDAPMessage::M_START_R
				&& message.getOpCode() != CDAPMessage::M_STOP_R) {
			throw CDAPException(
					std::string(
							"The resultReason parameter can only be set for M_CREATE_R, M_DELETE_R, M_READ_R, M_WRITE_R, M_START_R, M_STOP_R, M_CONNECT_R, M_RELEASE_R, M_CANCELREAD and M_CANCELREAD_R messages"),
					message);
		}
	}
}

void CDAPMessageValidator::validateScope(const CDAPMessage &message)
		throw (CDAPException) {
	if (message.getScope() != 0) {
		if (message.getOpCode() != CDAPMessage::M_CREATE
				&& message.getOpCode() != CDAPMessage::M_DELETE
				&& message.getOpCode() != CDAPMessage::M_READ
				&& message.getOpCode() != CDAPMessage::M_WRITE
				&& message.getOpCode() != CDAPMessage::M_START
				&& message.getOpCode() != CDAPMessage::M_STOP) {
			throw CDAPException(
					std::string(
							"The scope parameter can only be set for M_CREATE, M_DELETE, M_READ, M_WRITE, M_START or M_STOP messages"),
					message);
		}
	}
}

void CDAPMessageValidator::validateSrcAEInst(const CDAPMessage &message)
		throw (CDAPException) {
	if (!message.getSrcAEInst().empty()) {
		if (message.getOpCode() != CDAPMessage::M_CONNECT
				&& message.getOpCode() != CDAPMessage::M_CONNECT_R) {
			throw CDAPException(
					std::string(
							"SrcAEInst can only be set for M_CONNECT and M_CONNECT_R messages"),
					message);
		}
	}
}

void CDAPMessageValidator::validateSrcAEName(const CDAPMessage &message)
		throw (CDAPException) {
	if (!message.getSrcAEName().empty()) {

	}
	if (message.getOpCode() != CDAPMessage::M_CONNECT
			&& message.getOpCode() != CDAPMessage::M_CONNECT_R) {
		throw CDAPException(
				std::string(
						"SrcAEName can only be set for M_CONNECT and M_CONNECT_R messages"),
				message);
	}
}

void CDAPMessageValidator::validateSrcApInst(const CDAPMessage &message)
		throw (CDAPException) {
	if (message.getSrcApInst().empty()) {
		if (message.getOpCode() != CDAPMessage::M_CONNECT
				&& message.getOpCode() != CDAPMessage::M_CONNECT_R) {
			throw CDAPException(
					std::string(
							"SrcApInst can only be set for M_CONNECT and M_CONNECT_R messages"),
					message);
		}
	}
}

void CDAPMessageValidator::validateSrcApName(const CDAPMessage &message)
		throw (CDAPException) {
	if (message.getSrcApName().empty()) {
		if (message.getOpCode() == CDAPMessage::M_CONNECT) {
			throw CDAPException(
					std::string(
							"SrcApName must be set for the M_CONNECT message"),
					message);
		} else if (message.getOpCode() == CDAPMessage::M_CONNECT_R) {
			//TODO not sure what to do
		}
	} else {
		if (message.getOpCode() != CDAPMessage::M_CONNECT
				&& message.getOpCode() != CDAPMessage::M_CONNECT_R) {
			throw CDAPException(
					std::string(
							"SrcApName can only be set for M_CONNECT and M_CONNECT_R messages"),
					message);
		}
	}
}

void CDAPMessageValidator::validateVersion(const CDAPMessage &message)
		throw (CDAPException) {
	if (message.getVersion() == 0) {
		if (message.getOpCode() == CDAPMessage::M_CONNECT
				|| message.getOpCode() == CDAPMessage::M_CONNECT_R) {
			throw CDAPException(
					std::string(
							"Version must be set for M_CONNECT and M_CONNECT_R messages"),
					message);
		}
	}
}

/* CLASS CDAPMessage */
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
		AuthTypes authMech, const AuthValue &authValue, std::string destAEInst,
		std::string destAEName, std::string destApInst, std::string destApName,
		std::string srcAEInst, std::string srcAEName, std::string srcApInst,
		std::string srcApName, int invokeID) throw (CDAPException) {
	cdapMessage.set_absSyntax(ABSTRACT_SYNTAX_VERSION);
	cdapMessage.setAuthMech(authMech);
	cdapMessage.setAuthValue(authValue);
	cdapMessage.setDestAEInst(destAEInst);
	cdapMessage.setDestAEName(destAEName);
	cdapMessage.setDestApInst(destApInst);
	cdapMessage.setDestApInst(destApInst);
	cdapMessage.setDestApName(destApName);
	cdapMessage.setInvokeID(invokeID);
	cdapMessage.setOpCode(M_CONNECT);
	cdapMessage.setSrcAEInst(srcAEInst);
	cdapMessage.setSrcAEName(srcAEName);
	cdapMessage.setSrcApInst(srcApInst);
	cdapMessage.setSrcApName(srcApName);
	cdapMessage.setVersion(1);
	CDAPMessageValidator::validate(cdapMessage);

}

void CDAPMessage::getOpenConnectionResponseMessage(CDAPMessage &cdapMessage,
		AuthTypes authMech, const AuthValue &authValue, std::string destAEInst,
		std::string destAEName, std::string destApInst, std::string destApName,
		int result, std::string resultReason, std::string srcAEInst,
		std::string srcAEName, std::string srcApInst, std::string srcApName,
		int invokeID) throw (CDAPException) {
	cdapMessage.set_absSyntax(ABSTRACT_SYNTAX_VERSION);
	cdapMessage.setAuthMech(authMech);
	cdapMessage.setAuthValue(authValue);
	cdapMessage.setDestAEInst(destAEInst);
	cdapMessage.setDestAEName(destAEName);
	cdapMessage.setDestApInst(destApInst);
	cdapMessage.setDestApInst(destApInst);
	cdapMessage.setDestApName(destApName);
	cdapMessage.setOpCode(M_CONNECT_R);
	cdapMessage.setInvokeID(invokeID);
	cdapMessage.setResult(result);
	cdapMessage.setResultReason(resultReason);
	cdapMessage.setSrcAEInst(srcAEInst);
	cdapMessage.setSrcAEName(srcAEName);
	cdapMessage.setSrcApInst(srcApInst);
	cdapMessage.setSrcApName(srcApName);
	cdapMessage.setVersion(1);
	CDAPMessageValidator::validate(cdapMessage);

}

void CDAPMessage::getReleaseConnectionRequestMessage(CDAPMessage &cdapMessage,
		Flags flags) throw (CDAPException) {
	cdapMessage.setFlags(flags);
	cdapMessage.setOpCode(M_RELEASE);
	CDAPMessageValidator::validate(cdapMessage);

}

void CDAPMessage::getReleaseConnectionResponseMessage(CDAPMessage &cdapMessage,
		Flags flags, int result, std::string resultReason, int invokeID)
				throw (CDAPException) {
	cdapMessage.setFlags(flags);
	cdapMessage.setInvokeID(invokeID);
	cdapMessage.setOpCode(M_RELEASE_R);
	cdapMessage.setResult(result);
	cdapMessage.setResultReason(resultReason);
	CDAPMessageValidator::validate(cdapMessage);

}

void CDAPMessage::getCreateObjectRequestMessage(CDAPMessage &cdapMessage,
		char filter[], Flags flags, std::string objClass, long objInst,
		std::string objName, const ObjectValue &objValue, int scope)
				throw (CDAPException) {
	cdapMessage.setFilter(filter);
	cdapMessage.setFlags(flags);
	cdapMessage.setObjClass(objClass);
	cdapMessage.setObjInst(objInst);
	cdapMessage.setObjName(objName);
	cdapMessage.setObjValue(objValue);
	cdapMessage.setOpCode(M_CREATE);
	cdapMessage.setScope(scope);
	CDAPMessageValidator::validate(cdapMessage);

}

void CDAPMessage::getCreateObjectResponseMessage(CDAPMessage &cdapMessage,
		Flags flags, std::string objClass, long objInst, std::string objName,
		const ObjectValue &objValue, int result, std::string resultReason,
		int invokeID) throw (CDAPException) {
	cdapMessage.setFlags(flags);
	cdapMessage.setInvokeID(invokeID);
	cdapMessage.setObjClass(objClass);
	cdapMessage.setObjInst(objInst);
	cdapMessage.setObjName(objName);
	cdapMessage.setObjValue(objValue);
	cdapMessage.setOpCode(M_CREATE_R);
	cdapMessage.setResult(result);
	cdapMessage.setResultReason(resultReason);
	CDAPMessageValidator::validate(cdapMessage);

}

void CDAPMessage::getDeleteObjectRequestMessage(CDAPMessage &cdapMessage,
		char filter[], Flags flags, std::string objClass, long objInst,
		std::string objName, const ObjectValue &objectValue, int scope)
				throw (CDAPException) {
	cdapMessage.setFilter(filter);
	cdapMessage.setFlags(flags);
	cdapMessage.setObjClass(objClass);
	cdapMessage.setObjInst(objInst);
	cdapMessage.setObjName(objName);
	cdapMessage.setObjValue(objectValue);
	cdapMessage.setOpCode(M_DELETE);
	cdapMessage.setScope(scope);
	CDAPMessageValidator::validate(cdapMessage);

}
void CDAPMessage::getDeleteObjectResponseMessage(CDAPMessage &cdapMessage,
		Flags flags, std::string objClass, long objInst, std::string objName,
		int result, std::string resultReason, int invokeID)
				throw (CDAPException) {
	cdapMessage.setFlags(flags);
	cdapMessage.setInvokeID(invokeID);
	cdapMessage.setObjClass(objClass);
	cdapMessage.setObjInst(objInst);
	cdapMessage.setObjName(objName);
	cdapMessage.setOpCode(M_DELETE_R);
	cdapMessage.setResult(result);
	cdapMessage.setResultReason(resultReason);
	CDAPMessageValidator::validate(cdapMessage);

}
void CDAPMessage::getStartObjectRequestMessage(CDAPMessage &cdapMessage,
		char filter[], Flags flags, std::string objClass,
		const ObjectValue &objValue, long objInst, std::string objName,
		int scope) throw (CDAPException) {
	cdapMessage.setFilter(filter);
	cdapMessage.setFlags(flags);
	cdapMessage.setObjClass(objClass);
	cdapMessage.setObjInst(objInst);
	cdapMessage.setObjName(objName);
	cdapMessage.setObjValue(objValue);
	cdapMessage.setOpCode(M_START);
	cdapMessage.setScope(scope);
	CDAPMessageValidator::validate(cdapMessage);

}
void CDAPMessage::getStartObjectResponseMessage(CDAPMessage &cdapMessage,
		Flags flags, int result, std::string resultReason, int invokeID)
				throw (CDAPException) {
	cdapMessage.setFlags(flags);
	cdapMessage.setInvokeID(invokeID);
	cdapMessage.setOpCode(M_START_R);
	cdapMessage.setResult(result);
	cdapMessage.setResultReason(resultReason);
	CDAPMessageValidator::validate(cdapMessage);

}
void CDAPMessage::getStartObjectResponseMessage(CDAPMessage &cdapMessage,
		Flags flags, std::string objectClass, const ObjectValue &objectValue,
		long objInst, std::string objName, int result, std::string resultReason,
		int invokeID) throw (CDAPException) {
	cdapMessage.setFlags(flags);
	cdapMessage.setInvokeID(invokeID);
	cdapMessage.setOpCode(M_START_R);
	cdapMessage.setObjClass(objectClass);
	cdapMessage.setObjInst(objInst);
	cdapMessage.setObjName(objName);
	cdapMessage.setObjValue(objectValue);
	cdapMessage.setResult(result);
	cdapMessage.setResultReason(resultReason);
	CDAPMessageValidator::validate(cdapMessage);
}
void CDAPMessage::getStopObjectRequestMessage(CDAPMessage &cdapMessage,
		char filter[], Flags flags, std::string objClass,
		const ObjectValue &objValue, long objInst, std::string objName,
		int scope) throw (CDAPException) {
	cdapMessage.setFilter(filter);
	cdapMessage.setFlags(flags);
	cdapMessage.setObjClass(objClass);
	cdapMessage.setObjInst(objInst);
	cdapMessage.setObjName(objName);
	cdapMessage.setObjValue(objValue);
	cdapMessage.setOpCode(M_STOP);
	cdapMessage.setScope(scope);
	CDAPMessageValidator::validate(cdapMessage);
}
void CDAPMessage::getStopObjectResponseMessage(CDAPMessage &cdapMessage,
		Flags flags, int result, std::string resultReason, int invokeID)
				throw (CDAPException) {
	cdapMessage.setFlags(flags);
	cdapMessage.setInvokeID(invokeID);
	cdapMessage.setOpCode(M_STOP_R);
	cdapMessage.setResult(result);
	cdapMessage.setResultReason(resultReason);
	CDAPMessageValidator::validate(cdapMessage);
}
void CDAPMessage::getReadObjectRequestMessage(CDAPMessage &cdapMessage,
		char filter[], Flags flags, std::string objClass, long objInst,
		std::string objName, int scope) throw (CDAPException) {
	cdapMessage.setFilter(filter);
	cdapMessage.setFlags(flags);
	cdapMessage.setObjClass(objClass);
	cdapMessage.setObjInst(objInst);
	cdapMessage.setObjName(objName);
	cdapMessage.setOpCode(M_READ);
	cdapMessage.setScope(scope);
	CDAPMessageValidator::validate(cdapMessage);
}
void CDAPMessage::getReadObjectResponseMessage(CDAPMessage &cdapMessage,
		Flags flags, std::string objClass, long objInst, std::string objName,
		const ObjectValue &objValue, int result, std::string resultReason,
		int invokeID) throw (CDAPException) {
	cdapMessage.setFlags(flags);
	cdapMessage.setInvokeID(invokeID);
	cdapMessage.setObjClass(objClass);
	cdapMessage.setObjInst(objInst);
	cdapMessage.setObjName(objName);
	cdapMessage.setObjValue(objValue);
	cdapMessage.setOpCode(M_READ_R);
	cdapMessage.setResult(result);
	cdapMessage.setResultReason(resultReason);
	CDAPMessageValidator::validate(cdapMessage);
}
void CDAPMessage::getWriteObjectRequestMessage(CDAPMessage &cdapMessage,
		char filter[], Flags flags, std::string objClass, long objInst,
		const ObjectValue &objValue, std::string objName, int scope)
				throw (CDAPException) {
	cdapMessage.setFilter(filter);
	cdapMessage.setFlags(flags);
	cdapMessage.setObjClass(objClass);
	cdapMessage.setObjInst(objInst);
	cdapMessage.setObjName(objName);
	cdapMessage.setObjValue(objValue);
	cdapMessage.setOpCode(M_WRITE);
	cdapMessage.setScope(scope);
	CDAPMessageValidator::validate(cdapMessage);
}
void CDAPMessage::getWriteObjectResponseMessage(CDAPMessage &cdapMessage,
		Flags flags, int result, int invokeID, std::string resultReason)
				throw (CDAPException) {
	cdapMessage.setFlags(flags);
	cdapMessage.setOpCode(M_WRITE_R);
	cdapMessage.setResult(result);
	cdapMessage.setResultReason(resultReason);
	cdapMessage.setInvokeID(invokeID);
	CDAPMessageValidator::validate(cdapMessage);
}
void CDAPMessage::getCancelReadRequestMessage(CDAPMessage &cdapMessage,
		Flags flags, int invokeID) throw (CDAPException) {
	cdapMessage.setFlags(flags);
	cdapMessage.setInvokeID(invokeID);
	cdapMessage.setOpCode(M_CANCELREAD);
	CDAPMessageValidator::validate(cdapMessage);
}
void CDAPMessage::getCancelReadResponseMessage(CDAPMessage &cdapMessage,
		Flags flags, int invokeID, int result, std::string resultReason)
				throw (CDAPException) {
	cdapMessage.setFlags(flags);
	cdapMessage.setInvokeID(invokeID);
	cdapMessage.setOpCode(M_CANCELREAD_R);
	cdapMessage.setResult(result);
	cdapMessage.setResultReason(resultReason);
	CDAPMessageValidator::validate(cdapMessage);
}

/*	TODO: Implement these functions	*/
int CDAPMessage::get_absSyntax() const{
	return abs_syntax;
}

void CDAPMessage::set_absSyntax(int absSyntax){}

CDAPMessage::AuthTypes CDAPMessage::get_authMech() const{
	return auth_mech;
}

void CDAPMessage::setAuthMech(AuthTypes authMech){}

const AuthValue& CDAPMessage::getAuthValue() const{
	return auth_value;
}

void CDAPMessage::setAuthValue(AuthValue authValue){}
std::string CDAPMessage::getDestAEInst() const{
	return dest_ae_inst;
}
void CDAPMessage::setDestAEInst(std::string destAEInst){}
std::string CDAPMessage::getDestAEName() const{
	return dest_ae_name;
}
void CDAPMessage::setDestAEName(std::string destAEName){}
std::string CDAPMessage::getDestApInst() const{
	return dest_ap_inst;
}
void CDAPMessage::setDestApInst(std::string destApInst){}
std::string CDAPMessage::getDestApName() const{
	return dest_ap_name;
}
void CDAPMessage::setDestApName(std::string destApName){}
char* CDAPMessage::getFilter() const{
	return filter;
}
void CDAPMessage::setFilter(char filter[]){}
CDAPMessage::Flags CDAPMessage::getFlags() const{
	return flags;
}
void CDAPMessage::setFlags(Flags flags){}
int CDAPMessage::getInvokeID() const{
	return invoke_id;
}
void CDAPMessage::setInvokeID(int invokeID){}
std::string CDAPMessage::getObjClass() const{
	return obj_class;
}
void CDAPMessage::setObjClass(std::string objClass){}
long CDAPMessage::getObjInst() const{
	return obj_inst;
}
void CDAPMessage::setObjInst(long objInst){}
std::string CDAPMessage::getObjName() const{
	return obj_name;
}
void CDAPMessage::setObjName(std::string objName){}
const ObjectValue& CDAPMessage::getObjValue() const{
	return obj_value;
}
void CDAPMessage::setObjValue(const ObjectValue &objValue){}
CDAPMessage::Opcode CDAPMessage::getOpCode() const{
	return op_code;
}
void CDAPMessage::setOpCode(Opcode opCode){}
int CDAPMessage::getResult() const{
	return result;
}
void CDAPMessage::setResult(int result){}
std::string CDAPMessage::getResultReason() const{
	return result_reason;
}
void CDAPMessage::setResultReason(std::string resultReason){}
int CDAPMessage::getScope() const{
	return scope;
}
void CDAPMessage::setScope(int scope){}
std::string CDAPMessage::getSrcAEInst() const{
	return src_ae_inst;
}
void CDAPMessage::setSrcAEInst(std::string srcAEInst){}
std::string CDAPMessage::getSrcAEName() const{
	return src_ae_name;
}
void CDAPMessage::setSrcAEName(std::string srcAEName){}
std::string CDAPMessage::getSrcApInst() const{
	return src_ap_inst;
}
void CDAPMessage::setSrcApInst(std::string srcApInst){}
std::string CDAPMessage::getSrcApName() const{
	return src_ap_name;
}
void CDAPMessage::setSrcApName(std::string srcApName){}
long CDAPMessage::getVersion() const{
	return version;
}
void CDAPMessage::setVersion(long version){}
std::string toString(){
	return std::string("return");
}
}

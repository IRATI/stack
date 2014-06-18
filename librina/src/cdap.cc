//
// CDAP
//
//    Bernat Gaston         <bernat.gaston@i2cat.net>
//    Francesco Salvestrini <f.salvestrini@nextworks.it>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA  02110-1301  USA
//

#include "librina/cdap.h"
#include "cdap-impl.h"

namespace rina {

// CLASS AuthValue
AuthValue::AuthValue() {
}
AuthValue::AuthValue(const std::string &auth_name,
		const std::string &auth_password, const std::string &auth_other) {
	auth_name_ = auth_name;
	auth_password_ = auth_password;
	auth_other_ = auth_other;
}
const std::string AuthValue::get_auth_name() const {
	return auth_name_;
}
const std::string AuthValue::get_auth_password() const {
	return auth_password_;
}
const std::string AuthValue::get_auth_other() const {
	return auth_other_;
}
bool AuthValue::is_empty() const {
	if (auth_name_.empty() && auth_password_.empty()
					&& auth_other_.empty()) {
		return true;
	} else
		return false;
}
std::string AuthValue::to_string() const {
	return "Auth name: " + auth_name_ + "; Auth password: " + auth_password_
			+ "; Auth other: " + auth_other_;
}

// CLASS AbstractObjectValue
template<typename T>
AbstractObjectValue<T>::AbstractObjectValue() {
	empty_ = true;
}
template<typename T>
AbstractObjectValue<T>::AbstractObjectValue(T &value) {
	value_ = value;
	empty_ = false;
}
template<typename T>
AbstractObjectValue<T>::~AbstractObjectValue() {
}
template<typename T>
T AbstractObjectValue<T>::get_value() const {
	return value_;
}
template<typename T>
bool AbstractObjectValue<T>::is_empty() const {
	return empty_;
}

// CLASS IntObjectValue
IntObjectValue::IntObjectValue() :
		AbstractObjectValue() {
}
IntObjectValue::IntObjectValue(int value) :
		AbstractObjectValue(value) {
}
bool IntObjectValue::operator==(const AbstractObjectValue<int> &other) {
	return value_ == other.get_value();
}

// CLASS SIntObjectValue
SIntObjectValue::SIntObjectValue() :
		AbstractObjectValue() {
}
SIntObjectValue::SIntObjectValue(short int value) :
		AbstractObjectValue(value) {
}
bool SIntObjectValue::operator==(const AbstractObjectValue<short int> &other) {
	return value_ == other.get_value();
}

// CLASS LongObjectValue
LongObjectValue::LongObjectValue() :
		AbstractObjectValue() {
}
LongObjectValue::LongObjectValue(long long value) :
		AbstractObjectValue(value) {
}
bool LongObjectValue::operator==(const AbstractObjectValue<long long> &other) {
	return value_ == other.get_value();
}

// CLASS SLongObjectValue
SLongObjectValue::SLongObjectValue() :
		AbstractObjectValue() {
}
SLongObjectValue::SLongObjectValue(long value) :
		AbstractObjectValue(value) {
}
bool SLongObjectValue::operator==(const AbstractObjectValue<long> &other) {
	return value_ == other.get_value();
}

// CLASS StringObjectValue
StringObjectValue::StringObjectValue() :
		AbstractObjectValue() {
}
StringObjectValue::StringObjectValue(std::string value) :
		AbstractObjectValue(value) {
}
bool StringObjectValue::operator==(
		const AbstractObjectValue<std::string> &other) {
	return value_ == other.get_value();
}

// CLASS ByteArrayObjectValue
ByteArrayObjectValue::ByteArrayObjectValue() :
		AbstractObjectValue() {
}
ByteArrayObjectValue::ByteArrayObjectValue(char* value) :
		AbstractObjectValue(value) {
}
bool ByteArrayObjectValue::operator==(const AbstractObjectValue<char*> &other) {
	return value_ == other.get_value();
}

// CLASS FloatObjectValue
FloatObjectValue::FloatObjectValue() :
		AbstractObjectValue() {
}
FloatObjectValue::FloatObjectValue(float value) :
		AbstractObjectValue(value) {
}
bool FloatObjectValue::operator==(const AbstractObjectValue<float> &other) {
	return value_ == other.get_value();
}

// CLASS BooleanObjectValue
BooleanObjectValue::BooleanObjectValue() :
		AbstractObjectValue() {
}
BooleanObjectValue::BooleanObjectValue(bool value) :
		AbstractObjectValue(value) {
}
bool BooleanObjectValue::operator==(const AbstractObjectValue<bool> &other) {
	return value_ == other.get_value();
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
void CDAPMessageValidator::validate(const CDAPMessage *message) {
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

void CDAPMessageValidator::validateAbsSyntax(const CDAPMessage *message) {
	if (message->get_abs_syntax() == 0) {
		if (message->get_op_code() == CDAPMessage::M_CONNECT
				|| message->get_op_code() == CDAPMessage::M_CONNECT_R) {
			throw CDAPException(
					"AbsSyntax must be set for M_CONNECT and M_CONNECT_R messages");
		}
	} else {
		if ((message->get_op_code() != CDAPMessage::M_CONNECT)
				&& (message->get_op_code() != CDAPMessage::M_CONNECT_R)) {
			throw CDAPException(
					"AbsSyntax can only be set for M_CONNECT and M_CONNECT_R messages");
		}
	}
}

void CDAPMessageValidator::validateAuthMech(const CDAPMessage *message) {
	if (message->get_auth_mech() != CDAPMessage::AUTH_NONE) {
		if ((message->get_op_code() != CDAPMessage::M_CONNECT)
				&& message->get_op_code() != CDAPMessage::M_CONNECT_R) {
			throw CDAPException(
					"AuthMech can only be set for M_CONNECT and M_CONNECT_R messages");
		}
	}
}

void CDAPMessageValidator::validateAuthValue(const CDAPMessage *message) {
	if ((message->get_op_code() != CDAPMessage::M_CONNECT)
			&& (message->get_op_code() != CDAPMessage::M_CONNECT_R)) {
		throw CDAPException(
				"AuthValue can only be set for M_CONNECT and M_CONNECT_R messages");
	}
}

void CDAPMessageValidator::validateDestAEInst(const CDAPMessage *message) {
	if (!message->get_dest_ae_inst().empty()) {
		if ((message->get_op_code() != CDAPMessage::M_CONNECT)
				&& (message->get_op_code() != CDAPMessage::M_CONNECT_R)) {
			throw CDAPException(
					"dest_ae_inst can only be set for M_CONNECT and M_CONNECT_R messages");
		}
	}
}

void CDAPMessageValidator::validateDestAEName(const CDAPMessage *message) {
	if (!message->get_dest_ae_name().empty()) {
		if ((message->get_op_code() != CDAPMessage::M_CONNECT)
				&& (message->get_op_code() != CDAPMessage::M_CONNECT_R)) {
			throw CDAPException(
					"DestAEName can only be set for M_CONNECT and M_CONNECT_R messages");
		}
	}
}

void CDAPMessageValidator::validateDestApInst(const CDAPMessage *message) {
	if (!message->get_dest_ap_inst().empty()) {
		if (message->get_op_code() != CDAPMessage::M_CONNECT
				&& message->get_op_code() != CDAPMessage::M_CONNECT_R) {
			throw CDAPException(
					"DestApInst can only be set for M_CONNECT and M_CONNECT_R messages");
		}
	}
}

void CDAPMessageValidator::validateDestApName(const CDAPMessage *message) {
	if (message->get_dest_ap_name().empty()) {
		if (message->get_op_code() == CDAPMessage::M_CONNECT) {
			throw CDAPException(
					"DestApName must be set for the M_CONNECT message");
		} else if (message->get_op_code() == CDAPMessage::M_CONNECT_R) {
			//TODO not sure what to do
		}
	} else {
		if (message->get_op_code() != CDAPMessage::M_CONNECT
				&& message->get_op_code() != CDAPMessage::M_CONNECT_R) {
			throw CDAPException(
					"DestApName can only be set for M_CONNECT and M_CONNECT_R messages");
		}
	}
}

void CDAPMessageValidator::validateFilter(const CDAPMessage *message) {
	if (message->get_filter() != 0) {
		if (message->get_op_code() != CDAPMessage::M_CREATE
				&& message->get_op_code() != CDAPMessage::M_DELETE
				&& message->get_op_code() != CDAPMessage::M_READ
				&& message->get_op_code() != CDAPMessage::M_WRITE
				&& message->get_op_code() != CDAPMessage::M_START
				&& message->get_op_code() != CDAPMessage::M_STOP) {
			throw CDAPException(
					"The filter parameter can only be set for M_CREATE, M_DELETE, M_READ, M_WRITE, M_START or M_STOP messages");
		}
	}
}

void CDAPMessageValidator::validateInvokeID(const CDAPMessage *message) {
	if (message->get_invoke_id() == 0) {
		if (message->get_op_code() == CDAPMessage::M_CONNECT
				|| message->get_op_code() == CDAPMessage::M_CONNECT_R
				|| message->get_op_code() == CDAPMessage::M_RELEASE_R
				|| message->get_op_code() == CDAPMessage::M_CREATE_R
				|| message->get_op_code() == CDAPMessage::M_DELETE_R
				|| message->get_op_code() == CDAPMessage::M_READ_R
				|| message->get_op_code() == CDAPMessage::M_CANCELREAD
				|| message->get_op_code() == CDAPMessage::M_CANCELREAD_R
				|| message->get_op_code() == CDAPMessage::M_WRITE_R
				|| message->get_op_code() == CDAPMessage::M_START_R
				|| message->get_op_code() == CDAPMessage::M_STOP_R) {
			throw CDAPException("The invoke id parameter cannot be 0");
		}
	}
}

void CDAPMessageValidator::validateObjClass(const CDAPMessage *message) {
	if (!message->get_obj_class().empty()) {
		if (!message->get_obj_name().empty()) {
			throw CDAPException(
					"If the objClass parameter is set, the objName parameter also has to be set");
		}
		if (message->get_op_code() != CDAPMessage::M_CREATE
				&& message->get_op_code() != CDAPMessage::M_CREATE_R
				&& message->get_op_code() != CDAPMessage::M_DELETE
				&& message->get_op_code() != CDAPMessage::M_DELETE_R
				&& message->get_op_code() != CDAPMessage::M_READ
				&& message->get_op_code() != CDAPMessage::M_READ_R
				&& message->get_op_code() != CDAPMessage::M_WRITE
				&& message->get_op_code() != CDAPMessage::M_WRITE_R
				&& message->get_op_code() != CDAPMessage::M_START
				&& message->get_op_code() != CDAPMessage::M_STOP
				&& message->get_op_code() != CDAPMessage::M_START_R
				&& message->get_op_code() != CDAPMessage::M_STOP_R) {
			throw CDAPException(
					"The objClass parameter can only be set for M_CREATE, M_CREATE_R, M_DELETE, M_DELETE_R, M_READ, M_READ_R, M_WRITE, M_WRITE_R, M_START, M_STOP, M_START_R, M_STOP_R messages");
		}
	}
}

void CDAPMessageValidator::validateObjInst(const CDAPMessage *message) {
	if (message->get_obj_inst() != 0) {
		if (message->get_op_code() != CDAPMessage::M_CREATE
				&& message->get_op_code() != CDAPMessage::M_CREATE_R
				&& message->get_op_code() != CDAPMessage::M_DELETE
				&& message->get_op_code() != CDAPMessage::M_DELETE_R
				&& message->get_op_code() != CDAPMessage::M_READ
				&& message->get_op_code() != CDAPMessage::M_READ_R
				&& message->get_op_code() != CDAPMessage::M_WRITE
				&& message->get_op_code() != CDAPMessage::M_WRITE_R
				&& message->get_op_code() != CDAPMessage::M_START
				&& message->get_op_code() != CDAPMessage::M_STOP
				&& message->get_op_code() != CDAPMessage::M_START_R
				&& message->get_op_code() != CDAPMessage::M_STOP_R) {
			throw CDAPException(
					"The objInst parameter can only be set for M_CREATE, M_CREATE_R, M_DELETE, M_DELETE_R, M_READ, M_READ_R, M_WRITE, M_WRITE_R, M_START, M_START_R, M_STOP and M_STOP_R messages");
		}
	}
}

void CDAPMessageValidator::validateObjName(const CDAPMessage *message) {
	if (!message->get_obj_name().empty()) {
		if (message->get_obj_class().empty()) {
			throw new CDAPException(
					"If the objName parameter is set, the objClass parameter also has to be set");
		}
		if (message->get_op_code() != CDAPMessage::M_CREATE
				&& message->get_op_code() != CDAPMessage::M_CREATE_R
				&& message->get_op_code() != CDAPMessage::M_DELETE
				&& message->get_op_code() != CDAPMessage::M_DELETE_R
				&& message->get_op_code() != CDAPMessage::M_READ
				&& message->get_op_code() != CDAPMessage::M_READ_R
				&& message->get_op_code() != CDAPMessage::M_WRITE
				&& message->get_op_code() != CDAPMessage::M_WRITE_R
				&& message->get_op_code() != CDAPMessage::M_START
				&& message->get_op_code() != CDAPMessage::M_STOP
				&& message->get_op_code() != CDAPMessage::M_START_R
				&& message->get_op_code() != CDAPMessage::M_STOP_R) {
			throw CDAPException(
					"The objName parameter can only be set for M_CREATE, M_CREATE_R, M_DELETE, M_DELETE_R, M_READ, M_READ_R, M_WRITE, M_WRITE_R, M_START, M_STOP, M_START_R and M_STOP_R messages");
		}
	}
}

void CDAPMessageValidator::validateObjValue(const CDAPMessage *message) {
	if (message->get_obj_value()->is_empty()) {
		if (message->get_op_code() == CDAPMessage::M_WRITE) {
			throw CDAPException(
					"The objValue parameter must be set for M_WRITE messages");
		}
	} else {
		if (message->get_op_code() != CDAPMessage::M_CREATE
				&& message->get_op_code() != CDAPMessage::M_CREATE_R
				&& message->get_op_code() != CDAPMessage::M_READ_R
				&& message->get_op_code() != CDAPMessage::M_WRITE
				&& message->get_op_code() != CDAPMessage::M_START
				&& message->get_op_code() != CDAPMessage::M_STOP
				&& message->get_op_code() != CDAPMessage::M_START_R
				&& message->get_op_code() != CDAPMessage::M_STOP_R
				&& message->get_op_code() != CDAPMessage::M_WRITE_R
				&& message->get_op_code() != CDAPMessage::M_DELETE
				&& message->get_op_code() != CDAPMessage::M_READ) {
			throw CDAPException(
					"The objValue parameter can only be set for M_CREATE, M_DELETE, M_CREATE_R, M_READ_R, M_WRITE, M_START, M_START_R, M_STOP, M_STOP_R and M_WRITE_R messages");
		}
	}
}

void CDAPMessageValidator::validateOpcode(const CDAPMessage *message) {
	if (message->get_op_code() == CDAPMessage::NONE_OPCODE) {
		throw CDAPException("The opcode must be set for all the messages");
	}
}

void CDAPMessageValidator::validateResult(const CDAPMessage *message) {
	/* FIXME: Do something with sense */
	message->get_abs_syntax();
}

void CDAPMessageValidator::validateResultReason(const CDAPMessage *message) {
	if (!message->get_result_reason().empty()) {
		if (message->get_op_code() != CDAPMessage::M_CREATE_R
				&& message->get_op_code() != CDAPMessage::M_DELETE_R
				&& message->get_op_code() != CDAPMessage::M_READ_R
				&& message->get_op_code() != CDAPMessage::M_WRITE_R
				&& message->get_op_code() != CDAPMessage::M_CONNECT_R
				&& message->get_op_code() != CDAPMessage::M_RELEASE_R
				&& message->get_op_code() != CDAPMessage::M_CANCELREAD
				&& message->get_op_code() != CDAPMessage::M_CANCELREAD_R
				&& message->get_op_code() != CDAPMessage::M_START_R
				&& message->get_op_code() != CDAPMessage::M_STOP_R) {
			throw CDAPException(
					"The resultReason parameter can only be set for M_CREATE_R, M_DELETE_R, M_READ_R, M_WRITE_R, M_START_R, M_STOP_R, M_CONNECT_R, M_RELEASE_R, M_CANCELREAD and M_CANCELREAD_R messages");
		}
	}
}

void CDAPMessageValidator::validateScope(const CDAPMessage *message) {
	if (message->get_scope() != 0) {
		if (message->get_op_code() != CDAPMessage::M_CREATE
				&& message->get_op_code() != CDAPMessage::M_DELETE
				&& message->get_op_code() != CDAPMessage::M_READ
				&& message->get_op_code() != CDAPMessage::M_WRITE
				&& message->get_op_code() != CDAPMessage::M_START
				&& message->get_op_code() != CDAPMessage::M_STOP) {
			throw CDAPException(
					"The scope parameter can only be set for M_CREATE, M_DELETE, M_READ, M_WRITE, M_START or M_STOP messages");
		}
	}
}

void CDAPMessageValidator::validateSrcAEInst(const CDAPMessage *message) {
	if (!message->get_src_ae_inst().empty()) {
		if (message->get_op_code() != CDAPMessage::M_CONNECT
				&& message->get_op_code() != CDAPMessage::M_CONNECT_R) {
			throw CDAPException(
					"SrcAEInst can only be set for M_CONNECT and M_CONNECT_R messages");
		}
	}
}

void CDAPMessageValidator::validateSrcAEName(const CDAPMessage *message) {
	if (!message->get_src_ae_name().empty()) {

	}
	if (message->get_op_code() != CDAPMessage::M_CONNECT
			&& message->get_op_code() != CDAPMessage::M_CONNECT_R) {
		throw CDAPException(
				"SrcAEName can only be set for M_CONNECT and M_CONNECT_R messages");
	}
}

void CDAPMessageValidator::validateSrcApInst(const CDAPMessage *message) {
	if (message->get_src_ap_inst().empty()) {
		if (message->get_op_code() != CDAPMessage::M_CONNECT
				&& message->get_op_code() != CDAPMessage::M_CONNECT_R) {
			throw CDAPException(
					"SrcApInst can only be set for M_CONNECT and M_CONNECT_R messages");
		}
	}
}

void CDAPMessageValidator::validateSrcApName(const CDAPMessage *message) {
	if (message->get_src_ap_name().empty()) {
		if (message->get_op_code() == CDAPMessage::M_CONNECT) {
			throw CDAPException(
					"SrcApName must be set for the M_CONNECT message");
		} else if (message->get_op_code() == CDAPMessage::M_CONNECT_R) {
			//TODO not sure what to do
		}
	} else {
		if (message->get_op_code() != CDAPMessage::M_CONNECT
				&& message->get_op_code() != CDAPMessage::M_CONNECT_R) {
			throw CDAPException(
					"SrcApName can only be set for M_CONNECT and M_CONNECT_R messages");
		}
	}
}

void CDAPMessageValidator::validateVersion(const CDAPMessage *message) {
	if (message->get_version() == 0) {
		if (message->get_op_code() == CDAPMessage::M_CONNECT
				|| message->get_op_code() == CDAPMessage::M_CONNECT_R) {
			throw CDAPException(
					"Version must be set for M_CONNECT and M_CONNECT_R messages");
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
	obj_value_ = new ByteArrayObjectValue;
	op_code_ = NONE_OPCODE;
	result_ = 0;
	scope_ = 0;
	version_ = 0;
}
CDAPMessage::~CDAPMessage() {
	delete obj_value_;

}
const CDAPMessage* CDAPMessage::getOpenConnectionRequestMessage(
		AuthTypes auth_mech, const AuthValue &auth_value,
		const std::string &dest_ae_inst, const std::string &dest_ae_name,
		const std::string &dest_ap_inst, const std::string &dest_ap_name,
		const std::string &src_ae_inst, const std::string &src_ae_name,
		const std::string &src_ap_inst, const std::string &src_ap_name,
		int invoke_id) {
	CDAPMessage *cdap_message = new CDAPMessage();
	cdap_message->set_abs_syntax(ABSTRACT_SYNTAX_VERSION);
	cdap_message->set_auth_mech(auth_mech);
	cdap_message->set_auth_value(auth_value);
	cdap_message->set_dest_ae_inst(dest_ae_inst);
	cdap_message->set_dest_ae_name(dest_ae_name);
	cdap_message->set_dest_ap_inst(dest_ap_inst);
	cdap_message->set_dest_ap_name(dest_ap_name);
	cdap_message->set_invoke_id(invoke_id);
	cdap_message->set_op_code(M_CONNECT);
	cdap_message->set_src_ae_inst(src_ae_inst);
	cdap_message->set_src_ae_name(src_ae_name);
	cdap_message->set_src_ap_inst(src_ap_inst);
	cdap_message->set_src_ap_name(src_ap_name);
	cdap_message->set_version(1);
	CDAPMessageValidator::validate(cdap_message);
	return cdap_message;
}
const CDAPMessage* CDAPMessage::getOpenConnectionResponseMessage(
		AuthTypes auth_mech, const AuthValue &auth_value,
		const std::string &dest_ae_inst, const std::string &dest_ae_name,
		const std::string &dest_ap_inst, const std::string &dest_ap_name,
		int result, const std::string &result_reason,
		const std::string &src_ae_inst, const std::string &src_ae_name,
		const std::string &src_ap_inst, const std::string &src_ap_name,
		int invoke_id) {
	CDAPMessage *cdap_message = new CDAPMessage();
	cdap_message->set_abs_syntax(ABSTRACT_SYNTAX_VERSION);
	cdap_message->set_auth_mech(auth_mech);
	cdap_message->set_auth_value(auth_value);
	cdap_message->set_dest_ae_inst(dest_ae_inst);
	cdap_message->set_dest_ae_name(dest_ae_name);
	cdap_message->set_dest_ap_inst(dest_ap_inst);
	cdap_message->set_dest_ap_name(dest_ap_name);
	cdap_message->set_op_code(M_CONNECT_R);
	cdap_message->set_invoke_id(invoke_id);
	cdap_message->set_result(result);
	cdap_message->set_result_reason(result_reason);
	cdap_message->set_src_ae_inst(src_ae_inst);
	cdap_message->set_src_ae_name(src_ae_name);
	cdap_message->set_src_ap_inst(src_ap_inst);
	cdap_message->set_src_ap_name(src_ap_name);
	cdap_message->set_version(1);
	CDAPMessageValidator::validate(cdap_message);
	return cdap_message;
}
CDAPMessage* CDAPMessage::getReleaseConnectionRequestMessage(Flags flags) {
	CDAPMessage *cdap_message = new CDAPMessage();
	cdap_message->set_flags(flags);
	cdap_message->set_op_code(M_RELEASE);
	CDAPMessageValidator::validate(cdap_message);
	return cdap_message;
}
const CDAPMessage* CDAPMessage::getReleaseConnectionResponseMessage(Flags flags,
		int result, const std::string &result_reason, int invoke_id) {
	CDAPMessage *cdap_message = new CDAPMessage();
	cdap_message->set_flags(flags);
	cdap_message->set_invoke_id(invoke_id);
	cdap_message->set_op_code(M_RELEASE_R);
	cdap_message->set_result(result);
	cdap_message->set_result_reason(result_reason);
	CDAPMessageValidator::validate(cdap_message);
	return cdap_message;
}
CDAPMessage* CDAPMessage::getCreateObjectRequestMessage(char filter[],
		Flags flags, const std::string &obj_class, long obj_inst,
		const std::string &obj_name, ObjectValueInterface *obj_value,
		int scope) {
	CDAPMessage *cdap_message = new CDAPMessage();
	cdap_message->set_filter(filter);
	cdap_message->set_flags(flags);
	cdap_message->set_obj_class(obj_class);
	cdap_message->set_obj_inst(obj_inst);
	cdap_message->set_obj_name(obj_name);
	cdap_message->set_obj_value(obj_value);
	cdap_message->set_op_code(M_CREATE);
	cdap_message->set_scope(scope);
	CDAPMessageValidator::validate(cdap_message);
	return cdap_message;
}
const CDAPMessage* CDAPMessage::getCreateObjectResponseMessage(Flags flags,
		const std::string &obj_class, long obj_inst, const std::string &obj_name,
		ObjectValueInterface *obj_value, int result,
		const std::string &result_reason, int invoke_id) {
	CDAPMessage *cdap_message = new CDAPMessage();
	cdap_message->set_flags(flags);
	cdap_message->set_invoke_id(invoke_id);
	cdap_message->set_obj_class(obj_class);
	cdap_message->set_obj_inst(obj_inst);
	cdap_message->set_obj_name(obj_name);
	cdap_message->set_obj_value(obj_value);
	cdap_message->set_op_code(M_CREATE_R);
	cdap_message->set_result(result);
	cdap_message->set_result_reason(result_reason);
	CDAPMessageValidator::validate(cdap_message);
	return cdap_message;
}
CDAPMessage* CDAPMessage::getDeleteObjectRequestMessage(char filter[],
		Flags flags, const std::string &obj_class, long obj_inst,
		const std::string &obj_name, ObjectValueInterface *obj_value,
		int scope) {
	CDAPMessage *cdap_message = new CDAPMessage();
	cdap_message->set_filter(filter);
	cdap_message->set_flags(flags);
	cdap_message->set_obj_class(obj_class);
	cdap_message->set_obj_inst(obj_inst);
	cdap_message->set_obj_name(obj_name);
	cdap_message->set_obj_value(obj_value);
	cdap_message->set_op_code(M_DELETE);
	cdap_message->set_scope(scope);
	CDAPMessageValidator::validate(cdap_message);
	return cdap_message;
}
const CDAPMessage* CDAPMessage::getDeleteObjectResponseMessage(Flags flags,
		const std::string &obj_class, long obj_inst, const std::string &obj_name,
		int result, const std::string &result_reason, int invoke_id) {
	CDAPMessage *cdap_message = new CDAPMessage();
	cdap_message->set_flags(flags);
	cdap_message->set_invoke_id(invoke_id);
	cdap_message->set_obj_class(obj_class);
	cdap_message->set_obj_inst(obj_inst);
	cdap_message->set_obj_name(obj_name);
	cdap_message->set_op_code(M_DELETE_R);
	cdap_message->set_result(result);
	cdap_message->set_result_reason(result_reason);
	CDAPMessageValidator::validate(cdap_message);
	return cdap_message;
}
CDAPMessage* CDAPMessage::getStartObjectRequestMessage(char filter[],
		Flags flags, const std::string &obj_class,
		ObjectValueInterface *objValue, long obj_inst,
		const std::string &obj_name, int scope) {
	CDAPMessage *cdap_message = new CDAPMessage();
	cdap_message->set_filter(filter);
	cdap_message->set_flags(flags);
	cdap_message->set_obj_class(obj_class);
	cdap_message->set_obj_inst(obj_inst);
	cdap_message->set_obj_name(obj_name);
	cdap_message->set_obj_value(objValue);
	cdap_message->set_op_code(M_START);
	cdap_message->set_scope(scope);
	CDAPMessageValidator::validate(cdap_message);
	return cdap_message;
}
const CDAPMessage* CDAPMessage::getStartObjectResponseMessage(Flags flags,
		int result, const std::string &result_reason, int invoke_id) {
	CDAPMessage *cdap_message = new CDAPMessage();
	cdap_message->set_flags(flags);
	cdap_message->set_invoke_id(invoke_id);
	cdap_message->set_op_code(M_START_R);
	cdap_message->set_result(result);
	cdap_message->set_result_reason(result_reason);
	CDAPMessageValidator::validate(cdap_message);
	return cdap_message;
}
const CDAPMessage* CDAPMessage::getStartObjectResponseMessage(Flags flags,
		const std::string &object_class, ObjectValueInterface *obj_value,
		long obj_inst, const std::string &obj_name, int result,
		const std::string &result_reason, int invoke_id) {
	CDAPMessage *cdap_message = new CDAPMessage();
	cdap_message->set_flags(flags);
	cdap_message->set_invoke_id(invoke_id);
	cdap_message->set_op_code(M_START_R);
	cdap_message->set_obj_class(object_class);
	cdap_message->set_obj_inst(obj_inst);
	cdap_message->set_obj_name(obj_name);
	cdap_message->set_obj_value(obj_value);
	cdap_message->set_result(result);
	cdap_message->set_result_reason(result_reason);
	CDAPMessageValidator::validate(cdap_message);
	return cdap_message;
}
CDAPMessage* CDAPMessage::getStopObjectRequestMessage(char filter[],
		Flags flags, const std::string &obj_class,
		ObjectValueInterface *obj_value, long obj_inst,
		const std::string &obj_name, int scope) {
	CDAPMessage *cdap_message = new CDAPMessage();
	cdap_message->set_filter(filter);
	cdap_message->set_flags(flags);
	cdap_message->set_obj_class(obj_class);
	cdap_message->set_obj_inst(obj_inst);
	cdap_message->set_obj_name(obj_name);
	cdap_message->set_obj_value(obj_value);
	cdap_message->set_op_code(M_STOP);
	cdap_message->set_scope(scope);
	CDAPMessageValidator::validate(cdap_message);
	return cdap_message;
}
const CDAPMessage* CDAPMessage::getStopObjectResponseMessage(Flags flags,
		int result, const std::string &result_reason, int invoke_id) {
	CDAPMessage *cdap_message = new CDAPMessage();
	cdap_message->set_flags(flags);
	cdap_message->set_invoke_id(invoke_id);
	cdap_message->set_op_code(M_STOP_R);
	cdap_message->set_result(result);
	cdap_message->set_result_reason(result_reason);
	CDAPMessageValidator::validate(cdap_message);
	return cdap_message;
}
CDAPMessage* CDAPMessage::getReadObjectRequestMessage(char filter[],
		Flags flags, const std::string &obj_class, long obj_inst,
		const std::string &obj_name, int scope) {
	CDAPMessage *cdap_message = new CDAPMessage();
	cdap_message->set_filter(filter);
	cdap_message->set_flags(flags);
	cdap_message->set_obj_class(obj_class);
	cdap_message->set_obj_inst(obj_inst);
	cdap_message->set_obj_name(obj_name);
	cdap_message->set_op_code(M_READ);
	cdap_message->set_scope(scope);
	CDAPMessageValidator::validate(cdap_message);
	return cdap_message;
}
const CDAPMessage* CDAPMessage::getReadObjectResponseMessage(Flags flags,
		const std::string &obj_class, long obj_inst, const std::string &obj_name,
		ObjectValueInterface *obj_value, int result,
		const std::string &result_reason, int invoke_id) {
	CDAPMessage *cdap_message = new CDAPMessage();
	cdap_message->set_flags(flags);
	cdap_message->set_invoke_id(invoke_id);
	cdap_message->set_obj_class(obj_class);
	cdap_message->set_obj_inst(obj_inst);
	cdap_message->set_obj_name(obj_name);
	cdap_message->set_obj_value(obj_value);
	cdap_message->set_op_code(M_READ_R);
	cdap_message->set_result(result);
	cdap_message->set_result_reason(result_reason);
	CDAPMessageValidator::validate(cdap_message);
	return cdap_message;
}
CDAPMessage* CDAPMessage::getWriteObjectRequestMessage(char filter[],
		Flags flags, const std::string &obj_class, long obj_inst,
		ObjectValueInterface *obj_value, const std::string &obj_name,
		int scope) {
	CDAPMessage *cdap_message = new CDAPMessage();
	cdap_message->set_filter(filter);
	cdap_message->set_flags(flags);
	cdap_message->set_obj_class(obj_class);
	cdap_message->set_obj_inst(obj_inst);
	cdap_message->set_obj_name(obj_name);
	cdap_message->set_obj_value(obj_value);
	cdap_message->set_op_code(M_WRITE);
	cdap_message->set_scope(scope);
	CDAPMessageValidator::validate(cdap_message);
	return cdap_message;
}
const CDAPMessage* CDAPMessage::getWriteObjectResponseMessage(Flags flags,
		int result, int invoke_id, const std::string &result_reason) {
	CDAPMessage *cdap_message = new CDAPMessage();
	cdap_message->set_flags(flags);
	cdap_message->set_op_code(M_WRITE_R);
	cdap_message->set_result(result);
	cdap_message->set_result_reason(result_reason);
	cdap_message->set_invoke_id(invoke_id);
	CDAPMessageValidator::validate(cdap_message);
	return cdap_message;
}
const CDAPMessage* CDAPMessage::getCancelReadRequestMessage(Flags flags,
		int invoke_id) {
	CDAPMessage *cdap_message = new CDAPMessage();
	cdap_message->set_flags(flags);
	cdap_message->set_invoke_id(invoke_id);
	cdap_message->set_op_code(M_CANCELREAD);
	CDAPMessageValidator::validate(cdap_message);
	return cdap_message;
}
const CDAPMessage* CDAPMessage::getCancelReadResponseMessage(Flags flags,
		int invoke_id, int result, const std::string &result_reason) {
	CDAPMessage *cdap_message = new CDAPMessage();
	cdap_message->set_flags(flags);
	cdap_message->set_invoke_id(invoke_id);
	cdap_message->set_op_code(M_CANCELREAD_R);
	cdap_message->set_result(result);
	cdap_message->set_result_reason(result_reason);
	CDAPMessageValidator::validate(cdap_message);
	return cdap_message;
}
std::string CDAPMessage::to_string() const {
	std::stringstream ss;
	ss << std::endl << get_op_code() << std::endl;
	if (get_op_code() == CDAPMessage::M_CONNECT
			|| get_op_code() == CDAPMessage::M_CONNECT_R) {
		ss << "Abstract syntax: " << get_abs_syntax() << std::endl;
		ss << "Authentication mechanism: " << get_auth_mech() << std::endl;
		if (!get_auth_value().is_empty())
			ss << "Authentication value: " << get_auth_value().to_string()
					<< std::endl;
		if (!get_src_ap_name().empty())
			ss << "Source AP name: " << get_src_ap_name() << std::endl;
		if (!get_src_ap_inst().empty())
			ss << "Source AP instance: " << get_src_ap_inst() << std::endl;
		if (!get_src_ae_name().empty())
			ss << "Source AE name: " << get_src_ae_name() << std::endl;
		if (!get_src_ae_inst().empty())
			ss << "Source AE instance: " << get_src_ae_inst() << std::endl;
		if (!get_dest_ap_name().empty())
			ss << "Destination AP name: " << get_dest_ap_name() << std::endl;
		if (!get_dest_ap_inst().empty())
			ss << "Destination AP instance: " << get_dest_ap_inst()
					<< std::endl;
		if (!get_dest_ae_name().empty())
			ss << "Destination AE name: " << get_dest_ae_name() << std::endl;
		if (!get_dest_ae_inst().empty())
			ss << "Destination AE instance: " << get_dest_ae_inst()
					<< std::endl;
	}
	if (get_filter() != 0)
		ss << "Filter: " << get_filter() << std::endl;
	if (get_flags() != CDAPMessage::NONE_FLAGS)
		ss << "Flags: " << get_flags() << std::endl;
	if (get_invoke_id() != 0)
		ss << "Invoke id: " << get_invoke_id() << std::endl;
	if (!get_obj_class().empty())
		ss << "Object class: " << get_obj_class() << std::endl;
	if (!get_obj_name().empty())
		ss << "Object name: " << get_obj_name() << std::endl;
	if (get_obj_inst() != 0)
		ss << "Object instance: " + get_obj_inst() << std::endl;
	if (!get_obj_value()->is_empty())
		ss << "Object value: " << get_obj_value() << std::endl;
	if (get_op_code() == CDAPMessage::M_CONNECT_R
			|| get_op_code() == CDAPMessage::M_RELEASE_R
			|| get_op_code() == CDAPMessage::M_READ_R
			|| get_op_code() == CDAPMessage::M_WRITE_R
			|| get_op_code() == CDAPMessage::M_CANCELREAD_R
			|| get_op_code() == CDAPMessage::M_START_R
			|| get_op_code() == CDAPMessage::M_STOP_R
			|| get_op_code() == CDAPMessage::M_CREATE_R
			|| get_op_code() == CDAPMessage::M_DELETE_R) {
		ss << "Result: " << get_result() << std::endl;
		if (!get_result_reason().empty())
			ss << "Result Reason: " << get_result_reason() << std::endl;
	}
	if (get_op_code() == CDAPMessage::M_READ
			|| get_op_code() == CDAPMessage::M_WRITE
			|| get_op_code() == CDAPMessage::M_CANCELREAD
			|| get_op_code() == CDAPMessage::M_START
			|| get_op_code() == CDAPMessage::M_STOP
			|| get_op_code() == CDAPMessage::M_CREATE
			|| get_op_code() == CDAPMessage::M_DELETE) {
		ss << "Scope: " << get_scope() << std::endl;
	}
	if (get_version() != 0)
		ss << "Version: " << get_version() << std::endl;
	return ss.str();
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
void CDAPMessage::set_auth_value(const AuthValue &arg0) {
	auth_value_ = arg0;
}
const std::string& CDAPMessage::get_dest_ae_inst() const {
	return dest_ae_inst_;
}
void CDAPMessage::set_dest_ae_inst(const std::string &arg0) {
	dest_ae_inst_ = arg0;
}
const std::string& CDAPMessage::get_dest_ae_name() const {
	return dest_ae_name_;
}
void CDAPMessage::set_dest_ae_name(const std::string &arg0) {
	dest_ae_name_ = arg0;
}
const std::string& CDAPMessage::get_dest_ap_inst() const {
	return dest_ap_inst_;
}
void CDAPMessage::set_dest_ap_inst(const std::string &arg0) {
	dest_ap_inst_ = arg0;
}
const std::string& CDAPMessage::get_dest_ap_name() const {
	return dest_ap_name_;
}
void CDAPMessage::set_dest_ap_name(const std::string &arg0) {
	dest_ap_name_ = arg0;
}
const char* CDAPMessage::get_filter() const {
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
const std::string& CDAPMessage::get_obj_class() const {
	return obj_class_;
}
void CDAPMessage::set_obj_class(const std::string &arg0) {
	obj_class_ = arg0;
}
long CDAPMessage::get_obj_inst() const {
	return obj_inst_;
}
void CDAPMessage::set_obj_inst(long arg0) {
	obj_inst_ = arg0;
}
const std::string& CDAPMessage::get_obj_name() const {
	return obj_name_;
}
void CDAPMessage::set_obj_name(const std::string &arg0) {
	obj_name_ = arg0;
}
const ObjectValueInterface* CDAPMessage::get_obj_value() const {
	return obj_value_;
}
void CDAPMessage::set_obj_value(ObjectValueInterface *arg0) {
	delete obj_value_;
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
const std::string& CDAPMessage::get_result_reason() const {
	return result_reason_;
}
void CDAPMessage::set_result_reason(const std::string &arg0) {
	result_reason_ = arg0;
}
int CDAPMessage::get_scope() const {
	return scope_;
}
void CDAPMessage::set_scope(int arg0) {
	scope_ = arg0;
}
const std::string& CDAPMessage::get_src_ae_inst() const {
	return src_ae_inst_;
}
void CDAPMessage::set_src_ae_inst(const std::string &arg0) {
	src_ae_inst_ = arg0;
}
const std::string& CDAPMessage::get_src_ae_name() const {
	return src_ae_name_;
}
void CDAPMessage::set_src_ae_name(const std::string &arg0) {
	src_ae_name_ = arg0;
}
const std::string& CDAPMessage::get_src_ap_inst() const {
	return src_ap_inst_;
}
void CDAPMessage::set_src_ap_inst(const std::string &arg0) {
	src_ap_inst_ = arg0;
}
const std::string& CDAPMessage::get_src_ap_name() const {
	return src_ap_name_;
}

void CDAPMessage::set_src_ap_name(const std::string &arg0) {
	src_ap_name_ = arg0;
}
long CDAPMessage::get_version() const {
	return version_;
}
void CDAPMessage::set_version(long arg0) {
	version_ = arg0;
}

/*	class CDAPSessionDescriptor	*/
CDAPSessionDescriptor::CDAPSessionDescriptor(int port_id) {
	port_id_ = port_id;
}
CDAPSessionDescriptor::CDAPSessionDescriptor(int abs_syntax,
		CDAPMessage::AuthTypes auth_mech, AuthValue auth_value) {
	abs_syntax_ = abs_syntax;
	auth_mech_ = auth_mech;
	auth_value_ = auth_value;
}
const ApplicationProcessNamingInformation CDAPSessionDescriptor::get_source_application_process_naming_info() {
	ap_naming_info_ = ApplicationProcessNamingInformation(src_ap_name_,src_ap_inst_);
	if (!src_ae_name_.empty()) {
		ap_naming_info_.setEntityName(src_ae_name_);
	}

	if (!src_ae_inst_.empty()) {
		ap_naming_info_.setEntityInstance(src_ae_inst_);
	}
	return ap_naming_info_;
}
const ApplicationProcessNamingInformation CDAPSessionDescriptor::get_destination_application_process_naming_info() {
	ap_naming_info_ = ApplicationProcessNamingInformation(dest_ap_name_,dest_ap_inst_);
	if (!dest_ae_name_.empty()) {
		ap_naming_info_.setEntityName(dest_ae_name_);
	}

	if (!dest_ae_inst_.empty()) {
		ap_naming_info_.setEntityInstance(dest_ae_inst_);
	}
	return ap_naming_info_;
}

void CDAPSessionDescriptor::set_dest_ae_inst(const std::string *dest_ae_inst) {
	dest_ae_inst_ = *dest_ae_inst;
}
void CDAPSessionDescriptor::set_dest_ae_name(const std::string *dest_ae_name) {
	dest_ae_name_ = *dest_ae_name;
}
void CDAPSessionDescriptor::set_dest_ap_inst(const std::string *dest_ap_inst) {
	dest_ap_inst_ = *dest_ap_inst;
}
void CDAPSessionDescriptor::set_dest_ap_name(const std::string *dest_ap_name) {
	dest_ap_name_ = *dest_ap_name;
}
void CDAPSessionDescriptor::set_src_ae_inst(const std::string *src_ae_inst) {
	src_ae_inst_ = *src_ae_inst;
}
void CDAPSessionDescriptor::set_src_ae_name(const std::string *src_ae_name) {
	src_ae_name_ = *src_ae_name;
}
void CDAPSessionDescriptor::set_src_ap_inst(const std::string *src_ap_inst) {
	src_ap_inst_ = *src_ap_inst;
}
void CDAPSessionDescriptor::set_src_ap_name(const std::string *src_ap_name) {
	src_ap_name_ = *src_ap_name;
}
void CDAPSessionDescriptor::set_version(const long version) {
	version_ = version;
}
void CDAPSessionDescriptor::set_ap_naming_info(const
		ApplicationProcessNamingInformation* ap_naming_info) {
	ap_naming_info_ = *ap_naming_info;
}
int CDAPSessionDescriptor::get_port_id() const {
	return port_id_;
}
std::string CDAPSessionDescriptor::get_dest_ap_name() const {
	return dest_ap_name_;
}
const ApplicationProcessNamingInformation& CDAPSessionDescriptor::get_ap_naming_info() const {
	return ap_naming_info_;
}

//	CLASS RIBDaemonException
RIBDaemonException::RIBDaemonException(ErrorCode arg0) :
		Exception("RIBDaemon caused an exception") {
	error_code_ = arg0;
}

RIBDaemonException::RIBDaemonException(ErrorCode arg0, const char* arg1) :
		Exception(arg1) {
	error_code_ = arg0;
}

// CLASS WireMessageProviderFactory
WireMessageProviderInterface* WireMessageProviderFactory::createWireMessageProvider() {
	return new GPBWireMessageProvider();
}

// CLASS CDAPSessionManagerFactory
CDAPSessionManagerInterface* CDAPSessionManagerFactory::createCDAPSessionManager(
		WireMessageProviderFactory *wire_message_provider_factory,
		long timeout) {
	return new CDAPSessionManager(wire_message_provider_factory, timeout);
}

}

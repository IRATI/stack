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

#include <iostream>

namespace rina {
//CLASS CDAP Error Code
const int CDAPErrorCodes::CONNECTION_REJECTED_ERROR = -1;

// CLASS AuthPolicy
std::string AuthPolicy::to_string() const {
	std::stringstream ss;
	ss << "Policy name: " << name_ << std::endl;
	ss << "Supported versions: ";
	for (std::list<std::string>::const_iterator it = versions_.begin();
			it != versions_.end(); ++it){
		ss << *it << ";";
	}
	ss << std::endl;

	return ss.str();
}

// CLASS IntObjectValue
IntObjectValue::IntObjectValue() :
		AbstractObjectValue() {
}
IntObjectValue::IntObjectValue(int value) :
		AbstractObjectValue(value) {
}
bool IntObjectValue::operator==(const AbstractObjectValue<int> &other) {
	return value_ == *(int*)other.get_value();
}
ObjectValueInterface::types IntObjectValue::isType() const {
	return ObjectValueInterface::inttype;
}

// CLASS SIntObjectValue
SIntObjectValue::SIntObjectValue() :
		AbstractObjectValue() {
}
SIntObjectValue::SIntObjectValue(short int value) :
		AbstractObjectValue(value) {
}
bool SIntObjectValue::operator==(const AbstractObjectValue<short int> &other) {
	return value_ == *(short int*)other.get_value();
}
ObjectValueInterface::types SIntObjectValue::isType() const {
	return ObjectValueInterface::sinttype;
}

// CLASS LongObjectValue
LongObjectValue::LongObjectValue() :
		AbstractObjectValue() {
}
LongObjectValue::LongObjectValue(long long value) :
		AbstractObjectValue(value) {
}
bool LongObjectValue::operator==(const AbstractObjectValue<long long> &other) {
	return value_ == *(long long*)other.get_value();
}
ObjectValueInterface::types LongObjectValue::isType() const {
	return ObjectValueInterface::longtype;
}

// CLASS SLongObjectValue
SLongObjectValue::SLongObjectValue() :
		AbstractObjectValue() {
}
SLongObjectValue::SLongObjectValue(long value) :
		AbstractObjectValue(value) {
}
bool SLongObjectValue::operator==(const AbstractObjectValue<long> &other) {
	return value_ == *(long*)other.get_value();
}
ObjectValueInterface::types SLongObjectValue::isType() const {
	return ObjectValueInterface::slongtype;
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
	return value_ == *(std::string*)other.get_value();
}
ObjectValueInterface::types StringObjectValue::isType() const {
	return ObjectValueInterface::stringtype;
}

// CLASS ByteArrayObjectValue
ByteArrayObjectValue::ByteArrayObjectValue() :
		AbstractObjectValue() {
}
ByteArrayObjectValue::~ByteArrayObjectValue() {
}
ByteArrayObjectValue::ByteArrayObjectValue(SerializedObject value) :
		AbstractObjectValue(value) {
}
bool ByteArrayObjectValue::operator==(const AbstractObjectValue<SerializedObject> &other) {
	return value_.message_ == (*(SerializedObject*)other.get_value()).message_;
}
ObjectValueInterface::types ByteArrayObjectValue::isType() const {
	return ObjectValueInterface::bytetype;
}

// CLASS FloatObjectValue
FloatObjectValue::FloatObjectValue() :
		AbstractObjectValue() {
}
FloatObjectValue::FloatObjectValue(float value) :
		AbstractObjectValue(value) {
}
bool FloatObjectValue::operator==(const AbstractObjectValue<float> &other) {
	return value_ == *(float*)other.get_value();
}
ObjectValueInterface::types FloatObjectValue::isType() const {
	return ObjectValueInterface::floattype;
}

// CLASS BooleanObjectValue
DoubleObjectValue::DoubleObjectValue() :
		AbstractObjectValue() {
}
DoubleObjectValue::DoubleObjectValue(double value) :
		AbstractObjectValue(value) {
}
bool DoubleObjectValue::operator==(const AbstractObjectValue<double> &other) {
	return value_ == *(double*)other.get_value();
}
ObjectValueInterface::types DoubleObjectValue::isType() const {
	return ObjectValueInterface::doubletype;
}

// CLASS BooleanObjectValue
BooleanObjectValue::BooleanObjectValue() :
		AbstractObjectValue() {
}
BooleanObjectValue::BooleanObjectValue(bool value) :
		AbstractObjectValue(value) {
}
bool BooleanObjectValue::operator==(const AbstractObjectValue<bool> &other) {
	return value_ == *(bool*)other.get_value();
}
ObjectValueInterface::types BooleanObjectValue::isType() const {
	return ObjectValueInterface::booltype;
}

// CLASS CDAPException
CDAPException::CDAPException() :
		Exception("CDAP message caused an Exception") {
	result_ = OTHER;
}

CDAPException::CDAPException(std::string arg0) :
		Exception(arg0.c_str()) {
	result_ = OTHER;
}
CDAPException::CDAPException(ErrorCode result, std::string arg1) :
		Exception(arg1.c_str()) {
	result_ = result;
}
CDAPException::ErrorCode CDAPException::get_result() const {
	return result_;
}

/* CLASS CDAPMessageValidator */
void CDAPMessageValidator::validate(const CDAPMessage *message) {
	validateAbsSyntax(message);
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
		if (message->get_obj_name().empty()) {
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
	if (message->get_obj_value() == 0 || message->get_obj_value()->is_empty()) {
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
		if (message->get_op_code() != CDAPMessage::M_CONNECT
				&& message->get_op_code() != CDAPMessage::M_CONNECT_R) {
			throw CDAPException(
					"SrcAEName can only be set for M_CONNECT and M_CONNECT_R messages");
		}
	}
}

void CDAPMessageValidator::validateSrcApInst(const CDAPMessage *message) {
	if (!message->get_src_ap_inst().empty()) {
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
	filter_ = 0;
	flags_ = NONE_FLAGS;
	invoke_id_ = 0;
	obj_inst_ = 0;
	obj_value_ = 0;
	op_code_ = NONE_OPCODE;
	result_ = 0;
	scope_ = 0;
	version_ = 0;
}
CDAPMessage::~CDAPMessage() {
	if (obj_value_) {
		delete obj_value_;
		obj_value_ = 0;
	}

	if (filter_) {
		delete filter_;
		filter_ = 0;
	}
}

CDAPMessage* CDAPMessage::getOpenConnectionRequestMessage(
		const AuthPolicy &auth_policy,
		const std::string &dest_ae_inst, const std::string &dest_ae_name,
		const std::string &dest_ap_inst, const std::string &dest_ap_name,
		const std::string &src_ae_inst, const std::string &src_ae_name,
		const std::string &src_ap_inst, const std::string &src_ap_name,
		int invoke_id) {
	CDAPMessage *cdap_message = new CDAPMessage();
	cdap_message->set_abs_syntax(ABSTRACT_SYNTAX_VERSION);
	cdap_message->set_auth_policy(auth_policy);
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
	return cdap_message;
}

CDAPMessage* CDAPMessage::getOpenConnectionResponseMessage(
		const AuthPolicy &auth_policy,
		const std::string &dest_ae_inst, const std::string &dest_ae_name,
		const std::string &dest_ap_inst, const std::string &dest_ap_name,
		int result, const std::string &result_reason,
		const std::string &src_ae_inst, const std::string &src_ae_name,
		const std::string &src_ap_inst, const std::string &src_ap_name,
		int invoke_id) {
	CDAPMessage *cdap_message = new CDAPMessage();
	cdap_message->set_abs_syntax(ABSTRACT_SYNTAX_VERSION);
	cdap_message->set_auth_policy(auth_policy);
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
	return cdap_message;
}

CDAPMessage* CDAPMessage::getReleaseConnectionRequestMessage(Flags flags) {
	CDAPMessage *cdap_message = new CDAPMessage();
	cdap_message->set_flags(flags);
	cdap_message->set_op_code(M_RELEASE);
	return cdap_message;
}

CDAPMessage* CDAPMessage::getReleaseConnectionResponseMessage(Flags flags,
		int result, const std::string &result_reason, int invoke_id) {
	CDAPMessage *cdap_message = new CDAPMessage();
	cdap_message->set_flags(flags);
	cdap_message->set_invoke_id(invoke_id);
	cdap_message->set_op_code(M_RELEASE_R);
	cdap_message->set_result(result);
	cdap_message->set_result_reason(result_reason);
	return cdap_message;
}

CDAPMessage* CDAPMessage::getCreateObjectRequestMessage(char * filter,
		Flags flags, const std::string &obj_class, long obj_inst,
		const std::string &obj_name,
		int scope) {
	CDAPMessage *cdap_message = new CDAPMessage();
	cdap_message->set_filter(filter);
	cdap_message->set_flags(flags);
	cdap_message->set_obj_class(obj_class);
	cdap_message->set_obj_inst(obj_inst);
	cdap_message->set_obj_name(obj_name);
	cdap_message->set_op_code(M_CREATE);
	cdap_message->set_scope(scope);
	return cdap_message;
}

CDAPMessage* CDAPMessage::getCreateObjectResponseMessage(Flags flags,
		const std::string &obj_class, long obj_inst, const std::string &obj_name,
		int result,
		const std::string &result_reason, int invoke_id) {
	CDAPMessage *cdap_message = new CDAPMessage();
	cdap_message->set_flags(flags);
	cdap_message->set_invoke_id(invoke_id);
	cdap_message->set_obj_class(obj_class);
	cdap_message->set_obj_inst(obj_inst);
	cdap_message->set_obj_name(obj_name);
	cdap_message->set_op_code(M_CREATE_R);
	cdap_message->set_result(result);
	cdap_message->set_result_reason(result_reason);
	return cdap_message;
}

CDAPMessage* CDAPMessage::getDeleteObjectRequestMessage(char * filter,
		Flags flags, const std::string &obj_class, long obj_inst,
		const std::string &obj_name, int scope) {
	CDAPMessage *cdap_message = new CDAPMessage();
	cdap_message->set_filter(filter);
	cdap_message->set_flags(flags);
	cdap_message->set_obj_class(obj_class);
	cdap_message->set_obj_inst(obj_inst);
	cdap_message->set_obj_name(obj_name);
	cdap_message->set_op_code(M_DELETE);
	cdap_message->set_scope(scope);
	return cdap_message;
}

CDAPMessage* CDAPMessage::getDeleteObjectResponseMessage(Flags flags,
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
	return cdap_message;
}

CDAPMessage* CDAPMessage::getStartObjectRequestMessage(char * filter,
		Flags flags, const std::string &obj_class, long obj_inst,
		const std::string &obj_name, int scope) {
	CDAPMessage *cdap_message = new CDAPMessage();
	cdap_message->set_filter(filter);
	cdap_message->set_flags(flags);
	cdap_message->set_obj_class(obj_class);
	cdap_message->set_obj_inst(obj_inst);
	cdap_message->set_obj_name(obj_name);
	cdap_message->set_op_code(M_START);
	cdap_message->set_scope(scope);
	return cdap_message;
}

CDAPMessage* CDAPMessage::getStartObjectResponseMessage(Flags flags,
		int result, const std::string &result_reason, int invoke_id) {
	CDAPMessage *cdap_message = new CDAPMessage();
	cdap_message->set_flags(flags);
	cdap_message->set_invoke_id(invoke_id);
	cdap_message->set_op_code(M_START_R);
	cdap_message->set_result(result);
	cdap_message->set_result_reason(result_reason);
	return cdap_message;
}

CDAPMessage* CDAPMessage::getStartObjectResponseMessage(Flags flags,
		const std::string &object_class,
		long obj_inst, const std::string &obj_name, int result,
		const std::string &result_reason, int invoke_id) {
	CDAPMessage *cdap_message = new CDAPMessage();
	cdap_message->set_flags(flags);
	cdap_message->set_invoke_id(invoke_id);
	cdap_message->set_op_code(M_START_R);
	cdap_message->set_obj_class(object_class);
	cdap_message->set_obj_inst(obj_inst);
	cdap_message->set_obj_name(obj_name);
	cdap_message->set_result(result);
	cdap_message->set_result_reason(result_reason);
	return cdap_message;
}

CDAPMessage* CDAPMessage::getStopObjectRequestMessage(char * filter,
		Flags flags, const std::string &obj_class, long obj_inst,
		const std::string &obj_name, int scope) {
	CDAPMessage *cdap_message = new CDAPMessage();
	cdap_message->set_filter(filter);
	cdap_message->set_flags(flags);
	cdap_message->set_obj_class(obj_class);
	cdap_message->set_obj_inst(obj_inst);
	cdap_message->set_obj_name(obj_name);
	cdap_message->set_op_code(M_STOP);
	cdap_message->set_scope(scope);
	return cdap_message;
}

CDAPMessage* CDAPMessage::getStopObjectResponseMessage(Flags flags,
		int result, const std::string &result_reason, int invoke_id) {
	CDAPMessage *cdap_message = new CDAPMessage();
	cdap_message->set_flags(flags);
	cdap_message->set_invoke_id(invoke_id);
	cdap_message->set_op_code(M_STOP_R);
	cdap_message->set_result(result);
	cdap_message->set_result_reason(result_reason);
	return cdap_message;
}

CDAPMessage* CDAPMessage::getReadObjectRequestMessage(char * filter,
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
	return cdap_message;
}

CDAPMessage* CDAPMessage::getReadObjectResponseMessage(Flags flags,
		const std::string &obj_class, long obj_inst, const std::string &obj_name,
		int result,
		const std::string &result_reason, int invoke_id) {
	CDAPMessage *cdap_message = new CDAPMessage();
	cdap_message->set_flags(flags);
	cdap_message->set_invoke_id(invoke_id);
	cdap_message->set_obj_class(obj_class);
	cdap_message->set_obj_inst(obj_inst);
	cdap_message->set_obj_name(obj_name);
	cdap_message->set_op_code(M_READ_R);
	cdap_message->set_result(result);
	cdap_message->set_result_reason(result_reason);
	return cdap_message;
}

CDAPMessage* CDAPMessage::getWriteObjectRequestMessage(char * filter,
		Flags flags, const std::string &obj_class, long obj_inst,
		const std::string &obj_name,
		int scope) {
	CDAPMessage *cdap_message = new CDAPMessage();
	cdap_message->set_filter(filter);
	cdap_message->set_flags(flags);
	cdap_message->set_obj_class(obj_class);
	cdap_message->set_obj_inst(obj_inst);
	cdap_message->set_obj_name(obj_name);
	cdap_message->set_op_code(M_WRITE);
	cdap_message->set_scope(scope);
	return cdap_message;
}

CDAPMessage* CDAPMessage::getWriteObjectResponseMessage(Flags flags,
		int result, int invoke_id, const std::string &result_reason) {
	CDAPMessage *cdap_message = new CDAPMessage();
	cdap_message->set_flags(flags);
	cdap_message->set_op_code(M_WRITE_R);
	cdap_message->set_result(result);
	cdap_message->set_result_reason(result_reason);
	cdap_message->set_invoke_id(invoke_id);
	return cdap_message;
}

CDAPMessage* CDAPMessage::getCancelReadRequestMessage(Flags flags,
		int invoke_id) {
	CDAPMessage *cdap_message = new CDAPMessage();
	cdap_message->set_flags(flags);
	cdap_message->set_invoke_id(invoke_id);
	cdap_message->set_op_code(M_CANCELREAD);
	return cdap_message;
}

CDAPMessage* CDAPMessage::getCancelReadResponseMessage(Flags flags,
		int invoke_id, int result, const std::string &result_reason) {
	CDAPMessage *cdap_message = new CDAPMessage();
	cdap_message->set_flags(flags);
	cdap_message->set_invoke_id(invoke_id);
	cdap_message->set_op_code(M_CANCELREAD_R);
	cdap_message->set_result(result);
	cdap_message->set_result_reason(result_reason);
	return cdap_message;
}

CDAPMessage* CDAPMessage::getRequestMessage(Opcode opcode, char * filter,
		Flags flags, const std::string &obj_class, long obj_inst,
		const std::string &obj_name, int scope)
{
	CDAPMessage *cdap_message = new CDAPMessage();
	cdap_message->set_filter(filter);
	cdap_message->set_flags(flags);
	cdap_message->set_obj_class(obj_class);
	cdap_message->set_obj_inst(obj_inst);
	cdap_message->set_obj_name(obj_name);
	cdap_message->set_op_code(opcode);
	cdap_message->set_scope(scope);
	return cdap_message;
}

CDAPMessage* CDAPMessage::getResponseMessage(Opcode opcode, Flags flags,
		const std::string &obj_class, long obj_inst,
		const std::string &obj_name, int result,
		const std::string &result_reason, int invoke_id)
{
	CDAPMessage *cdap_message = new CDAPMessage();
	cdap_message->set_flags(flags);
	cdap_message->set_invoke_id(invoke_id);
	cdap_message->set_obj_class(obj_class);
	cdap_message->set_obj_inst(obj_inst);
	cdap_message->set_obj_name(obj_name);
	cdap_message->set_op_code(opcode);
	cdap_message->set_result(result);
	cdap_message->set_result_reason(result_reason);
	return cdap_message;
}

bool CDAPMessage::is_request_message() const
{
	if (op_code_ == M_CONNECT || op_code_ == M_RELEASE ||
			op_code_ == M_CREATE || op_code_ == M_DELETE ||
			op_code_ == M_READ || op_code_ == M_WRITE ||
			op_code_ == M_START || op_code_ == M_STOP ||
			op_code_ == M_CANCELREAD) {
		return true;
	} else {
		return false;
	}
}

std::string CDAPMessage::to_string() const {
	std::stringstream ss;
	ss << std::endl << opcodeToString(op_code_) << std::endl;
	if (op_code_ == CDAPMessage::M_CONNECT
			|| op_code_ == CDAPMessage::M_CONNECT_R) {
		if (abs_syntax_ != 0)
			ss << "Abstract syntax: " << abs_syntax_ << std::endl;
		ss << "Authentication policy: " << auth_policy_.to_string() << std::endl;
		if (!src_ap_name_.empty())
			ss << "Source AP name: " << src_ap_name_ << std::endl;
		if (!src_ap_inst_.empty())
			ss << "Source AP instance: " << src_ap_inst_ << std::endl;
		if (!src_ae_name_.empty())
			ss << "Source AE name: " << src_ae_name_ << std::endl;
		if (!src_ae_inst_.empty())
			ss << "Source AE instance: " << src_ae_inst_ << std::endl;
		if (!dest_ap_name_.empty())
			ss << "Destination AP name: " << dest_ap_name_ << std::endl;
		if (!dest_ap_inst_.empty())
			ss << "Destination AP instance: " << dest_ap_inst_<< std::endl;
		if (!dest_ae_name_.empty())
			ss << "Destination AE name: " << dest_ae_name_ << std::endl;
		if (!dest_ae_inst_.empty())
			ss << "Destination AE instance: " << dest_ae_inst_ << std::endl;
	}
	if (filter_ != 0)
		ss << "Filter: " << filter_ << std::endl;
	if (flags_ != CDAPMessage::NONE_FLAGS)
		ss << "Flags: " << flags_ << std::endl;
	if (invoke_id_ != 0)
		ss << "Invoke id: " << invoke_id_ << std::endl;
	if (!obj_class_.empty())
		ss << "Object class: " << obj_class_ << std::endl;
	if (!obj_name_.empty())
		ss << "Object name: " << obj_name_ << std::endl;
	if (obj_inst_ != 0)
		ss << "Object instance: " << obj_inst_ << std::endl;
	if (obj_value_ != 0 && !obj_value_->is_empty())
		ss << "Object value: " << obj_value_ << std::endl;
	if (op_code_ == CDAPMessage::M_CONNECT_R
			|| op_code_ == CDAPMessage::M_RELEASE_R
			|| op_code_ == CDAPMessage::M_READ_R
			|| op_code_ == CDAPMessage::M_WRITE_R
			|| op_code_ == CDAPMessage::M_CANCELREAD_R
			|| op_code_ == CDAPMessage::M_START_R
			|| op_code_ == CDAPMessage::M_STOP_R
			|| op_code_ == CDAPMessage::M_CREATE_R
			|| op_code_ == CDAPMessage::M_DELETE_R) {
		ss << "Result: " << result_ << std::endl;
		if (!result_reason_.empty())
			ss << "Result Reason: " << result_reason_ << std::endl;
	}
	if (op_code_ == CDAPMessage::M_READ
			|| op_code_ == CDAPMessage::M_WRITE
			|| op_code_ == CDAPMessage::M_CANCELREAD
			|| op_code_ == CDAPMessage::M_START
			|| op_code_ == CDAPMessage::M_STOP
			|| op_code_ == CDAPMessage::M_CREATE
			|| op_code_ == CDAPMessage::M_DELETE) {
		ss << "Scope: " << scope_ << std::endl;
	}
	if (version_ != 0)
		ss << "Version: " << version_ << std::endl;
	return ss.str();
}

int CDAPMessage::get_abs_syntax() const {
	return abs_syntax_;
}
void CDAPMessage::set_abs_syntax(int arg0) {
	abs_syntax_ = arg0;
}
const AuthPolicy& CDAPMessage::get_auth_policy() const {
	return auth_policy_;
}
void CDAPMessage::set_auth_policy(const AuthPolicy& policy) {
	auth_policy_ = policy;
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
void CDAPMessage::set_filter(char * filter) {
	filter_ = filter;
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

const std::string CDAPMessage::opcodeToString(Opcode opcode) {
	std::string result;

	switch(opcode) {
	case M_CONNECT:
		result = "0_M_CONNECT";
		break;
	case M_CONNECT_R:
		result = "1_M_CONNECT_R";
		break;
	case M_RELEASE:
		result = "2_M_RELEASE";
		break;
	case M_RELEASE_R:
		result = "3_M_RELEASER";
		break;
	case M_CREATE:
		result = "4_M_CREATE";
		break;
	case M_CREATE_R:
		result = "5_M_CREATE_R";
		break;
	case M_DELETE:
		result = "6_M_DELETE";
		break;
	case M_DELETE_R:
		result = "7_M_DELETE_R";
		break;
	case M_READ:
		result = "8_M_READ";
		break;
	case M_READ_R:
		result = "9_M_READ_R";
		break;
	case M_CANCELREAD:
		result = "10_M_CANCELREAD";
		break;
	case M_CANCELREAD_R:
		result = "11_M_CANCELREAD_R";
		break;
	case M_WRITE:
		result = "12_M_WRITE";
		break;
	case M_WRITE_R:
		result = "13_M_WRITE_R";
		break;
	case M_START:
		result = "14_M_START";
		break;
	case M_START_R:
		result = "15_M_START_r";
		break;
	case M_STOP:
		result = "16_M_STOP";
		break;
	case M_STOP_R:
		result = "17_M_STOP_R";
		break;
	case NONE_OPCODE:
		result = "18_NON_OPCODE";
		break;
	default:
		result = "Wrong operation code";
	}

	return result;
}

/*	class CDAPSessionDescriptor	*/
CDAPSessionDescriptor::CDAPSessionDescriptor() {
	port_id_ = 0;
	version_ = 0;
	abs_syntax_ = 0;
}
CDAPSessionDescriptor::CDAPSessionDescriptor(int port_id) {
	port_id_ = port_id;
	version_ = 0;
	abs_syntax_ = 0;
}
CDAPSessionDescriptor::CDAPSessionDescriptor(int abs_syntax,
		const AuthPolicy& policy) {
	abs_syntax_ = abs_syntax;
	auth_policy_ = policy;
	port_id_ = 0;
	version_ = 0;
}
CDAPSessionDescriptor::~CDAPSessionDescriptor() {
}
const ApplicationProcessNamingInformation CDAPSessionDescriptor::get_source_application_process_naming_info() {
	ap_naming_info_ = ApplicationProcessNamingInformation(src_ap_name_,src_ap_inst_);
	if (!src_ae_name_.empty()) {
		ap_naming_info_.entityName = src_ae_name_;
	}

	if (!src_ae_inst_.empty()) {
		ap_naming_info_.entityInstance = src_ae_inst_;
	}
	return ap_naming_info_;
}
const ApplicationProcessNamingInformation CDAPSessionDescriptor::get_destination_application_process_naming_info() {
	ap_naming_info_ = ApplicationProcessNamingInformation(dest_ap_name_,dest_ap_inst_);
	if (!dest_ae_name_.empty()) {
		ap_naming_info_.entityName = dest_ae_name_;
	}

	if (!dest_ae_inst_.empty()) {
		ap_naming_info_.entityInstance = dest_ae_inst_;
	}
	return ap_naming_info_;
}
void CDAPSessionDescriptor::set_abs_syntax(const int abs_syntax) {
	abs_syntax_ = abs_syntax;
}
void CDAPSessionDescriptor::set_auth_policy(const AuthPolicy& policy) {
	auth_policy_ = policy;
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

// CLASS CDAPMessageInterface
const std::string CDAPSessionInterface::SESSION_STATE_NONE = "NONE";
const std::string CDAPSessionInterface::SESSION_STATE_AWAIT_CON = "AWAIT CON";
const std::string CDAPSessionInterface::SESSION_STATE_CON = "CONNECTED";
const std::string CDAPSessionInterface::SESSION_STATE_AWAIT_CLOSE = "AWAIT CLOSE";

}

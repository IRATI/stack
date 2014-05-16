#include "exceptions.h"

namespace rina {
/*	CLASS ApplicationProcessException */
const int ApplicationProcessException::UNEXISTING_SYNOYM = 1;
const int ApplicationProcessException::WRONG_APPLICATION_PROCES_NAME = 2;
const int ApplicationProcessException::NULL_OR_MALFORMED_SYNONYM = 3;
const int ApplicationProcessException::ALREADY_EXISTING_SYNOYM = 4;
const int ApplicationProcessException::NULL_OR_MALFORMED_WHATEVERCAST_NAME = 5;
const int ApplicationProcessException::ALREADY_EXISTING_WHATEVERCAST_NAME = 6;
const int ApplicationProcessException::UNEXISTING_WHATEVERCAST_NAME = 7;

ApplicationProcessException::ApplicationProcessException()
{
	errorCode = 0;
}

ApplicationProcessException::ApplicationProcessException(int arg0)
{
	errorCode = arg0;
}

ApplicationProcessException::ApplicationProcessException(int arg0, std::string message)
{
	errorCode = arg0;
	error_message = new char[message.size() + 1];
	std::copy(message.begin(), message.end(), error_message);
	this->error_message[message.size()] = '\0';
}

ApplicationProcessException::~ApplicationProcessException() throw()
{
	delete[] error_message;
}

int ApplicationProcessException::getErrorCode() const
{
	return errorCode;
}

void ApplicationProcessException::setErrorCode(int errorCode)
{
	this->errorCode = errorCode;
}

const char* ApplicationProcessException::what() const throw()
{
	return error_message;
}

/* CLASS CDAPException */
CDAPException::CDAPException() {
}

CDAPException::CDAPException(Exception &arg0) {
	result_reason = arg0.description_;
}

CDAPException::CDAPException(std::string arg0, int arg1, std::string arg2) {
	error_message = arg0;
	result = arg1;
	result_reason = arg2;
}

CDAPException::CDAPException(int arg0, std::string arg1){
	result = arg0;
	result_reason = arg1;
}

CDAPException::CDAPException(std::string arg0) {
	result_reason = arg0;
}

CDAPException::CDAPException(std::string arg0, const CDAPMessage &arg1) {
	result_reason = arg0;
	cdap_message = arg1;
}

std::string CDAPException::get_operation() const {
	return error_message;
}

void CDAPException::set_operation(std::string arg0) {
	error_message = arg0;
}

int CDAPException::get_result() const {
	return result;
}

void CDAPException::set_result(int arg0) {
	result = arg0;
}

std::string CDAPException::get_result_reason() const {
	return result_reason;
}

void CDAPException::set_result_reason(std::string arg0) {
	result_reason = arg0;
}

const CDAPMessage& CDAPException::get_cdap_message() const {
	return cdap_message;
}

void CDAPException::set_cdap_message(const CDAPMessage &arg0) {
	cdap_message = arg0;
}

virtual const char * CDAPException::what() const throw() {
	return error_message.c_str();
}

}

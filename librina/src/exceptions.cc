#include "exceptions.h"

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
	this->errorCode = 0;
}

ApplicationProcessException::ApplicationProcessException(int errorCode)
{
	this->errorCode = errorCode;
}

ApplicationProcessException::ApplicationProcessException(int errorCode, std::string message)
{
	this->errorCode = errorCode;
	this->message = new char[message.size() + 1];
	std::copy(message.begin(), message.end(), this->message);
	this->message[message.size()] = '\0';
}

ApplicationProcessException::~ApplicationProcessException() throw()
{
	delete[] message;
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
	return message;
}

#include "exceptions.h"

//namespace rina {
///*	CLASS ApplicationProcessException */
//const int ApplicationProcessException::UNEXISTING_SYNOYM = 1;
//const int ApplicationProcessException::WRONG_APPLICATION_PROCES_NAME = 2;
//const int ApplicationProcessException::NULL_OR_MALFORMED_SYNONYM = 3;
//const int ApplicationProcessException::ALREADY_EXISTING_SYNOYM = 4;
//const int ApplicationProcessException::NULL_OR_MALFORMED_WHATEVERCAST_NAME = 5;
//const int ApplicationProcessException::ALREADY_EXISTING_WHATEVERCAST_NAME = 6;
//const int ApplicationProcessException::UNEXISTING_WHATEVERCAST_NAME = 7;
//
//ApplicationProcessException::ApplicationProcessException()
//{
//	error_code = 0;
//}
//
//ApplicationProcessException::ApplicationProcessException(int arg0)
//{
//	error_code = arg0;
//}
//
//ApplicationProcessException::ApplicationProcessException(int arg0, const char* arg1)
//{
//	error_code = arg0;
//	error_message = arg1;
//}
//
//ApplicationProcessException::~ApplicationProcessException() throw()
//{
//	delete[] error_message;
//}
//
//int ApplicationProcessException::get_error_code() const
//{
//	return error_code;
//}
//
//void ApplicationProcessException::set_error_code(int arg0)
//{
//	error_code = arg0;
//}
//
//const char* ApplicationProcessException::what() const throw()
//{
//	return error_message;
//}
//}

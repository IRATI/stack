//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#ifndef LIBRINA_EXCEPTIONS_H
#define LIBRINA_EXCEPTIONS_H

#ifdef __cplusplus

#include <stdexcept>
#include <string>

namespace rina {
class Exception : public std::exception {
public:
        Exception() { }
        Exception(const std::string & s) : description_(s) { }

        virtual ~Exception() throw() { }

        virtual const char * what() const throw()
        { return description_.c_str(); }

protected:
        std::string description_;
};

/**
 * General exceptions thrown by Application Processes
 *
 */
class ApplicationProcessException: public std::exception{
	public:
	/*	Constants	*/
		static const int UNEXISTING_SYNOYM ;
		static const int WRONG_APPLICATION_PROCES_NAME;
		static const int NULL_OR_MALFORMED_SYNONYM;
		static const int ALREADY_EXISTING_SYNOYM;
		static const int NULL_OR_MALFORMED_WHATEVERCAST_NAME;
		static const int ALREADY_EXISTING_WHATEVERCAST_NAME;
		static const int UNEXISTING_WHATEVERCAST_NAME;
	/*	Constructors and Destructors	*/
		ApplicationProcessException();
		ApplicationProcessException(int errorCode);
		ApplicationProcessException(int errorCode, std::string message);
		~ApplicationProcessException() throw();
	/*	Accessors	*/
		int getErrorCode() const;
		void setErrorCode(int errorCode);
	/*	Methods	*/
		virtual const char* what() const throw();
	/*	Members	*/
	private:
		int errorCode;
		char* error_message;
};

class CDAPMessage;
class CDAPException: public Exception{
	public:
	/*	Constructors and Destructors	*/
		CDAPException();
		CDAPException(Exception &ex);
		CDAPException(std::string operation, int result, std::string result_reason);
		CDAPException(int result, std::string result_reason);
		CDAPException(std::string result_reason);
		CDAPException(std::string result_reason, const CDAPMessage &cdap_message);
        virtual ~CDAPException() throw() {};
    /*	Accessors	*/
		std::string get_operation() const;
		void set_operation(std::string operation);
		int get_result() const;
		void set_result(int result);
		std::string get_result_reason() const;
		void set_result_reason(std::string result_reason);
		const CDAPMessage& get_cdap_message() const;
		void set_cdap_message(const CDAPMessage &cdap_message);
	/*	Methods	*/
        virtual const char * what() const throw();
	private:
    /*	Members	*/
		/**
		 * Name of the operation that failed
		 */
		std::string error_message;
		/**
		 * Operation result code
		 */
		int result;
		/**
		 * Result reason
		 */
		std::string result_reason;
		/**
		 * The CDAPMessage that caused the exception
		 */
		CDAPMessage* cdap_message;
};

/**
 * Exceptions thrown by the RIB Daemon
 * TODO finish this class
 *
 */
class RIBDaemonException: public Exception{
	/*	Constants	*/
	public:
		/** Error codes **/
		static const int UNKNOWN_OBJECT_CLASS;
		static const int MALFORMED_MESSAGE_SUBSCRIPTION_REQUEST;
		static const int MALFORMED_MESSAGE_UNSUBSCRIPTION_REQUEST;
		static const int SUBSCRIBER_WAS_NOT_SUBSCRIBED;
		static const int OBJECTCLASS_AND_OBJECT_NAME_OR_OBJECT_INSTANCE_NOT_SPECIFIED;
		static const int OBJECTNAME_NOT_PRESENT_IN_THE_RIB;
		static const int RESPONSE_REQUIRED_BUT_MESSAGE_HANDLER_IS_NULL;
		static const int PROBLEMS_SENDING_CDAP_MESSAGE;
		static const int OPERATION_NOT_ALLOWED_AT_THIS_OBJECT;
		static const int UNRECOGNIZED_OBJECT_NAME;
		static const int OBJECTCLASS_DOES_NOT_MATCH_OBJECTNAME;
		static const int OBJECT_ALREADY_HAS_THIS_CHILD;
		static const int CHILD_NOT_FOUND;
		static const int OBJECT_ALREADY_EXISTS;
		static const int RIB_OBJECT_AND_OBJECT_NAME_NULL;
		static const int PROBLEMS_DECODING_OBJECT;
		static const int OBJECT_VALUE_IS_NULL;
	/*	Members	*/
	protected:
		int errorCode;
	/* Constructors and Destructors	 */
	public:
		RIBDaemonException(int errorCode);
		RIBDaemonException(int errorCode, std::string message);
		RIBDaemonException(int errorCode, Exception &ex);
        virtual ~RIBDaemonException() throw(){};
	/*	Accessors	*/
	public:
		int getErrorCode();
		void setErrorCode(int errorCode);
	/*	Methods	*/
	public:
        virtual const char * what() const throw();
};

}

#endif

#endif

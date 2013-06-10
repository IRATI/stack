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

%module rina

%include <stdint.i>
%include <stl.i>
%include <exception.i>

%include "stdlist.i"

#ifdef SWIGJAVA
#endif

/**
 * unsigned char * typemaps. 
 * These are input typemaps for mapping a Java byte[] array to a C char array.
 * Note that as a Java array is used and thus passeed by reference, the C
 * routine can return data to Java via the parameter.
 *
 * Example usage wrapping:
 *   void foo(unsigned char *array);
 *  
 * Java usage:
 *   byte b[] = new byte[20];
 *   modulename.foo(b);
 */
%typemap(jni)    unsigned char * "jbyteArray"
%typemap(jtype)  unsigned char * "byte[]"
%typemap(jstype) unsigned char * "byte[]"
%typemap(in)     unsigned char * {
        $1 = (unsigned char *) JCALL2(GetByteArrayElements, jenv, $input, 0); 
}

%typemap(argout) unsigned char * {
        JCALL3(ReleaseByteArrayElements, jenv, $input, (jbyte *) $1, 0); 
}

%typemap(javain) unsigned char * "$javainput"

%typemap(javaout) unsigned char* {
        return $jnicall;
 }

/* Macro to map C++ exceptions to Java exceptions*/
%define WRAP_THROW_EXCEPTION( MATCH, CPPTYPE, JTYPE, JNITYPE )
%javaexception(JTYPE) MATCH {
  try {
    $action
  } catch (CPPTYPE & e) {
    jclass eclass = jenv->FindClass(JNITYPE);
    if (eclass) {
      jenv->ThrowNew(eclass, e.what());
    }
  }
}
%enddef

/* Define the class IPC Exception */
%typemap(javabase) Exception "java.lang.Exception";
%typemap(javacode) Exception %{
  public String getMessage() {
    return what();
  }
%}

WRAP_THROW_EXCEPTION(rina::Flow::readSDU, rina::IPCException, 
		"eu.irati.librina.IPCException",
		"eu/irati/librina/IPCException" );
WRAP_THROW_EXCEPTION(rina::Flow::writeSDU, rina::IPCException, 
		"eu.irati.librina.IPCException",
		"eu/irati/librina/IPCException");
WRAP_THROW_EXCEPTION(rina::IPCManager::registerApplication, rina::IPCException, 
		"eu.irati.librina.IPCException",
		"eu/irati/librina/IPCException");
WRAP_THROW_EXCEPTION(rina::IPCManager::unregisterApplication, rina::IPCException, 
		"eu.irati.librina.IPCException",
		"eu/irati/librina/IPCException");
WRAP_THROW_EXCEPTION(rina::IPCManager::allocateFlowRequest, rina::IPCException, 
		"eu.irati.librina.IPCException",
		"eu/irati/librina/IPCException");
WRAP_THROW_EXCEPTION(rina::IPCManager::allocateFlowResponse, rina::IPCException, 
		"eu.irati.librina.IPCException",
		"eu/irati/librina/IPCException");
WRAP_THROW_EXCEPTION(rina::IPCManager::deallocateFlow, rina::IPCException, 
		"eu.irati.librina.IPCException",
		"eu/irati/librina/IPCException");
WRAP_THROW_EXCEPTION(rina::IPCProcessFactory::create, rina::IPCException, 
		"eu.irati.librina.IPCException",
		"eu/irati/librina/IPCException");
WRAP_THROW_EXCEPTION(rina::IPCProcessFactory::destroy, rina::IPCException, 
		"eu.irati.librina.IPCException",
		"eu/irati/librina/IPCException");
WRAP_THROW_EXCEPTION(rina::IPCProcess::assignToDIF, rina::IPCException, 
		"eu.irati.librina.IPCException",
		"eu/irati/librina/IPCException");
WRAP_THROW_EXCEPTION(rina::IPCProcess::notifyRegistrationToSupportingDIF, rina::IPCException, 
		"eu.irati.librina.IPCException",
		"eu/irati/librina/IPCException");
WRAP_THROW_EXCEPTION(rina::IPCProcess::notifyUnregistrationFromSupportingDIF, rina::IPCException, 
		"eu.irati.librina.IPCException",
		"eu/irati/librina/IPCException");
WRAP_THROW_EXCEPTION(rina::IPCProcess::enroll, rina::IPCException, 
		"eu.irati.librina.IPCException",
		"eu/irati/librina/IPCException");
WRAP_THROW_EXCEPTION(rina::IPCProcess::disconnectFromNeighbor, rina::IPCException, 
		"eu.irati.librina.IPCException",
		"eu/irati/librina/IPCException");
WRAP_THROW_EXCEPTION(rina::IPCProcess::registerApplication, rina::IPCException, 
		"eu.irati.librina.IPCException",
		"eu/irati/librina/IPCException");
WRAP_THROW_EXCEPTION(rina::IPCProcess::unregisterApplication, rina::IPCException, 
		"eu.irati.librina.IPCException",
		"eu/irati/librina/IPCException");
WRAP_THROW_EXCEPTION(rina::IPCProcess::allocateFlow, rina::IPCException, 
		"eu.irati.librina.IPCException",
		"eu/irati/librina/IPCException");
WRAP_THROW_EXCEPTION(rina::IPCProcess::queryRIB, rina::IPCException, 
		"eu.irati.librina.IPCException",
		"eu/irati/librina/IPCException");
WRAP_THROW_EXCEPTION(rina::ApplicationManager::applicationRegistered, rina::IPCException, 
		"eu.irati.librina.IPCException",
		"eu/irati/librina/IPCException");
WRAP_THROW_EXCEPTION(rina::ApplicationManager::applicationUnregistered, rina::IPCException, 
		"eu.irati.librina.IPCException",
		"eu/irati/librina/IPCException");
WRAP_THROW_EXCEPTION(rina::ApplicationManager::flowAllocated, rina::IPCException, 
		"eu.irati.librina.IPCException",
		"eu/irati/librina/IPCException");

%{
#include "exceptions.h"
#include "patterns.h"
#include "librina-common.h"
#include "librina-application.h"
#include "librina-ipc-manager.h"
%}

%rename(equals) rina::ApplicationProcessNamingInformation::operator==(const ApplicationProcessNamingInformation &other) const;
%rename(differs) rina::ApplicationProcessNamingInformation::operator!=(const ApplicationProcessNamingInformation &other) const;
%rename(isLessThanOrEquals) rina::ApplicationProcessNamingInformation::operator<=(const ApplicationProcessNamingInformation &other) const;   
%rename(isLessThan) rina::ApplicationProcessNamingInformation::operator<(const ApplicationProcessNamingInformation &other) const;
%rename(isMoreThanOrEquals) rina::ApplicationProcessNamingInformation::operator>=(const ApplicationProcessNamingInformation &other) const;   
%rename(isMoreThan) rina::ApplicationProcessNamingInformation::operator>(const ApplicationProcessNamingInformation &other) const;  
%rename(equals) rina::QoSCube::operator==(const QoSCube &other) const;  
%rename(differs) rina::QoSCube::operator!=(const QoSCube &other) const;  

%include "exceptions.h"
%include "patterns.h"
%include "librina-common.h"
%include "librina-application.h"
%include "librina-ipc-manager.h"

%template(DIFPropertiesVector) std::vector<rina::DIFProperties>;
%template(FlowVector) std::vector<rina::Flow>;
%template(QoSCubeList) std::list<rina::QoSCube>;
%template(ApplicationProcessNamingInformationList) std::list<rina::ApplicationProcessNamingInformation>;
%template(IPCManagerSingleton) Singleton<rina::IPCManager>;
%template(IPCEventProducerSingleton) Singleton<rina::IPCEventProducer>;
%template(IPCProcessFactorySingleton) Singleton<rina::IPCProcessFactory>;
%template(IPCProcessVector) std::vector<rina::IPCProcess>;
%template(ApplicationManagerSingleton) Singleton<rina::ApplicationManager>;

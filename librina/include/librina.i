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
%typemap(javabase) IPCException "java.lang.Exception";
%typemap(javacode) IPCException %{
  public String getMessage() {
    return what();
  }
%}

WRAP_THROW_EXCEPTION(Flow::readSDU, IPCException, 
		"eu.irati.librina.IPCException",
		"eu/irati/librina/IPCException" );
WRAP_THROW_EXCEPTION(Flow::writeSDU, IPCException, 
		"eu.irati.librina.IPCException",
		"eu/irati/librina/IPCException");
WRAP_THROW_EXCEPTION(IPCManager::registerApplication, IPCException, 
		"eu.irati.librina.IPCException",
		"eu/irati/librina/IPCException");
WRAP_THROW_EXCEPTION(IPCManager::unregisterApplication, IPCException, 
		"eu.irati.librina.IPCException",
		"eu/irati/librina/IPCException");
WRAP_THROW_EXCEPTION(IPCManager::allocateFlowRequest, IPCException, 
		"eu.irati.librina.IPCException",
		"eu/irati/librina/IPCException");
WRAP_THROW_EXCEPTION(IPCManager::allocateFlowResponse, IPCException, 
		"eu.irati.librina.IPCException",
		"eu/irati/librina/IPCException");
WRAP_THROW_EXCEPTION(IPCManager::deallocate, IPCException, 
		"eu.irati.librina.IPCException",
		"eu/irati/librina/IPCException");

%{
#include "librina-common.h"
#include "librina-application.h"
%}

%rename(equals) ApplicationProcessNamingInformation::operator==(const ApplicationProcessNamingInformation &other) const;
%rename(differs) ApplicationProcessNamingInformation::operator!=(const ApplicationProcessNamingInformation &other) const;
%rename(isLessThanOrEquals) ApplicationProcessNamingInformation::operator<=(const ApplicationProcessNamingInformation &other) const;   
%rename(isLessThan) ApplicationProcessNamingInformation::operator<(const ApplicationProcessNamingInformation &other) const;
%rename(isMoreThanOrEquals) ApplicationProcessNamingInformation::operator>=(const ApplicationProcessNamingInformation &other) const;   
%rename(isMoreThan) ApplicationProcessNamingInformation::operator>(const ApplicationProcessNamingInformation &other) const;  
%rename(equals) QoSCube::operator==(const QoSCube &other) const;  
%rename(differs) QoSCube::operator!=(const QoSCube &other) const;  

%include "librina-common.h"
%include "librina-application.h"

%template(DIFPropertiesVector) std::vector<DIFProperties>;
%template(FlowVector) std::vector<Flow>;
%template(QoSCubeList) std::list<QoSCube>;
%template(ApplicationProcessNamingInformationList) std::list<ApplicationProcessNamingInformation>;

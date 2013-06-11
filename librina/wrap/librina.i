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

/* Define the class Exception */
%typemap(javabase) Exception "java.lang.Exception";
%typemap(javacode) Exception %{
  public String getMessage() {
    return what();
  }
%}

%typemap(throws, throws="eu.irati.librina.IPCException") rina::IPCException {
  jclass excep = jenv->FindClass("eu/irati/librina/IPCException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}

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

/* Macro for defining collection iterators */
%define MAKE_COLLECTION_ITERABLE( ITERATORNAME, JTYPE, CPPCOLLECTION, CPPTYPE )
%typemap(javainterfaces) ITERATORNAME "java.util.Iterator<JTYPE>"
%typemap(javacode) ITERATORNAME %{
  public void remove() throws UnsupportedOperationException {
    throw new UnsupportedOperationException();
  }

  public JTYPE next() throws java.util.NoSuchElementException {
    if (!hasNext()) {
      throw new java.util.NoSuchElementException();
    }

    return nextImpl();
  }
%}
%javamethodmodifiers ITERATORNAME::nextImpl "private";
%inline %{
  struct ITERATORNAME {
    typedef CPPCOLLECTION<CPPTYPE> collection_t;
    ITERATORNAME(const collection_t& t) : it(t.begin()), collection(t) {}
    bool hasNext() const {
      return it != collection.end();
    }

    const CPPTYPE& nextImpl() {
      const CPPTYPE& type = *it++;
      return type;
    }
  private:
    collection_t::const_iterator it;
    const collection_t& collection;    
  };
%}
%typemap(javainterfaces) CPPCOLLECTION<CPPTYPE> "Iterable<JTYPE>"
%newobject CPPCOLLECTION<CPPTYPE>::iterator() const;
%extend CPPCOLLECTION<CPPTYPE> {
  ITERATORNAME *iterator() const {
    return new ITERATORNAME(*$self);
  }
}
%enddef

/* Define iterator for QoSCube list */
MAKE_COLLECTION_ITERABLE(QoSCubeListIterator, QoSCube, std::list, rina::QoSCube);
/* Define iterator for ApplicationProcessNamingInformation list */
MAKE_COLLECTION_ITERABLE(ApplicationProcessNamingInformationListIterator, ApplicationProcessNamingInformation, std::list, rina::ApplicationProcessNamingInformation);

%template(DIFPropertiesVector) std::vector<rina::DIFProperties>;
%template(FlowVector) std::vector<rina::Flow>;
%template(FlowPointerVector) std::vector<rina::Flow *>;
%template(ApplicationRegistrationVector) std::vector<rina::ApplicationRegistration *>;
%template(QoSCubeList) std::list<rina::QoSCube>;
%template(QoSCubeVector) std::vector<rina::QoSCube>;
%template(PolicyVector) std::vector<rina::Policy>;
%template(ApplicationProcessNamingInformationList) std::list<rina::ApplicationProcessNamingInformation>;
%template(IPCManagerSingleton) Singleton<rina::IPCManager>;
%template(IPCEventProducerSingleton) Singleton<rina::IPCEventProducer>;
%template(IPCProcessFactorySingleton) Singleton<rina::IPCProcessFactory>;
%template(IPCProcessVector) std::vector<rina::IPCProcess>;
%template(IPCProcessPointerVector) std::vector<rina::IPCProcess *>;
%template(ApplicationManagerSingleton) Singleton<rina::ApplicationManager>;

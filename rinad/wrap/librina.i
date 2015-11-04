/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

%module rina

%include "enums.swg"
%include <stdint.i>
%include <stl.i>

%include "stdlist.i"

#ifdef SWIGJAVA
#endif

SWIG_JAVABODY_PROXY(public, public, SWIGTYPE)
SWIG_JAVABODY_TYPEWRAPPER(public, public, public, SWIGTYPE)

/**
 * void * typemaps. 
 * These are input typemaps for mapping a Java byte[] array to a C void array.
 * Note that as a Java array is used and thus passeed by reference, the C
 * routine can return data to Java via the parameter.
 *
 * Example usage wrapping:
 *   void foo(void *array);
 *  
 * Java usage:
 *   byte b[] = new byte[20];
 *   modulename.foo(b);
 */
%typemap(jni)    void * "jbyteArray"
%typemap(jtype)  void * "byte[]"
%typemap(jstype) void * "byte[]"
%typemap(in)     void * {
        $1 = (void *) JCALL2(GetByteArrayElements, jenv, $input, 0); 
}

%typemap(argout) void * {
        JCALL3(ReleaseByteArrayElements, jenv, $input, (jbyte *) $1, 0); 
}

%typemap(javain) void * "$javainput"

%typemap(javaout) void * {
        return $jnicall;
 }

/**
 * char * typemaps. 
 * These are input typemaps for mapping a Java byte[] array to a C char array.
 * Note that as a Java array is used and thus passeed by reference, the C
 * routine can return data to Java via the parameter.
 *
 * Example usage wrapping:
 *   void foo(char *array);
 *  
 * Java usage:
 *   byte b[] = new byte[20];
 *   modulename.foo(b);
 */
/*
%typemap(jni)    char * "jbyteArray"
%typemap(jtype)  char * "byte[]"
%typemap(jstype) char * "byte[]"
%typemap(in)     char * {
        $1 = (char *) JCALL2(GetByteArrayElements, jenv, $input, 0); 
}

%typemap(argout) char * {
        JCALL3(ReleaseByteArrayElements, jenv, $input, (jbyte *) $1, 0); 
}

%typemap(javain) char * "$javainput"

%typemap(javaout) char * {
        return $jnicall;
 }
*/
/* Define the class Exception */
%typemap(javabase) Exception "java.lang.Exception";
%typemap(javacode) Exception %{
  public String getMessage() {
    return what();
  }
%}

%typemap(throws, throws="eu.irati.librina.Exception") Exception {
  jclass excep = jenv->FindClass("eu/irati/librina/Exception");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.IPCException") rina::IPCException {
  jclass excep = jenv->FindClass("eu/irati/librina/IPCException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.FlowNotAllocatedException") rina::FlowNotAllocatedException {
  jclass excep = jenv->FindClass("eu/irati/librina/FlowNotAllocatedException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.UnknownFlowException") rina::UnknownFlowException {
  jclass excep = jenv->FindClass("eu/irati/librina/UnknownFlowException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.InvalidArgumentsException") rina::InvalidArgumentsException {
  jclass excep = jenv->FindClass("eu/irati/librina/InvalidArgumentsException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.ReadSDUException") rina::ReadSDUException {
  jclass excep = jenv->FindClass("eu/irati/librina/ReadSDUException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.WriteSDUException") rina::WriteSDUException {
  jclass excep = jenv->FindClass("eu/irati/librina/WriteSDUException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.ApplicationRegistrationException") rina::ApplicationRegistrationException {
  jclass excep = jenv->FindClass("eu/irati/librina/ApplicationRegistrationException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.ApplicationUnregistrationException") rina::ApplicationUnregistrationException {
  jclass excep = jenv->FindClass("eu/irati/librina/ApplicationUnregistrationException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.FlowAllocationException") rina::FlowAllocationException {
  jclass excep = jenv->FindClass("eu/irati/librina/FlowAllocationException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.FlowDeallocationException") rina::FlowDeallocationException {
  jclass excep = jenv->FindClass("eu/irati/librina/FlowDeallocationException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.AllocateFlowException") rina::AllocateFlowException {
  jclass excep = jenv->FindClass("eu/irati/librina/AllocateFlowException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.NotifyApplicationRegisteredException") rina::NotifyApplicationRegisteredException {
  jclass excep = jenv->FindClass("eu/irati/librina/NotifyApplicationRegisteredException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.NotifyApplicationUnregisteredException") rina::NotifyApplicationUnregisteredException {
  jclass excep = jenv->FindClass("eu/irati/librina/NotifyApplicationUnregisteredException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.NotifyFlowAllocatedException") rina::NotifyFlowAllocatedException {
  jclass excep = jenv->FindClass("eu/irati/librina/NotifyFlowAllocatedException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.RegisterApplicationResponseException") rina::RegisterApplicationResponseException {
  jclass excep = jenv->FindClass("eu/irati/librina/RegisterApplicationResponseException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.UnregisterApplicationResponseException") rina::UnregisterApplicationResponseException {
  jclass excep = jenv->FindClass("eu/irati/librina/UnregisterApplicationResponseException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.AllocateFlowResponseException") rina::AllocateFlowResponseException {
  jclass excep = jenv->FindClass("eu/irati/librina/AllocateFlowResponseException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.DeallocateFlowResponseException") rina::DeallocateFlowResponseException {
  jclass excep = jenv->FindClass("eu/irati/librina/DeallocateFlowResponseException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.GetDIFPropertiesException") rina::GetDIFPropertiesException {
  jclass excep = jenv->FindClass("eu/irati/librina/GetDIFPropertiesException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.GetDIFPropertiesResponseException") rina::GetDIFPropertiesResponseException {
  jclass excep = jenv->FindClass("eu/irati/librina/GetDIFPropertiesResponseException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.AllocateFlowRequestArrivedException") rina::AllocateFlowRequestArrivedException {
  jclass excep = jenv->FindClass("eu/irati/librina/AllocateFlowRequestArrivedException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.AppFlowArrivedException") rina::AppFlowArrivedException {
  jclass excep = jenv->FindClass("eu/irati/librina/AppFlowArrivedException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.IpcmDeallocateFlowException") rina::IpcmDeallocateFlowException {
  jclass excep = jenv->FindClass("eu/irati/librina/IpcmDeallocateFlowException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.NotifyFlowDeallocatedException") rina::NotifyFlowDeallocatedException {
  jclass excep = jenv->FindClass("eu/irati/librina/NotifyFlowDeallocatedException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.InitializationException") rina::InitializationException {
  jclass excep = jenv->FindClass("eu/irati/librina/InitializationException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}


/* Typemaps to allow eventWait, eventPoll and eventTimedWait to downcast IPCEvent to the correct class */
%define DOWNCAST_IPC_EVENT_CONSUMER( OPERATION )
%typemap(jni) rina::IPCEvent *rina::IPCEventProducer::OPERATION "jobject"
%typemap(jtype) rina::IPCEvent *rina::IPCEventProducer::OPERATION "eu.irati.librina.IPCEvent"
%typemap(jstype) rina::IPCEvent *rina::IPCEventProducer::OPERATION "eu.irati.librina.IPCEvent"
%typemap(javaout) rina::IPCEvent *rina::IPCEventProducer::OPERATION {
    return $jnicall;
  }

%typemap(out) rina::IPCEvent *rina::IPCEventProducer::OPERATION {
    if ($1->eventType == rina::APPLICATION_REGISTRATION_REQUEST_EVENT) {
    	rina::ApplicationRegistrationRequestEvent *appRegReqEvent = dynamic_cast<rina::ApplicationRegistrationRequestEvent *>($1);
        jclass clazz = jenv->FindClass("eu/irati/librina/ApplicationRegistrationRequestEvent");
        if (clazz) {
            jmethodID mid = jenv->GetMethodID(clazz, "<init>", "(JZ)V");
            if (mid) {
                jlong cptr = 0;
                *(rina::ApplicationRegistrationRequestEvent **)&cptr = appRegReqEvent; 
                $result = jenv->NewObject(clazz, mid, cptr, false);
            }
        }
    } else if ($1->eventType == rina::APPLICATION_UNREGISTRATION_REQUEST_EVENT) {
    	rina::ApplicationUnregistrationRequestEvent *appUnregReqEvent = dynamic_cast<rina::ApplicationUnregistrationRequestEvent *>($1);
        jclass clazz = jenv->FindClass("eu/irati/librina/ApplicationUnregistrationRequestEvent");
        if (clazz) {
            jmethodID mid = jenv->GetMethodID(clazz, "<init>", "(JZ)V");
            if (mid) {
                jlong cptr = 0;
                *(rina::ApplicationUnregistrationRequestEvent **)&cptr = appUnregReqEvent; 
                $result = jenv->NewObject(clazz, mid, cptr, false);
            }
        }
    } else if ($1->eventType == rina::FLOW_ALLOCATION_REQUESTED_EVENT) {
    	rina::FlowRequestEvent *flowReqEvent = dynamic_cast<rina::FlowRequestEvent *>($1);
        jclass clazz = jenv->FindClass("eu/irati/librina/FlowRequestEvent");
        if (clazz) {
            jmethodID mid = jenv->GetMethodID(clazz, "<init>", "(JZ)V");
            if (mid) {
                jlong cptr = 0;
                *(rina::FlowRequestEvent **)&cptr = flowReqEvent; 
                $result = jenv->NewObject(clazz, mid, cptr, false);
            }
        }
    } else if ($1->eventType == rina::FLOW_DEALLOCATION_REQUESTED_EVENT) {
    	rina::FlowDeallocateRequestEvent *flowReqEvent = dynamic_cast<rina::FlowDeallocateRequestEvent *>($1);
        jclass clazz = jenv->FindClass("eu/irati/librina/FlowDeallocateRequestEvent");
        if (clazz) {
            jmethodID mid = jenv->GetMethodID(clazz, "<init>", "(JZ)V");
            if (mid) {
                jlong cptr = 0;
                *(rina::FlowDeallocateRequestEvent **)&cptr = flowReqEvent; 
                $result = jenv->NewObject(clazz, mid, cptr, false);
            }
        }
    } else if ($1->eventType == rina::FLOW_DEALLOCATED_EVENT) {
    	rina::FlowDeallocatedEvent *flowReqEvent = dynamic_cast<rina::FlowDeallocatedEvent *>($1);
        jclass clazz = jenv->FindClass("eu/irati/librina/FlowDeallocatedEvent");
        if (clazz) {
            jmethodID mid = jenv->GetMethodID(clazz, "<init>", "(JZ)V");
            if (mid) {
                jlong cptr = 0;
                *(rina::FlowDeallocatedEvent **)&cptr = flowReqEvent; 
                $result = jenv->NewObject(clazz, mid, cptr, false);
            }
        }
    } else if ($1->eventType == rina::REGISTER_APPLICATION_RESPONSE_EVENT) {
    	rina::RegisterApplicationResponseEvent *flowReqEvent = dynamic_cast<rina::RegisterApplicationResponseEvent *>($1);
        jclass clazz = jenv->FindClass("eu/irati/librina/RegisterApplicationResponseEvent");
        if (clazz) {
            jmethodID mid = jenv->GetMethodID(clazz, "<init>", "(JZ)V");
            if (mid) {
                jlong cptr = 0;
                *(rina::RegisterApplicationResponseEvent **)&cptr = flowReqEvent; 
                $result = jenv->NewObject(clazz, mid, cptr, false);
            }
        }
    } else if ($1->eventType == rina::UNREGISTER_APPLICATION_RESPONSE_EVENT) {
    	rina::UnregisterApplicationResponseEvent *flowReqEvent = dynamic_cast<rina::UnregisterApplicationResponseEvent *>($1);
        jclass clazz = jenv->FindClass("eu/irati/librina/UnregisterApplicationResponseEvent");
        if (clazz) {
            jmethodID mid = jenv->GetMethodID(clazz, "<init>", "(JZ)V");
            if (mid) {
                jlong cptr = 0;
                *(rina::UnregisterApplicationResponseEvent **)&cptr = flowReqEvent; 
                $result = jenv->NewObject(clazz, mid, cptr, false);
            }
        }
    } else if ($1->eventType == rina::ALLOCATE_FLOW_RESPONSE_EVENT) {
    	rina::AllocateFlowResponseEvent *flowReqEvent = dynamic_cast<rina::AllocateFlowResponseEvent *>($1);
        jclass clazz = jenv->FindClass("eu/irati/librina/AllocateFlowResponseEvent");
        if (clazz) {
            jmethodID mid = jenv->GetMethodID(clazz, "<init>", "(JZ)V");
            if (mid) {
                jlong cptr = 0;
                *(rina::AllocateFlowResponseEvent **)&cptr = flowReqEvent; 
                $result = jenv->NewObject(clazz, mid, cptr, false);
            }
        }
    } else if ($1->eventType == rina::ALLOCATE_FLOW_REQUEST_RESULT_EVENT) {
    	rina::AllocateFlowRequestResultEvent *flowReqEvent = dynamic_cast<rina::AllocateFlowRequestResultEvent *>($1);
        jclass clazz = jenv->FindClass("eu/irati/librina/AllocateFlowRequestResultEvent");
        if (clazz) {
            jmethodID mid = jenv->GetMethodID(clazz, "<init>", "(JZ)V");
            if (mid) {
                jlong cptr = 0;
                *(rina::AllocateFlowRequestResultEvent **)&cptr = flowReqEvent; 
                $result = jenv->NewObject(clazz, mid, cptr, false);
            }
        }
    } else if ($1->eventType == rina::DEALLOCATE_FLOW_RESPONSE_EVENT) {
    	rina::DeallocateFlowResponseEvent *flowReqEvent = dynamic_cast<rina::DeallocateFlowResponseEvent *>($1);
        jclass clazz = jenv->FindClass("eu/irati/librina/DeallocateFlowResponseEvent");
        if (clazz) {
            jmethodID mid = jenv->GetMethodID(clazz, "<init>", "(JZ)V");
            if (mid) {
                jlong cptr = 0;
                *(rina::DeallocateFlowResponseEvent **)&cptr = flowReqEvent; 
                $result = jenv->NewObject(clazz, mid, cptr, false);
            }
        }
    } else if ($1->eventType == rina::GET_DIF_PROPERTIES_RESPONSE_EVENT) {
    	rina::GetDIFPropertiesResponseEvent *flowReqEvent = dynamic_cast<rina::GetDIFPropertiesResponseEvent *>($1);
        jclass clazz = jenv->FindClass("eu/irati/librina/GetDIFPropertiesResponseEvent");
        if (clazz) {
            jmethodID mid = jenv->GetMethodID(clazz, "<init>", "(JZ)V");
            if (mid) {
                jlong cptr = 0;
                *(rina::GetDIFPropertiesResponseEvent **)&cptr = flowReqEvent; 
                $result = jenv->NewObject(clazz, mid, cptr, false);
            }
        }
    }
} 
%enddef

DOWNCAST_IPC_EVENT_CONSUMER(eventWait);
DOWNCAST_IPC_EVENT_CONSUMER(eventPoll);
DOWNCAST_IPC_EVENT_CONSUMER(eventTimedWait);

%module(directors="1") cdapcallbackjava

%{
#include "librina/exceptions.h"
#include "librina/patterns.h"
#include "librina/concurrency.h"
#include "librina/common.h"
#include "librina/application.h"
#include "librina/cdap_rib_structures.h"
#include "librina/cdap_v2.h"
#include "librina/ipc-api.h"
%}

%feature("director") rina::cdap::CDAPCallbackInterface;

%rename(differs) rina::ApplicationProcessNamingInformation::operator!=(const ApplicationProcessNamingInformation &other) const;
%rename(equals) rina::ApplicationProcessNamingInformation::operator==(const ApplicationProcessNamingInformation &other) const;
%rename(assign) rina::ApplicationProcessNamingInformation::operator=(const ApplicationProcessNamingInformation &other);
%rename(assign) rina::SerializedObject::operator=(const SerializedObject &other);
%rename(assign) rina::UcharArray::operator=(const UcharArray &other);
%rename(differs) rina::UcharArray::operator!=(const UcharArray &other) const;
%rename(equals) rina::UcharArray::operator==(const UcharArray &other) const;
%rename(isLessThanOrEquals) rina::ApplicationProcessNamingInformation::operator<=(const ApplicationProcessNamingInformation &other) const;   
%rename(isLessThan) rina::ApplicationProcessNamingInformation::operator<(const ApplicationProcessNamingInformation &other) const;
%rename(isMoreThanOrEquals) rina::ApplicationProcessNamingInformation::operator>=(const ApplicationProcessNamingInformation &other) const;   
%rename(isMoreThan) rina::ApplicationProcessNamingInformation::operator>(const ApplicationProcessNamingInformation &other) const;  
%rename(equals) rina::FlowSpecification::operator==(const FlowSpecification &other) const;
%rename(differs) rina::FlowSpecification::operator!=(const FlowSpecification &other) const;
%rename(equals) rina::Thread::operator==(const Thread &other) const;
%rename(differs) rina::Thread::operator!=(const Thread &other) const;
%rename(equals) rina::Parameter::operator==(const Parameter &other) const;
%rename(differs) rina::Parameter::operator!=(const Parameter &other) const;
%rename(equals) rina::Policy::operator==(const Policy &other) const;
%rename(differs) rina::Policy::operator!=(const Policy &other) const;
%rename(equals) rina::FlowInformation::operator==(const FlowInformation &other) const;
%rename(differs) rina::FlowInformation::operator!=(const FlowInformation &other) const;
%rename(equals) rina::rib::RIBObjectData::operator==(const RIBObjectData &other) const;
%rename(differs) rina::rib::RIBObjectData::operator!=(const RIBObjectData &other) const;

%rename(equals) rina::Neighbor::operator==(const Neighbor &other) const;
%rename(differs) rina::Neighbor::operator!=(const Neighbor &other) const;

%include "../include/librina/exceptions.h"
%include "librina/patterns.h"
%include "librina/concurrency.h"
%include "librina/common.h"

%template(TempStringEncoder) rina::Encoder<std::string>;
%template(TempIntEncoder) rina::Encoder<int>;

%include "librina/application.h"
%include "librina/cdap_rib_structures.h"
%include "librina/cdap_v2.h"
%include "librina/ipc-api.h"

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
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
* unsigned char* typemaps. 
* These are input typemaps for mapping a c++ unsigned char * to a Java byte[].
*/
%typemap(jni) unsigned char * message_ "jbyteArray"
%typemap(jtype) unsigned char * message_ "byte[]"
%typemap(jstype) unsigned char * message_ "byte[]"
%typemap(javaout) unsigned char * message_ {
    return $jnicall;
}
%typemap(out) signed char * message_ {
    $result = JCALL1(NewByteArray, jenv, arg1->contentLength);
    JCALL4(SetByteArrayRegion, jenv, $result, 0, arg1->contentLength, $1);
}
%typemap(in) unsigned char *message_ {
    $1 = (unsigned char *)JCALL2(GetByteArrayElements, jenv, $input, 0);
}
%typemap(javain) unsigned char *message_ "$javainput"

/**
* std::string & typemaps. 
* These are input typemaps for mapping a c++ std::string& to a Java String[].
*/

%typemap(jstype) std::string& INPUT "String[]"
%typemap(jtype) std::string& INPUT "String[]"
%typemap(jni) std::string& INPUT "jobjectArray"
%typemap(javain)  std::string& INPUT "$javainput"
%typemap(in) std::string& INPUT (std::string temp) {
  if (!$input) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "array null");
    return $null;
  }
  if (JCALL1(GetArrayLength, jenv, $input) == 0) {
    SWIG_JavaThrowException(jenv, SWIG_JavaIndexOutOfBoundsException, "Array must contain at least 1 element");
  }
  $1 = &temp;
}
%typemap(argout) std::string& INPUT {
  jstring jvalue = JCALL1(NewStringUTF, jenv, temp$argnum.c_str()); 
  JCALL3(SetObjectArrayElement, jenv, $input, 0, jvalue);
}
%apply  std::string& INPUT { std::string & des_obj }

/* Define the class eu.irati.librina.Exception */
%typemap(javabase) rina::Exception "java.lang.RuntimeException";
%typemap(javacode) rina::Exception %{
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
/*
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
*/
%typemap(throws, throws="eu.irati.librina.GetDIFPropertiesException") rina::GetDIFPropertiesException {
  jclass excep = jenv->FindClass("eu/irati/librina/GetDIFPropertiesException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
/*
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
*/
%typemap(throws, throws="eu.irati.librina.InitializationException") rina::InitializationException {
  jclass excep = jenv->FindClass("eu/irati/librina/InitializationException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.CDAPException") rina::cdap::CDAPException {
  jclass excep = jenv->FindClass("eu/irati/librina/CDAPException");
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
    if ($1 == NULL) {
      return NULL;
    }
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
    } else if ($1->eventType == rina::APPLICATION_UNREGISTERED_EVENT) { /* mcr */
  	rina::ApplicationUnregisteredEvent *appUnregisteredEvent = dynamic_cast<rina::ApplicationUnregisteredEvent *>($1);
        jclass clazz = jenv->FindClass("eu/irati/librina/ApplicationUnregisteredEvent");
        if (clazz) {
            jmethodID mid = jenv->GetMethodID(clazz, "<init>", "(JZ)V");
            if (mid) {
                jlong cptr = 0;
                *(rina::ApplicationUnregisteredEvent **)&cptr = appUnregisteredEvent; 
                $result = jenv->NewObject(clazz, mid, cptr, false);
            }
        }
    } else if ($1->eventType == rina::APPLICATION_REGISTRATION_CANCELED_EVENT) { /* mcr */
  	rina::AppRegistrationCanceledEvent *canceledEvent = dynamic_cast<rina::AppRegistrationCanceledEvent *>($1);
        jclass clazz = jenv->FindClass("eu/irati/librina/AppRegistrationCanceledEvent");
        if (clazz) {
            jmethodID mid = jenv->GetMethodID(clazz, "<init>", "(JZ)V");
            if (mid) {
                jlong cptr = 0;
                *(rina::AppRegistrationCanceledEvent **)&cptr = canceledEvent; 
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
    } else {
      // Generate a warning message, rather than silently fail
      std::cerr << "Warning: Java bindings - unmapped IPCEvent of type=" << $1->eventType << std::endl;
    }
} 
%enddef

DOWNCAST_IPC_EVENT_CONSUMER(eventWait);
DOWNCAST_IPC_EVENT_CONSUMER(eventPoll);
DOWNCAST_IPC_EVENT_CONSUMER(eventTimedWait);

// Allow the callbacks between C++ and java
%module(directors="1") cdapcallbackjava

%{
#include <iostream>
#include "librina/exceptions.h"
#include "librina/patterns.h"
#include "librina/concurrency.h"
#include "librina/common.h"
#include "librina/application.h"
#include "librina/cdap_rib_structures.h"
#include "librina/cdap_v2.h"
#include "librina/rib_v2.h"
#include "librina/ipc-api.h"
#include "librina/configuration.h"
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
%rename(equals) rina::PsInfo::operator==(const PsInfo &other) const;
%rename(differs) rina::PsInfo::operator!=(const PsInfo &other) const;
%rename(assign) rina::ser_obj::operator=(const ser_obj &other);
%rename(assign) rina::cdap_rib::auth_policy::operator=(const auth_policy &other);
%rename(assign) rina::Neighbor::operator=(const Neighbor &other);
%rename(equals) rina::PolicyParameter::operator==(const PolicyParameter &other) const;
%rename(differs) rina::PolicyParameter::operator!=(const PolicyParameter &other) const;
%rename(equals) rina::PolicyConfig::operator==(const PolicyConfig &other) const;
%rename(differs) rina::PolicyConfig::operator!=(const PolicyConfig &other) const;
%rename(equals) rina::QoSCube::operator==(const QoSCube &other) const;
%rename(differs) rina::QoSCube::operator!=(const QoSCube &other) const;
%rename(assign) rina::EFCPConfiguration::operator=(const EFCPConfiguration &other);

/* This is for separating rib::init and rib::fini from cdap::init and cdap::fini */
%rename(cdap_init) rina::cdap::init(cdap::CDAPCallbackInterface *callback, cdap_rib::concrete_syntax_t& syntax, int fd);
%rename(cdap_fini) rina::cdap::fini(void);
%rename(rib_init) rina::rib::init(cacep::AppConHandlerInterface *app_con_callback, cdap_rib::cdap_params params);
%rename(rib_fini) rina::rib::fini(void);

/* This is for avoiding a file name too long to be managed. */

namespace rina {
namespace rib{
typedef void (*create_cb_t)(const rib_handle_t rib,
const cdap_rib::con_handle_t &con,
const std::string& fqn,
const std::string& class_,
const cdap_rib::filt_info_t &filt,
const int invoke_id,
const ser_obj_t &obj_req,
ser_obj_t &obj_reply,
cdap_rib::res_info_t& res);
}}

// Define Exceptions being thrown
%catches(rina::UnknownFlowException) rina::IPCManager::getFlowInformation(int portId);
%catches(rina::UnknownFlowException) rina::IPCManager::getPortIdToRemoteApp(const ApplicationProcessNamingInformation& remoteAppName);
%catches(rina::IPCException) rina::IPCManager::getRegistrationInfo(unsigned int seqNumber);
%catches(rina::FlowAllocationException) rina::IPCManager::internalRequestFlowAllocation(const ApplicationProcessNamingInformation& localAppName, const ApplicationProcessNamingInformation& remoteAppName,       const FlowSpecification& flowSpec, unsigned short sourceIPCProcessId);
%catches(rina::FlowAllocationException) rina::IPCManager::IPCManager::internalAllocateFlowResponse(const FlowRequestEvent& flowRequestEvent,int result,bool notifySource,unsigned short ipcProcessId,bool blocking);
%catches(rina::GetDIFPropertiesException) rina::IPCManager::getDIFProperties(const ApplicationProcessNamingInformation& applicationName, const ApplicationProcessNamingInformation& DIFName);
%catches(rina::ApplicationRegistrationException) rina::IPCManager::commitPendingRegistration(unsigned int seqNumber, const ApplicationProcessNamingInformation& DIFName);
%catches(rina::ApplicationRegistrationException) rina::IPCManager::withdrawPendingRegistration(unsigned int seqNumber);
%catches(rina::ApplicationUnregistrationException) rina::IPCManager::requestApplicationUnregistration(const ApplicationProcessNamingInformation& applicationName, const ApplicationProcessNamingInformation& DIFName);
%catches(rina::ApplicationUnregistrationException) rina::IPCManager::appUnregistrationResult(unsigned int seqNumber, bool success);
%catches(rina::FlowDeallocationException) rina::IPCManager::commitPendingFlow(unsigned int sequenceNumber, int portId, const ApplicationProcessNamingInformation& DIFName);
%catches(rina::FlowDeallocationException) rina::IPCManager::withdrawPendingFlow(unsigned int sequenceNumber);
%catches(rina::FlowDeallocationException) rina::IPCManager::requestFlowDeallocation(int portId);
%catches(rina::FlowDeallocationException) rina::IPCManager::flowDeallocationResult(int portId, bool success);
%catches(rina::FlowDeallocationException) rina::IPCManager::flowDeallocated(int portId);


%include "librina/exceptions.h"
%include "librina/patterns.h"
%include "librina/concurrency.h"
%include "librina/common.h"
%include "librina/configuration.h"

namespace rina {
namespace cdap {
class cdap_m_t;
}
}

%template(TempCDAPMessageEncoder) rina::Encoder<rina::cdap::cdap_m_t>;
%template(TempStringEncoder) rina::Encoder<std::string>;
%template(TempIntEncoder) rina::Encoder<int>;


%include "librina/application.h"
%include "librina/cdap_rib_structures.h"
%include "librina/cdap_v2.h"
%include "librina/ipc-api.h"
%include "librina/rib_v2.h"


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

/* Define iterator for ApplicationProcessNamingInformation list */
MAKE_COLLECTION_ITERABLE(ApplicationProcessNamingInformationListIterator, ApplicationProcessNamingInformation, std::list, rina::ApplicationProcessNamingInformation);
/* Define iterator for String list */
MAKE_COLLECTION_ITERABLE(StringListIterator, String, std::list, std::string);
/* Define iterator for Flow Information list */
MAKE_COLLECTION_ITERABLE(FlowInformationListIterator, FlowInformation, std::list, rina::FlowInformation);
/* Define iterator for Unsigned int list */
MAKE_COLLECTION_ITERABLE(UnsignedIntListIterator, Long, std::list, unsigned int);

%template(DIFPropertiesVector) std::vector<rina::DIFProperties>;
%template(FlowInformationVector) std::vector<rina::FlowInformation>;
%template(ApplicationRegistrationVector) std::vector<rina::ApplicationRegistration *>;
%template(ParameterList) std::list<rina::Parameter>;
%template(ApplicationProcessNamingInformationList) std::list<rina::ApplicationProcessNamingInformation>;
%template(IPCManagerSingleton) Singleton<rina::IPCManager>;
%template(IPCEventProducerSingleton) Singleton<rina::IPCEventProducer>;
%template(StringList) std::list<std::string>;
%template(FlowInformationList) std::list<rina::FlowInformation>;
%template(UnsignedIntList) std::list<unsigned int>;

/* end */

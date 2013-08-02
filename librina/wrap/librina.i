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
%typemap(throws, throws="eu.irati.librina.FlowNotAllocatedException") rina::FlowNotAllocatedException {
  jclass excep = jenv->FindClass("eu/irati/librina/FlowNotAllocatedException");
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
%typemap(throws, throws="eu.irati.librina.AssignToDIFException") rina::AssignToDIFException {
  jclass excep = jenv->FindClass("eu/irati/librina/AssignToDIFException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.NotifyRegistrationToDIFException") rina::NotifyRegistrationToDIFException {
  jclass excep = jenv->FindClass("eu/irati/librina/NotifyRegistrationToDIFException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.NotifyUnregistrationFromDIFException") rina::NotifyUnregistrationFromDIFException {
  jclass excep = jenv->FindClass("eu/irati/librina/NotifyUnregistrationFromDIFException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.EnrollException") rina::EnrollException {
  jclass excep = jenv->FindClass("eu/irati/librina/EnrollException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.DisconnectFromNeighborException") rina::DisconnectFromNeighborException {
  jclass excep = jenv->FindClass("eu/irati/librina/DisconnectFromNeighborException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.IpcmRegisterApplicationException") rina::IpcmRegisterApplicationException {
  jclass excep = jenv->FindClass("eu/irati/librina/IpcmRegisterApplicationException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.IpcmUnregisterApplicationException") rina::IpcmUnregisterApplicationException {
  jclass excep = jenv->FindClass("eu/irati/librina/IpcmUnregisterApplicationException");
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
%typemap(throws, throws="eu.irati.librina.QueryRIBException") rina::QueryRIBException {
  jclass excep = jenv->FindClass("eu/irati/librina/QueryRIBException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.CreateIPCProcessException") rina::CreateIPCProcessException {
  jclass excep = jenv->FindClass("eu/irati/librina/CreateIPCProcessException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.DestroyIPCProcessException") rina::DestroyIPCProcessException {
  jclass excep = jenv->FindClass("eu/irati/librina/DestroyIPCProcessException");
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
%typemap(throws, throws="eu.irati.librina.AssignToDIFResponseException") rina::AssignToDIFResponseException {
  jclass excep = jenv->FindClass("eu/irati/librina/AssignToDIFResponseException");
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
%typemap(throws, throws="eu.irati.librina.QueryRIBResponseException") rina::QueryRIBResponseException {
  jclass excep = jenv->FindClass("eu/irati/librina/QueryRIBResponseException");
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

/* Typemaps to allow eventWait, eventPoll and eventTimedWait to downcast IPCEvent to the correct class */
%define DOWNCAST_IPC_EVENT_CONSUMER( OPERATION )
%typemap(jni) rina::IPCEvent *rina::IPCEventProducer::OPERATION "jobject"
%typemap(jtype) rina::IPCEvent *rina::IPCEventProducer::OPERATION "eu.irati.librina.IPCEvent"
%typemap(jstype) rina::IPCEvent *rina::IPCEventProducer::OPERATION "eu.irati.librina.IPCEvent"
%typemap(javaout) rina::IPCEvent *rina::IPCEventProducer::OPERATION {
    return $jnicall;
  }

%typemap(out) rina::IPCEvent *rina::IPCEventProducer::OPERATION {
    if ($1->getType() == rina::APPLICATION_REGISTRATION_REQUEST_EVENT) {
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
    } else if ($1->getType() == rina::APPLICATION_UNREGISTRATION_REQUEST_EVENT) {
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
    } else if ($1->getType() == rina::FLOW_ALLOCATION_REQUESTED_EVENT) {
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
    } else if ($1->getType() == rina::FLOW_DEALLOCATION_REQUESTED_EVENT) {
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
    } else if ($1->getType() == rina::FLOW_DEALLOCATED_EVENT) {
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
    }
}
%enddef

DOWNCAST_IPC_EVENT_CONSUMER(eventWait);
DOWNCAST_IPC_EVENT_CONSUMER(eventPoll);
DOWNCAST_IPC_EVENT_CONSUMER(eventTimedWait);

%{
#include "exceptions.h"
#include "patterns.h"
#include "concurrency.h"
#include "librina-common.h"
#include "librina-application.h"
#include "librina-ipc-manager.h"
#include "librina-ipc-process.h"
%}

%rename(equals) rina::ApplicationProcessNamingInformation::operator==(const ApplicationProcessNamingInformation &other) const;
%rename(differs) rina::ApplicationProcessNamingInformation::operator!=(const ApplicationProcessNamingInformation &other) const;
%rename(isLessThanOrEquals) rina::ApplicationProcessNamingInformation::operator<=(const ApplicationProcessNamingInformation &other) const;   
%rename(isLessThan) rina::ApplicationProcessNamingInformation::operator<(const ApplicationProcessNamingInformation &other) const;
%rename(isMoreThanOrEquals) rina::ApplicationProcessNamingInformation::operator>=(const ApplicationProcessNamingInformation &other) const;   
%rename(isMoreThan) rina::ApplicationProcessNamingInformation::operator>(const ApplicationProcessNamingInformation &other) const;  
%rename(equals) rina::QoSCube::operator==(const QoSCube &other) const;  
%rename(differs) rina::QoSCube::operator!=(const QoSCube &other) const; 
%rename(equals) rina::FlowSpecification::operator==(const FlowSpecification &other) const;
%rename(differs) rina::FlowSpecification::operator!=(const FlowSpecification &other) const;
%rename(equals) rina::RIBObject::operator==(const RIBObject &other) const;
%rename(differs) rina::RIBObject::operator!=(const RIBObject &other) const;
%rename(equals) rina::Thread::operator==(const Thread &other) const;
%rename(differs) rina::Thread::operator!=(const Thread &other) const;

%include "exceptions.h"
%include "patterns.h"
%include "concurrency.h"
%include "librina-common.h"
%include "librina-application.h"
%include "librina-ipc-manager.h"
%include "librina-ipc-process.h"

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
/* Define iterator for RIBObject list */
MAKE_COLLECTION_ITERABLE(RIBObjectListIterator, RIBObject, std::list, rina::RIBObject);
/* Define iterator for String list */
MAKE_COLLECTION_ITERABLE(StringListIterator, String, std::list, std::string);

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
%template(ExtendedIPCManagerSingleton) Singleton<rina::ExtendedIPCManager>;
%template(RIBObjectList) std::list<rina::RIBObject>;
%template(StringList) std::list<std::string>;


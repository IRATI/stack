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

%include "enums.swg"
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
%typemap(throws, throws="eu.irati.librina.InitializationException") rina::InitializationException {
  jclass excep = jenv->FindClass("eu/irati/librina/InitializationException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.GetIPCProcessException") rina::GetIPCProcessException {
  jclass excep = jenv->FindClass("eu/irati/librina/GetIPCProcessException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.UpdateDIFConfigurationException") rina::UpdateDIFConfigurationException {
  jclass excep = jenv->FindClass("eu/irati/librina/UpdateDIFConfigurationException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.CreateConnectionException") rina::CreateConnectionException {
  jclass excep = jenv->FindClass("eu/irati/librina/CreateConnectionException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.UpdateConnectionException") rina::UpdateConnectionException {
  jclass excep = jenv->FindClass("eu/irati/librina/UpdateConnectionException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.DestroyConnectionException") rina::DestroyConnectionException {
  jclass excep = jenv->FindClass("eu/irati/librina/DestroyConnectionException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}
%typemap(throws, throws="eu.irati.librina.PDUForwardingTableException") rina::PDUForwardingTableException {
  jclass excep = jenv->FindClass("eu/irati/librina/PDUForwardingTableException");
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
    } else if ($1->getType() == rina::OS_PROCESS_FINALIZED) {
    	rina::OSProcessFinalizedEvent *flowReqEvent = dynamic_cast<rina::OSProcessFinalizedEvent *>($1);
        jclass clazz = jenv->FindClass("eu/irati/librina/OSProcessFinalizedEvent");
        if (clazz) {
            jmethodID mid = jenv->GetMethodID(clazz, "<init>", "(JZ)V");
            if (mid) {
                jlong cptr = 0;
                *(rina::OSProcessFinalizedEvent **)&cptr = flowReqEvent; 
                $result = jenv->NewObject(clazz, mid, cptr, false);
            }
        }
    } else if ($1->getType() == rina::REGISTER_APPLICATION_RESPONSE_EVENT) {
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
    } else if ($1->getType() == rina::UNREGISTER_APPLICATION_RESPONSE_EVENT) {
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
    } else if ($1->getType() == rina::ALLOCATE_FLOW_RESPONSE_EVENT) {
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
    } else if ($1->getType() == rina::ALLOCATE_FLOW_REQUEST_RESULT_EVENT) {
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
    } else if ($1->getType() == rina::DEALLOCATE_FLOW_RESPONSE_EVENT) {
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
    } else if ($1->getType() == rina::GET_DIF_PROPERTIES_RESPONSE_EVENT) {
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
    } else if ($1->getType() == rina::IPCM_REGISTER_APP_RESPONSE_EVENT) {
    	rina::IpcmRegisterApplicationResponseEvent *flowReqEvent = dynamic_cast<rina::IpcmRegisterApplicationResponseEvent *>($1);
        jclass clazz = jenv->FindClass("eu/irati/librina/IpcmRegisterApplicationResponseEvent");
        if (clazz) {
            jmethodID mid = jenv->GetMethodID(clazz, "<init>", "(JZ)V");
            if (mid) {
                jlong cptr = 0;
                *(rina::IpcmRegisterApplicationResponseEvent **)&cptr = flowReqEvent; 
                $result = jenv->NewObject(clazz, mid, cptr, false);
            }
        }
    } else if ($1->getType() == rina::IPCM_UNREGISTER_APP_RESPONSE_EVENT) {
    	rina::IpcmUnregisterApplicationResponseEvent *flowReqEvent = dynamic_cast<rina::IpcmUnregisterApplicationResponseEvent *>($1);
        jclass clazz = jenv->FindClass("eu/irati/librina/IpcmUnregisterApplicationResponseEvent");
        if (clazz) {
            jmethodID mid = jenv->GetMethodID(clazz, "<init>", "(JZ)V");
            if (mid) {
                jlong cptr = 0;
                *(rina::IpcmUnregisterApplicationResponseEvent **)&cptr = flowReqEvent; 
                $result = jenv->NewObject(clazz, mid, cptr, false);
            }
        }
    } else if ($1->getType() == rina::IPCM_DEALLOCATE_FLOW_RESPONSE_EVENT) {
    	rina::IpcmDeallocateFlowResponseEvent *flowReqEvent = dynamic_cast<rina::IpcmDeallocateFlowResponseEvent *>($1);
        jclass clazz = jenv->FindClass("eu/irati/librina/IpcmDeallocateFlowResponseEvent");
        if (clazz) {
            jmethodID mid = jenv->GetMethodID(clazz, "<init>", "(JZ)V");
            if (mid) {
                jlong cptr = 0;
                *(rina::IpcmDeallocateFlowResponseEvent **)&cptr = flowReqEvent; 
                $result = jenv->NewObject(clazz, mid, cptr, false);
            }
        }
    } else if ($1->getType() == rina::IPCM_ALLOCATE_FLOW_REQUEST_RESULT) {
    	rina::IpcmAllocateFlowRequestResultEvent *flowReqEvent = dynamic_cast<rina::IpcmAllocateFlowRequestResultEvent *>($1);
        jclass clazz = jenv->FindClass("eu/irati/librina/IpcmAllocateFlowRequestResultEvent");
        if (clazz) {
            jmethodID mid = jenv->GetMethodID(clazz, "<init>", "(JZ)V");
            if (mid) {
                jlong cptr = 0;
                *(rina::IpcmAllocateFlowRequestResultEvent **)&cptr = flowReqEvent; 
                $result = jenv->NewObject(clazz, mid, cptr, false);
            }
        }
    } else if ($1->getType() == rina::IPC_PROCESS_QUERY_RIB) {
    	rina::QueryRIBRequestEvent *flowReqEvent = dynamic_cast<rina::QueryRIBRequestEvent *>($1);
        jclass clazz = jenv->FindClass("eu/irati/librina/QueryRIBRequestEvent");
        if (clazz) {
            jmethodID mid = jenv->GetMethodID(clazz, "<init>", "(JZ)V");
            if (mid) {
                jlong cptr = 0;
                *(rina::QueryRIBRequestEvent **)&cptr = flowReqEvent; 
                $result = jenv->NewObject(clazz, mid, cptr, false);
            }
        }
    } else if ($1->getType() == rina::QUERY_RIB_RESPONSE_EVENT) {
    	rina::QueryRIBResponseEvent *flowReqEvent = dynamic_cast<rina::QueryRIBResponseEvent *>($1);
        jclass clazz = jenv->FindClass("eu/irati/librina/QueryRIBResponseEvent");
        if (clazz) {
            jmethodID mid = jenv->GetMethodID(clazz, "<init>", "(JZ)V");
            if (mid) {
                jlong cptr = 0;
                *(rina::QueryRIBResponseEvent **)&cptr = flowReqEvent; 
                $result = jenv->NewObject(clazz, mid, cptr, false);
            }
        }
    } else if ($1->getType() == rina::ASSIGN_TO_DIF_RESPONSE_EVENT) {
    	rina::AssignToDIFResponseEvent *flowReqEvent = dynamic_cast<rina::AssignToDIFResponseEvent *>($1);
        jclass clazz = jenv->FindClass("eu/irati/librina/AssignToDIFResponseEvent");
        if (clazz) {
            jmethodID mid = jenv->GetMethodID(clazz, "<init>", "(JZ)V");
            if (mid) {
                jlong cptr = 0;
                *(rina::AssignToDIFResponseEvent **)&cptr = flowReqEvent; 
                $result = jenv->NewObject(clazz, mid, cptr, false);
            }
        }
    } else if ($1->getType() == rina::ASSIGN_TO_DIF_REQUEST_EVENT) {
    	rina::AssignToDIFRequestEvent *flowReqEvent = dynamic_cast<rina::AssignToDIFRequestEvent *>($1);
        jclass clazz = jenv->FindClass("eu/irati/librina/AssignToDIFRequestEvent");
        if (clazz) {
            jmethodID mid = jenv->GetMethodID(clazz, "<init>", "(JZ)V");
            if (mid) {
                jlong cptr = 0;
                *(rina::AssignToDIFRequestEvent **)&cptr = flowReqEvent; 
                $result = jenv->NewObject(clazz, mid, cptr, false);
            }
        }
    } else if ($1->getType() == rina::UPDATE_DIF_CONFIG_RESPONSE_EVENT) {
    	rina::UpdateDIFConfigurationResponseEvent *flowReqEvent = dynamic_cast<rina::UpdateDIFConfigurationResponseEvent *>($1);
        jclass clazz = jenv->FindClass("eu/irati/librina/UpdateDIFConfigurationResponseEvent");
        if (clazz) {
            jmethodID mid = jenv->GetMethodID(clazz, "<init>", "(JZ)V");
            if (mid) {
                jlong cptr = 0;
                *(rina::UpdateDIFConfigurationResponseEvent **)&cptr = flowReqEvent; 
                $result = jenv->NewObject(clazz, mid, cptr, false);
            }
        }
    } else if ($1->getType() == rina::IPC_PROCESS_DIF_REGISTRATION_NOTIFICATION) {
    	rina::IPCProcessDIFRegistrationEvent *flowReqEvent = dynamic_cast<rina::IPCProcessDIFRegistrationEvent *>($1);
        jclass clazz = jenv->FindClass("eu/irati/librina/IPCProcessDIFRegistrationEvent");
        if (clazz) {
            jmethodID mid = jenv->GetMethodID(clazz, "<init>", "(JZ)V");
            if (mid) {
                jlong cptr = 0;
                *(rina::IPCProcessDIFRegistrationEvent **)&cptr = flowReqEvent; 
                $result = jenv->NewObject(clazz, mid, cptr, false);
            }
        }
    } else if ($1->getType() == rina::IPC_PROCESS_DAEMON_INITIALIZED_EVENT) {
    	rina::IPCProcessDaemonInitializedEvent *flowReqEvent = dynamic_cast<rina::IPCProcessDaemonInitializedEvent *>($1);
        jclass clazz = jenv->FindClass("eu/irati/librina/IPCProcessDaemonInitializedEvent");
        if (clazz) {
            jmethodID mid = jenv->GetMethodID(clazz, "<init>", "(JZ)V");
            if (mid) {
                jlong cptr = 0;
                *(rina::IPCProcessDaemonInitializedEvent **)&cptr = flowReqEvent; 
                $result = jenv->NewObject(clazz, mid, cptr, false);
            }
        }
    } else if ($1->getType() == rina::ENROLL_TO_DIF_REQUEST_EVENT) {
    	rina::EnrollToDIFRequestEvent *flowReqEvent = dynamic_cast<rina::EnrollToDIFRequestEvent *>($1);
        jclass clazz = jenv->FindClass("eu/irati/librina/EnrollToDIFRequestEvent");
        if (clazz) {
            jmethodID mid = jenv->GetMethodID(clazz, "<init>", "(JZ)V");
            if (mid) {
                jlong cptr = 0;
                *(rina::EnrollToDIFRequestEvent **)&cptr = flowReqEvent; 
                $result = jenv->NewObject(clazz, mid, cptr, false);
            }
        }
    } else if ($1->getType() == rina::ENROLL_TO_DIF_RESPONSE_EVENT) {
    	 rina::EnrollToDIFResponseEvent *flowReqEvent = dynamic_cast<rina::EnrollToDIFResponseEvent *>($1);
         jclass clazz = jenv->FindClass("eu/irati/librina/EnrollToDIFResponseEvent");
         if (clazz) {
             jmethodID mid = jenv->GetMethodID(clazz, "<init>", "(JZ)V");
             if (mid) {
                 jlong cptr = 0;
                 *(rina::EnrollToDIFResponseEvent **)&cptr = flowReqEvent; 
                 $result = jenv->NewObject(clazz, mid, cptr, false);
          }
       }
    } else if ($1->getType() == rina::NEIGHBORS_MODIFIED_NOTIFICAITON_EVENT) {
    	 rina::NeighborsModifiedNotificationEvent *flowReqEvent = dynamic_cast<rina::NeighborsModifiedNotificationEvent *>($1);
         jclass clazz = jenv->FindClass("eu/irati/librina/NeighborsModifiedNotificationEvent");
         if (clazz) {
             jmethodID mid = jenv->GetMethodID(clazz, "<init>", "(JZ)V");
             if (mid) {
                 jlong cptr = 0;
                 *(rina::NeighborsModifiedNotificationEvent **)&cptr = flowReqEvent; 
                 $result = jenv->NewObject(clazz, mid, cptr, false);
          }
       }
    } else if ($1->getType() == rina::IPC_PROCESS_CREATE_CONNECTION_RESPONSE) {
    	 rina::CreateConnectionResponseEvent *flowReqEvent = dynamic_cast<rina::CreateConnectionResponseEvent *>($1);
         jclass clazz = jenv->FindClass("eu/irati/librina/CreateConnectionResponseEvent");
         if (clazz) {
             jmethodID mid = jenv->GetMethodID(clazz, "<init>", "(JZ)V");
             if (mid) {
                 jlong cptr = 0;
                 *(rina::CreateConnectionResponseEvent **)&cptr = flowReqEvent; 
                 $result = jenv->NewObject(clazz, mid, cptr, false);
          }
       }
    } else if ($1->getType() == rina::IPC_PROCESS_UPDATE_CONNECTION_RESPONSE) {
    	 rina::UpdateConnectionResponseEvent *flowReqEvent = dynamic_cast<rina::UpdateConnectionResponseEvent *>($1);
         jclass clazz = jenv->FindClass("eu/irati/librina/UpdateConnectionResponseEvent");
         if (clazz) {
             jmethodID mid = jenv->GetMethodID(clazz, "<init>", "(JZ)V");
             if (mid) {
                 jlong cptr = 0;
                 *(rina::UpdateConnectionResponseEvent **)&cptr = flowReqEvent; 
                 $result = jenv->NewObject(clazz, mid, cptr, false);
          }
       }
    } else if ($1->getType() == rina::IPC_PROCESS_CREATE_CONNECTION_RESULT) {
    	 rina::CreateConnectionResultEvent *flowReqEvent = dynamic_cast<rina::CreateConnectionResultEvent *>($1);
         jclass clazz = jenv->FindClass("eu/irati/librina/CreateConnectionResultEvent");
         if (clazz) {
             jmethodID mid = jenv->GetMethodID(clazz, "<init>", "(JZ)V");
             if (mid) {
                 jlong cptr = 0;
                 *(rina::CreateConnectionResultEvent **)&cptr = flowReqEvent; 
                 $result = jenv->NewObject(clazz, mid, cptr, false);
          }
       }
    } else if ($1->getType() == rina::IPC_PROCESS_DESTROY_CONNECTION_RESULT) {
    	 rina::DestroyConnectionResultEvent *flowReqEvent = dynamic_cast<rina::DestroyConnectionResultEvent *>($1);
         jclass clazz = jenv->FindClass("eu/irati/librina/DestroyConnectionResultEvent");
         if (clazz) {
             jmethodID mid = jenv->GetMethodID(clazz, "<init>", "(JZ)V");
             if (mid) {
                 jlong cptr = 0;
                 *(rina::DestroyConnectionResultEvent **)&cptr = flowReqEvent; 
                 $result = jenv->NewObject(clazz, mid, cptr, false);
          }
       }
    } else if ($1->getType() == rina::IPC_PROCESS_DUMP_FT_RESPONSE) {
    	 rina::DumpFTResponseEvent *flowReqEvent = dynamic_cast<rina::DumpFTResponseEvent *>($1);
         jclass clazz = jenv->FindClass("eu/irati/librina/DumpFTResponseEvent");
         if (clazz) {
             jmethodID mid = jenv->GetMethodID(clazz, "<init>", "(JZ)V");
             if (mid) {
                 jlong cptr = 0;
                 *(rina::DumpFTResponseEvent **)&cptr = flowReqEvent; 
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
#include "common.h"
#include "application.h"
#include "ipc-manager.h"
#include "ipc-process.h"
%}

%rename(equals) rina::ConnectionPoliciesParameters::operator==(const ConnectionPoliciesParameters &other) const;
%rename(differs) rina::ConnectionPoliciesParameters::operator!=(const ConnectionPoliciesParameters &other) const;
%rename(differs) rina::ApplicationProcessNamingInformation::operator!=(const ApplicationProcessNamingInformation &other) const;
%rename(equals) rina::ApplicationProcessNamingInformation::operator==(const ApplicationProcessNamingInformation &other) const;
%rename(assign) rina::ApplicationProcessNamingInformation::operator=(const ApplicationProcessNamingInformation &other);
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
%rename(equals) rina::Parameter::operator==(const Parameter &other) const;
%rename(differs) rina::Parameter::operator!=(const Parameter &other) const;
%rename(equals) rina::Policy::operator==(const Policy &other) const;
%rename(differs) rina::Policy::operator!=(const Policy &other) const;
%rename(equals) rina::FlowInformation::operator==(const FlowInformation &other) const;
%rename(differs) rina::FlowInformation::operator!=(const FlowInformation &other) const;
%rename(equals) rina::Neighbor::operator==(const Neighbor &other) const;
%rename(differs) rina::Neighbor::operator!=(const Neighbor &other) const;
%rename(equals) rina::PDUForwardingTableEntry::operator==(const PDUForwardingTableEntry &other) const;
%rename(differs) rina::PDUForwardingTableEntry::operator!=(const PDUForwardingTableEntry &other) const;
%rename(equals) rina::PolicyParameter::operator==(const PolicyParameter &other) const;
%rename(differs) rina::PolicyParameter::operator!=(const PolicyParameter &other) const;
%rename(equals) rina::PolicyConfig::operator==(const PolicyConfig &other) const;
%rename(differs) rina::PolicyConfig::operator!=(const PolicyConfig &other) const;

%include "exceptions.h"
%include "patterns.h"
%include "concurrency.h"
%include "common.h"
%include "application.h"
%include "ipc-manager.h"
%include "ipc-process.h"

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
/* Define iterator for Flow Information list */
MAKE_COLLECTION_ITERABLE(FlowInformationListIterator, FlowInformation, std::list, rina::FlowInformation);
/* Define iterator for Neighbors list */
MAKE_COLLECTION_ITERABLE(NeighborListIterator, Neighbor, std::list, rina::Neighbor);
/* Define iterator for PDU Forwarding Table Entry list */
MAKE_COLLECTION_ITERABLE(PDUForwardingTableListIterator, PDUForwardingTableEntry, std::list, rina::PDUForwardingTableEntry);
/* Define iterator for Unsigned int list */
MAKE_COLLECTION_ITERABLE(UnsignedIntListIterator, Long, std::list, unsigned int);
/* Define iterator for PolicyParameter list */
MAKE_COLLECTION_ITERABLE(PolicyParameterListIterator, PolicyParameter, std::list, rina::PolicyParameter);
/* Define iterator for Parameter list */
MAKE_COLLECTION_ITERABLE(ParameterListIterator, Parameter, std::list, rina::Parameter);

%template(DIFPropertiesVector) std::vector<rina::DIFProperties>;
%template(FlowVector) std::vector<rina::Flow>;
%template(FlowPointerVector) std::vector<rina::Flow *>;
%template(ApplicationRegistrationVector) std::vector<rina::ApplicationRegistration *>;
%template(QoSCubeList) std::list<rina::QoSCube>;
%template(QoSCubeVector) std::vector<rina::QoSCube>;
%template(PolicyConfigList) std::list<rina::PolicyConfig>;
%template(ParameterList) std::list<rina::Parameter>;
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
%template(FlowInformationList) std::list<rina::FlowInformation>;
%template(KernelIPCProcessSingleton) Singleton<rina::KernelIPCProcess>;
%template(NeighborList) std::list<rina::Neighbor>;
%template(PDUForwardingTableEntryList) std::list<rina::PDUForwardingTableEntry>;
%template(UnsignedIntList) std::list<unsigned int>;
%template(PolicyParameterList) std::list<rina::PolicyParameter>;


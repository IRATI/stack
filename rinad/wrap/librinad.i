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

%module rinad

%include <std_string.i>
%include "enums.swg"
%include <stdint.i>
%include <stl.i>

%include "stdlist.i"
%import "librina.i"

#ifdef SWIGJAVA
#endif

%pragma(java) jniclassimports=%{
import eu.irati.librina.ser_obj_t;
import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.FlowSpecification;
import eu.irati.librina.Neighbor;
import eu.irati.librina.ADataObject;
import eu.irati.librina.DataTransferConstants;
import eu.irati.librina.DTCPConfig;
import eu.irati.librina.DIFInformation;
import eu.irati.librina.QoSCube;
import eu.irati.librina.RIBObjectData;
%}

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
 
%typemap(javaimports) SWIGTYPE 
%{
import eu.irati.librina.ser_obj_t;
import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.FlowSpecification;
import eu.irati.librina.Neighbor;
import eu.irati.librina.ADataObject;
import eu.irati.librina.DataTransferConstants;
import eu.irati.librina.DTCPConfig;
import eu.irati.librina.DIFInformation;
import eu.irati.librina.QoSCube;
import eu.irati.librina.RIBObjectData;
%}

%{
#include "librina/common.h"
#include "configuration.h"
#include "librina/cdap_rib_structures.h"
#include "encoder.h"
#include <string>
%}


%include "configuration.h"

namespace rina{
%template(TempDTEncoder) Encoder< rina::DataTransferConstants >;
%template(TempDFTEncoder) Encoder<rina::DirectoryForwardingTableEntry>;
%template(TempDFTLEncoder) Encoder<std::list<rina::DirectoryForwardingTableEntry> >;
%template(TempQOSCEncoder) Encoder<rina::QoSCube>;
%template(TempQOSCLEncoder) Encoder<std::list<rina::QoSCube> >;
%template(TempWNEncoder) Encoder<rina::WhatevercastName>;
%template(TempWNLEncoder) Encoder<std::list<rina::WhatevercastName> >;
%template(TempNeEncoder) Encoder<rina::Neighbor>;
%template(TempNeLEncoder) Encoder<std::list<rina::Neighbor> >;
%template(TempADOEncoder) Encoder<cdap::ADataObject>;
%template(TempIPCPConfigEncoder) Encoder<rinad::configs::ipcp_config_t>;
%template(TempIPCPEncoder) Encoder<rinad::configs::ipcp_t>;
%template(TempEIREncoder) Encoder<rinad::configs::EnrollmentInformationRequest>;
%template(TempFEncoder) Encoder<rinad::configs::Flow>;
%template(TempRTEncoder) Encoder<rina::RoutingTableEntry >;
%template(TempPDUFTEncoder) Encoder<rina::PDUForwardingTableEntry >;
%template(TempDTPIEncoder) Encoder<rina::DTPInformation >;
%template(TempDTCPCEncoder) Encoder<rina::DTCPConfig >;
%template(TempRIBObjectDataListEncoder) rina::Encoder< std::list< rina::rib::RIBObjectData > >;
}

%include "encoder.h"


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

%template(StringList) std::list<std::string>;
%template(RIBObjectDataList) std::list<rina::rib::RIBObjectData>;

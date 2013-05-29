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

%include "stdint.i"

/* 
 * unsigned char * typemaps. 
 * These are input typemaps for mapping a Java byte[] array to a C char array.
 * Note that as a Java array is used and thus passeed by reference, the C routine 
 * can return data to Java via the parameter.
 *
 * Example usage wrapping:
 *   void foo(unsigned char *array);
 *  
 * Java usage:
 *   byte b[] = new byte[20];
 *   modulename.foo(b);
 */
%typemap(jni) unsigned char * "jbyteArray"
%typemap(jtype) unsigned char * "byte[]"
%typemap(jstype) unsigned char * "byte[]"
%typemap(in) unsigned char * {
  $1 = (unsigned char *) JCALL2(GetByteArrayElements, jenv, $input, 0); 
}

%typemap(argout) unsigned char * {
  JCALL3(ReleaseByteArrayElements, jenv, $input, (jbyte *) $1, 0); 
}

%typemap(javain) unsigned char * "$javainput"

%typemap(javaout) unsigned char* {
    return $jnicall;
 }

/* Prevent default freearg typemap from being used */
%typemap(freearg) unsigned char * ""

%{
#include "librina-common.h"
#include "librina-application.h"
#include "librina-ipc-manager.h"
#include "librina-ipc-process.h"
#include "librina-faux-sockets.h"
#include "librina-cdap.h"
#include "librina-sdu-protection.h"
%}

%include "librina-common.h"
%include "librina-application.h"
%include "librina-ipc-manager.h"
%include "librina-ipc-process.h"
%include "librina-faux-sockets.h"
%include "librina-cdap.h"
%include "librina-sdu-protection.h"

/*
 * make dif_properties_t * into an array
 */
%include <carrays.i>
%array_functions(dif_properties_t, dif_properties_t_array);

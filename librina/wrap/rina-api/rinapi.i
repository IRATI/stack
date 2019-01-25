 %module rinapi
 %{
 /* Includes the header in the wrapper code */
 #include "rina/api.h"
 %}

%apply int { uint32_t };
%apply long { uint64_t };
%apply short { uint16_t };
%apply short { uint8_t };

// Type typemaps for marshalling char **
%typemap(jni) char ** "jstring"
%typemap(jtype) char ** "java.lang.String"
%typemap(jstype) char ** "java.lang.String"

// Typemaps for char ** as a parameter output type
%typemap(in) char ** (char *ppstring = 0) %{
  $1 = &ppstring;
%}
%typemap(argout) char ** {
  // Give Java proxy the C pointer (of newly created object)
  jclass clazz = (*jenv)->FindClass(jenv, "jstring");
  jfieldID fid = (*jenv)->GetFieldID(jenv, clazz, "swigCPtr", "J");
  jlong cPtr = 0;
  *(char **)&cPtr = *$1;
  (*jenv)->SetLongField(jenv, $input, fid, cPtr);
}
%typemap(javain) char ** "$javainput"

 /* Parse the header file to generate wrappers */
 %include "rina/api.h"

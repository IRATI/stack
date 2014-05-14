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

#include "librina-cdap.h"

namespace rina
{
  /* CLASS AuthValue      */

  AuthValue::AuthValue()
  {
    authOther = NULL;
  }

  std::string
  AuthValue::getAuthName() const
  {
    return authName;
  }

  void
  AuthValue::setAuthName(std::string arg0)
  {
    authName = arg0;
  }

  std::string
  AuthValue::getAuthPassword() const
  {
    return authPassword;
  }

  void
  AuthValue::setAuthPassword(std::string arg0)
  {
    authPassword = arg0;
  }

  char*
  AuthValue::getAuthOther() const
  {
    return authOther;
  }

  void
  AuthValue::setAuthOther(char* arg0)
  {
    authOther = arg0;
  }

  /* CLASS ObjectValue */

  ObjectValue::ObjectValue()
  {
    intval = 0;
    sintval = 0;
    int64val = 0;
    sint64val = 0;
    byteval = NULL;
    floatval = 0;
    doubleval = 0;
    booleanval = false;
  }

  int
  ObjectValue::getIntval() const
  {
    return intval;
  }

  void
  ObjectValue::setIntval(int arg0)
  {
    intval = arg0;
  }

  int
  ObjectValue::getSintval() const
  {
    return sintval;
  }

  void
  ObjectValue::setSintval(int arg0)
  {
    sintval = arg0;
  }

  long
  ObjectValue::getInt64val() const
  {
    return int64val;
  }

  void
  ObjectValue::setInt64val(long arg0)
  {
    int64val = arg0;
  }

  long
  ObjectValue::getSint64val() const
  {
    return sint64val;
  }

  void
  ObjectValue::setSint64val(long arg0)
  {
    sint64val = arg0;
  }

  std::string
  ObjectValue::getStrval() const
  {
    return strval;
  }

  void
  ObjectValue::setStrval(std::string arg0)
  {
    strval = arg0;
  }

  char*
  ObjectValue::getByteval() const
  {
    return byteval;
  }

  void
  ObjectValue::setByteval(char arg0[])
  {
    byteval = arg0;
  }

  int
  ObjectValue::getFloatval() const
  {
    return floatval;
  }

  void
  ObjectValue::setFloatval(int arg0)
  {
    floatval = arg0;
  }

  long
  ObjectValue::getDoubleval() const
  {
    return doubleval;
  }

  void
  ObjectValue::setDoubleval(long arg0)
  {
    doubleval = arg0;
  }

  void
  ObjectValue::setBooleanval(bool arg0)
  {
    booleanval = arg0;
  }

  bool
  ObjectValue::isBooleanval() const
  {
    return booleanval;

  }

  /* CLASS CDAPMessage */
  CDAPMessage::CDAPMessage()
  {
    absSyntax = 0;
    authMech = AUTH_NONE;
    filter = 0;
    flags = NONE_FLAGS;
    invokeID = 0;
    objInst = 0;
    opCode = NONE_OPCODE;
    result = 0;
    scope = 0;
    version = 0;
  }

  CDAPMessage*
  CDAPMessage::getOpenConnectionRequestMessage(AuthTypes authMech,
      const AuthValue &authValue, std::string destAEInst,
      std::string destAEName, std::string destApInst, std::string destApName,
      std::string srcAEInst, std::string srcAEName, std::string srcApInst,
      std::string srcApName, int invokeID) throw (CDAPException)
  {
    CDAPMessage cdapMessage;
    cdapMessage.setAbsSyntax(ABSTRACT_SYNTAX_VERSION);
    cdapMessage.setAuthMech(authMech);
    cdapMessage.setAuthValue(authValue);
    cdapMessage.setDestAEInst(destAEInst);
    cdapMessage.setDestAEName(destAEName);
    cdapMessage.setDestApInst(destApInst);
    cdapMessage.setDestApInst(destApInst);
    cdapMessage.setDestApName(destApName);
    cdapMessage.setInvokeID(invokeID);
    cdapMessage.setOpCode(M_CONNECT);
    cdapMessage.setSrcAEInst(srcAEInst);
    cdapMessage.setSrcAEName(srcAEName);
    cdapMessage.setSrcApInst(srcApInst);
    cdapMessage.setSrcApName(srcApName);
    cdapMessage.setVersion(1);
    CDAPMessageValidator.validate(cdapMessage);
    return &cdapMessage;
  }

  CDAPMessage*
  CDAPMessage::getOpenConnectionResponseMessage(AuthTypes authMech,
      const AuthValue &authValue, std::string destAEInst,
      std::string destAEName, std::string destApInst, std::string destApName,
      int result, std::string resultReason, std::string srcAEInst,
      std::string srcAEName, std::string srcApInst, std::string srcApName,
      int invokeID) throw (CDAPException)
  {
    CDAPMessage cdapMessage;
    cdapMessage.setAbsSyntax(ABSTRACT_SYNTAX_VERSION);
    cdapMessage.setAuthMech(authMech);
    cdapMessage.setAuthValue(authValue);
    cdapMessage.setDestAEInst(destAEInst);
    cdapMessage.setDestAEName(destAEName);
    cdapMessage.setDestApInst(destApInst);
    cdapMessage.setDestApInst(destApInst);
    cdapMessage.setDestApName(destApName);
    cdapMessage.setOpCode(M_CONNECT_R);
    cdapMessage.setInvokeID(invokeID);
    cdapMessage.setResult(result);
    cdapMessage.setResultReason(resultReason);
    cdapMessage.setSrcAEInst(srcAEInst);
    cdapMessage.setSrcAEName(srcAEName);
    cdapMessage.setSrcApInst(srcApInst);
    cdapMessage.setSrcApName(srcApName);
    cdapMessage.setVersion(1);
    CDAPMessageValidator.validate(cdapMessage);
    return &cdapMessage;
  }

  CDAPMessage*
  CDAPMessage::getReleaseConnectionRequestMessage(Flags flags)
      throw (CDAPException)
  {
    CDAPMessage cdapMessage;
    cdapMessage.setFlags(flags);
    cdapMessage.setOpCode(M_RELEASE);
    CDAPMessageValidator.validate(cdapMessage);
    return &cdapMessage;
  }

  CDAPMessage*
  CDAPMessage::getReleaseConnectionResponseMessage(Flags flags, int result,
      std::string resultReason, int invokeID) throw (CDAPException)
  {
    CDAPMessage cdapMessage;
    cdapMessage.setFlags(flags);
    cdapMessage.setInvokeID(invokeID);
    cdapMessage.setOpCode(M_RELEASE_R);
    cdapMessage.setResult(result);
    cdapMessage.setResultReason(resultReason);
    CDAPMessageValidator.validate(cdapMessage);
    return &cdapMessage;
  }

  CDAPMessage*
  CDAPMessage::getCreateObjectRequestMessage(char filter[], Flags flags,
      std::string objClass, long objInst, std::string objName,
      const ObjectValue &objValue, int scope) throw (CDAPException)
  {
    CDAPMessage cdapMessage;
    cdapMessage.setFilter(filter);
    cdapMessage.setFlags(flags);
    cdapMessage.setObjClass(objClass);
    cdapMessage.setObjInst(objInst);
    cdapMessage.setObjName(objName);
    cdapMessage.setObjValue(objValue);
    cdapMessage.setOpCode(M_CREATE);
    cdapMessage.setScope(scope);
    CDAPMessageValidator.validate(cdapMessage);
    return &cdapMessage;
  }

  CDAPMessage*
  CDAPMessage::getCreateObjectResponseMessage(Flags flags, std::string objClass,
      long objInst, std::string objName, const ObjectValue &objValue,
      int result, std::string resultReason, int invokeID) throw (CDAPException)
  {
    CDAPMessage cdapMessage;
    cdapMessage.setFlags(flags);
    cdapMessage.setInvokeID(invokeID);
    cdapMessage.setObjClass(objClass);
    cdapMessage.setObjInst(objInst);
    cdapMessage.setObjName(objName);
    cdapMessage.setObjValue(objValue);
    cdapMessage.setOpCode(M_CREATE_R);
    cdapMessage.setResult(result);
    cdapMessage.setResultReason(resultReason);
    CDAPMessageValidator.validate(cdapMessage);
    return &cdapMessage;
  }

  CDAPMessage*
  CDAPMessage::getDeleteObjectRequestMessage(char filter[], Flags flags,
      std::string objClass, long objInst, std::string objName,
      const ObjectValue &objectValue, int scope) throw (CDAPException)
  {
    CDAPMessage cdapMessage;
    cdapMessage.setFilter(filter);
    cdapMessage.setFlags(flags);
    cdapMessage.setObjClass(objClass);
    cdapMessage.setObjInst(objInst);
    cdapMessage.setObjName(objName);
    cdapMessage.setObjValue(objectValue);
    cdapMessage.setOpCode(M_DELETE);
    cdapMessage.setScope(scope);
    CDAPMessageValidator.validate(cdapMessage);
    return &cdapMessage;
  }
  CDAPMessage*
  CDAPMessage::getDeleteObjectResponseMessage(Flags flags, std::string objClass,
      long objInst, std::string objName, int result, std::string resultReason,
      int invokeID) throw (CDAPException)
  {
    CDAPMessage cdapMessage;
    cdapMessage.setFlags(flags);
    cdapMessage.setInvokeID(invokeID);
    cdapMessage.setObjClass(objClass);
    cdapMessage.setObjInst(objInst);
    cdapMessage.setObjName(objName);
    cdapMessage.setOpCode(M_DELETE_R);
    cdapMessage.setResult(result);
    cdapMessage.setResultReason(resultReason);
    CDAPMessageValidator.validate(cdapMessage);
    return &cdapMessage;
  }
  CDAPMessage*
  CDAPMessage::getStartObjectRequestMessage(char filter[], Flags flags,
      std::string objClass, const ObjectValue &objValue, long objInst,
      std::string objName, int scope) throw (CDAPException)
  {
    CDAPMessage cdapMessage;
    cdapMessage.setFilter(filter);
    cdapMessage.setFlags(flags);
    cdapMessage.setObjClass(objClass);
    cdapMessage.setObjInst(objInst);
    cdapMessage.setObjName(objName);
    cdapMessage.setObjValue(objValue);
    cdapMessage.setOpCode(M_START);
    cdapMessage.setScope(scope);
    CDAPMessageValidator.validate(cdapMessage);
    return &cdapMessage;
  }
  CDAPMessage*
  CDAPMessage::getStartObjectResponseMessage(Flags flags, int result,
      std::string resultReason, int invokeID) throw (CDAPException)
  {
    CDAPMessage cdapMessage;
    cdapMessage.setFlags(flags);
    cdapMessage.setInvokeID(invokeID);
    cdapMessage.setOpCode(M_START_R);
    cdapMessage.setResult(result);
    cdapMessage.setResultReason(resultReason);
    CDAPMessageValidator.validate(cdapMessage);
    return &cdapMessage;
  }
  CDAPMessage*
  CDAPMessage::getStartObjectResponseMessage(Flags flags,
      std::string objectClass, const ObjectValue &objectValue, long objInst,
      std::string objName, int result, std::string resultReason, int invokeID)
          throw (CDAPException)
  {
    CDAPMessage cdapMessage;
    cdapMessage.setFlags(flags);
    cdapMessage.setInvokeID(invokeID);
    cdapMessage.setOpCode(M_START_R);
    cdapMessage.setObjClass(objectClass);
    cdapMessage.setObjInst(objInst);
    cdapMessage.setObjName(objName);
    cdapMessage.setObjValue(objectValue);
    cdapMessage.setResult(result);
    cdapMessage.setResultReason(resultReason);
    CDAPMessageValidator.validate(cdapMessage);
    return &cdapMessage;
  }
  CDAPMessage*
  CDAPMessage::getStopObjectRequestMessage(char filter[], Flags flags,
      std::string objClass, const ObjectValue &objValue, long objInst,
      std::string objName, int scope) throw (CDAPException)
  {
    CDAPMessage cdapMessage;
    cdapMessage.setFilter(filter);
    cdapMessage.setFlags(flags);
    cdapMessage.setObjClass(objClass);
    cdapMessage.setObjInst(objInst);
    cdapMessage.setObjName(objName);
    cdapMessage.setObjValue(objValue);
    cdapMessage.setOpCode(M_STOP);
    cdapMessage.setScope(scope);
    CDAPMessageValidator.validate(cdapMessage);
    return &cdapMessage;
  }
  CDAPMessage*
  CDAPMessage::getStopObjectResponseMessage(Flags flags, int result,
      std::string resultReason, int invokeID) throw (CDAPException)
  {
    CDAPMessage cdapMessage;
    cdapMessage.setFlags(flags);
    cdapMessage.setInvokeID(invokeID);
    cdapMessage.setOpCode(M_STOP_R);
    cdapMessage.setResult(result);
    cdapMessage.setResultReason(resultReason);
    CDAPMessageValidator.validate(cdapMessage);
    return &cdapMessage;
  }
  CDAPMessage*
  CDAPMessage::getReadObjectRequestMessage(char filter[], Flags flags,
      std::string objClass, long objInst, std::string objName, int scope)
          throw (CDAPException)
  {
    CDAPMessage cdapMessage;
    cdapMessage.setFilter(filter);
    cdapMessage.setFlags(flags);
    cdapMessage.setObjClass(objClass);
    cdapMessage.setObjInst(objInst);
    cdapMessage.setObjName(objName);
    cdapMessage.setOpCode(M_READ);
    cdapMessage.setScope(scope);
    CDAPMessageValidator.validate(cdapMessage);
    return &cdapMessage;
  }
  CDAPMessage*
  CDAPMessage::getReadObjectResponseMessage(Flags flags, std::string objClass,
      long objInst, std::string objName, const ObjectValue &objValue,
      int result, std::string resultReason, int invokeID) throw (CDAPException)
  {
    CDAPMessage cdapMessage;
    cdapMessage.setFlags(flags);
    cdapMessage.setInvokeID(invokeID);
    cdapMessage.setObjClass(objClass);
    cdapMessage.setObjInst(objInst);
    cdapMessage.setObjName(objName);
    cdapMessage.setObjValue(objValue);
    cdapMessage.setOpCode(M_READ_R);
    cdapMessage.setResult(result);
    cdapMessage.setResultReason(resultReason);
    CDAPMessageValidator.validate(cdapMessage);
    return &cdapMessage;
  }
  CDAPMessage*
  CDAPMessage::getWriteObjectRequestMessage(char filter[], Flags flags,
      std::string objClass, long objInst, const ObjectValue &objValue,
      std::string objName, int scope) throw (CDAPException)
  {
    CDAPMessage cdapMessage;
    cdapMessage.setFilter(filter);
    cdapMessage.setFlags(flags);
    cdapMessage.setObjClass(objClass);
    cdapMessage.setObjInst(objInst);
    cdapMessage.setObjName(objName);
    cdapMessage.setObjValue(objValue);
    cdapMessage.setOpCode(M_WRITE);
    cdapMessage.setScope(scope);
    CDAPMessageValidator.validate(cdapMessage);
    return &cdapMessage;
  }
  CDAPMessage*
  CDAPMessage::getWriteObjectResponseMessage(Flags flags, int result,
      int invokeID, std::string resultReason) throw (CDAPException)
  {
    CDAPMessage cdapMessage;
    cdapMessage.setFlags(flags);
    cdapMessage.setOpCode(M_WRITE_R);
    cdapMessage.setResult(result);
    cdapMessage.setResultReason(resultReason);
    cdapMessage.setInvokeID(invokeID);
    CDAPMessageValidator.validate(cdapMessage);
    return &cdapMessage;
  }
  CDAPMessage*
  CDAPMessage::getCancelReadRequestMessage(Flags flags, int invokeID)
      throw (CDAPException)
  {
    CDAPMessage cdapMessage;
    cdapMessage.setFlags(flags);
    cdapMessage.setInvokeID(invokeID);
    cdapMessage.setOpCode(M_CANCELREAD);
    CDAPMessageValidator.validate(cdapMessage);
    return &cdapMessage;
  }
  CDAPMessage*
  CDAPMessage::getCancelReadResponseMessage(Flags flags, int invokeID,
      int result, std::string resultReason) throw (CDAPException)
  {
    CDAPMessage cdapMessage;
    cdapMessage.setFlags(flags);
    cdapMessage.setInvokeID(invokeID);
    cdapMessage.setOpCode(M_CANCELREAD_R);
    cdapMessage.setResult(result);
    cdapMessage.setResultReason(resultReason);
    CDAPMessageValidator.validate(cdapMessage);
    return &cdapMessage;
  }

 /* TODO: Implement rest of fucntions of this class */

}

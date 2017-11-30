/*
 * libcdap library, implementing CDAP.
 *
 * Copyright (C) 2015-2016 Nextworks
 * Author: Vincenzo Maffione <v.maffione@gmail.com>
 *
 * This file is part of rlite.
 *
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <iostream>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#include "cdap.hpp"
#include "cpputils.hpp"

using namespace std;

#define CDAP_ABS_SYNTAX 73

static const char *opcode_names_table[] = {
    [gpb::M_CONNECT]      = "M_CONNECT",
    [gpb::M_CONNECT_R]    = "M_CONNECT_R",
    [gpb::M_RELEASE]      = "M_RELEASE",
    [gpb::M_RELEASE_R]    = "M_RELEASE_R",
    [gpb::M_CREATE]       = "M_CREATE",
    [gpb::M_CREATE_R]     = "M_CREATE_R",
    [gpb::M_DELETE]       = "M_DELETE",
    [gpb::M_DELETE_R]     = "M_DELETE_R",
    [gpb::M_READ]         = "M_READ",
    [gpb::M_READ_R]       = "M_READ_R",
    [gpb::M_CANCELREAD]   = "M_CANCELREAD",
    [gpb::M_CANCELREAD_R] = "M_CANCELREAD_R",
    [gpb::M_WRITE]        = "M_WRITE",
    [gpb::M_WRITE_R]      = "M_WRITE_R",
    [gpb::M_START]        = "M_START",
    [gpb::M_START_R]      = "M_START_R",
    [gpb::M_STOP]         = "M_STOP",
    [gpb::M_STOP_R]       = "M_STOP_R",
};

#define MAX_CDAP_OPCODE gpb::M_STOP_R
#define MAX_CDAP_FIELD gpb::CDAPMessage::kVersionFieldNumber

#define FLNUM(_FL) gpb::CDAPMessage::k##_FL##FieldNumber

#define ENTRY_FILL(FL, OP, VA)                                                 \
    tab[((MAX_CDAP_OPCODE + 1) * FLNUM(FL) + gpb::OP)] = VA

#define CAN_EXIST 0
#define MUST_EXIST 1
#define MUST_NOT_EXIST 2

#define COMBO(FL, OP) ((MAX_CDAP_OPCODE + 1) * FLNUM(FL) + OP)

#define TAB_FILL(FL, VA)                                                       \
    for (unsigned _i = gpb::M_CONNECT; _i <= MAX_CDAP_OPCODE; _i++)            \
    tab[COMBO(FL, _i)] = VA

struct CDAPValidationTable {
    char tab[(MAX_CDAP_OPCODE + 1) * (1 + MAX_CDAP_FIELD)];

    CDAPValidationTable();

    bool check(int field, const char *flname, gpb::opCode_t op, bool observed);
};

bool
CDAPValidationTable::check(int field, const char *flname, gpb::opCode_t op,
                           bool observed)
{
    char expected = tab[(MAX_CDAP_OPCODE + 1) * field + op];

    if (expected == MUST_EXIST && !observed) {
        fprintf(stderr, "Invalid message: %s must contain field %s\n",
           opcode_names_table[op], flname);
        return false;
    }

    if (expected == MUST_NOT_EXIST && observed) {
        fprintf(stderr, "Invalid message: %s must not contain field %s\n",
           opcode_names_table[op], flname);
        return false;
    }

    return true;
}

CDAPValidationTable::CDAPValidationTable()
{
    /* abs_syntax */
    TAB_FILL(AbsSyntax, MUST_NOT_EXIST);
    ENTRY_FILL(AbsSyntax, M_CONNECT, MUST_EXIST);
    ENTRY_FILL(AbsSyntax, M_CONNECT_R, MUST_EXIST);

    /* auth_mech */
    TAB_FILL(AuthMech, MUST_NOT_EXIST);
    ENTRY_FILL(AuthMech, M_CONNECT, 0);
    ENTRY_FILL(AuthMech, M_CONNECT_R, 0);

    /* auth_value */
    TAB_FILL(AuthValue, MUST_NOT_EXIST);
    ENTRY_FILL(AuthValue, M_CONNECT, 0);
    ENTRY_FILL(AuthValue, M_CONNECT_R, 0);

    /* src_appl */
    TAB_FILL(SrcApName, MUST_NOT_EXIST);
    ENTRY_FILL(SrcApName, M_CONNECT, MUST_EXIST);
    ENTRY_FILL(SrcApName, M_CONNECT_R, MUST_EXIST);

    /* dst_appl */
    TAB_FILL(DestApName, MUST_NOT_EXIST);
    ENTRY_FILL(DestApName, M_CONNECT, MUST_EXIST);
    ENTRY_FILL(DestApName, M_CONNECT_R, MUST_EXIST);

    /* filter */
    ENTRY_FILL(Filter, M_CONNECT, MUST_NOT_EXIST);
    ENTRY_FILL(Filter, M_CONNECT_R, MUST_NOT_EXIST);
    ENTRY_FILL(Filter, M_RELEASE, MUST_NOT_EXIST);
    ENTRY_FILL(Filter, M_RELEASE_R, MUST_NOT_EXIST);
    ENTRY_FILL(Filter, M_CREATE, MUST_NOT_EXIST);
    ENTRY_FILL(Filter, M_CREATE_R, MUST_NOT_EXIST);
    ENTRY_FILL(Filter, M_DELETE_R, MUST_NOT_EXIST);
    ENTRY_FILL(Filter, M_READ_R, MUST_NOT_EXIST);
    ENTRY_FILL(Filter, M_CANCELREAD, MUST_NOT_EXIST);
    ENTRY_FILL(Filter, M_CANCELREAD_R, MUST_NOT_EXIST);
    ENTRY_FILL(Filter, M_WRITE_R, MUST_NOT_EXIST);
    ENTRY_FILL(Filter, M_START_R, MUST_NOT_EXIST);
    ENTRY_FILL(Filter, M_STOP_R, MUST_NOT_EXIST);

    /* invoke_id */
    ENTRY_FILL(InvokeID, M_CONNECT, MUST_EXIST);
    ENTRY_FILL(InvokeID, M_CONNECT_R, MUST_EXIST);
    ENTRY_FILL(InvokeID, M_RELEASE_R, MUST_EXIST);
    ENTRY_FILL(InvokeID, M_CREATE_R, MUST_EXIST);
    ENTRY_FILL(InvokeID, M_DELETE_R, MUST_EXIST);
    ENTRY_FILL(InvokeID, M_READ_R, MUST_EXIST);
    ENTRY_FILL(InvokeID, M_CANCELREAD, MUST_EXIST);
    ENTRY_FILL(InvokeID, M_CANCELREAD_R, MUST_EXIST);
    ENTRY_FILL(InvokeID, M_WRITE_R, MUST_EXIST);
    ENTRY_FILL(InvokeID, M_START_R, MUST_EXIST);
    ENTRY_FILL(InvokeID, M_STOP_R, MUST_EXIST);

    /* obj_class */
    ENTRY_FILL(ObjClass, M_CONNECT, MUST_NOT_EXIST);
    ENTRY_FILL(ObjClass, M_CONNECT_R, MUST_NOT_EXIST);
    ENTRY_FILL(ObjClass, M_RELEASE, MUST_NOT_EXIST);
    ENTRY_FILL(ObjClass, M_RELEASE_R, MUST_NOT_EXIST);
    ENTRY_FILL(ObjClass, M_CANCELREAD, MUST_NOT_EXIST);
    ENTRY_FILL(ObjClass, M_CANCELREAD_R, MUST_NOT_EXIST);

    /* obj_inst */
    ENTRY_FILL(ObjInst, M_CONNECT, MUST_NOT_EXIST);
    ENTRY_FILL(ObjInst, M_CONNECT_R, MUST_NOT_EXIST);
    ENTRY_FILL(ObjInst, M_RELEASE, MUST_NOT_EXIST);
    ENTRY_FILL(ObjInst, M_RELEASE_R, MUST_NOT_EXIST);
    ENTRY_FILL(ObjInst, M_CANCELREAD, MUST_NOT_EXIST);
    ENTRY_FILL(ObjInst, M_CANCELREAD_R, MUST_NOT_EXIST);

    /* obj_name */
    ENTRY_FILL(ObjName, M_CONNECT, MUST_NOT_EXIST);
    ENTRY_FILL(ObjName, M_CONNECT_R, MUST_NOT_EXIST);
    ENTRY_FILL(ObjName, M_RELEASE, MUST_NOT_EXIST);
    ENTRY_FILL(ObjName, M_RELEASE_R, MUST_NOT_EXIST);
    ENTRY_FILL(ObjName, M_CANCELREAD, MUST_NOT_EXIST);
    ENTRY_FILL(ObjName, M_CANCELREAD_R, MUST_NOT_EXIST);

    /* obj_value */
    ENTRY_FILL(ObjValue, M_CONNECT, MUST_NOT_EXIST);
    ENTRY_FILL(ObjValue, M_CONNECT_R, MUST_NOT_EXIST);
    ENTRY_FILL(ObjValue, M_RELEASE, MUST_NOT_EXIST);
    ENTRY_FILL(ObjValue, M_RELEASE_R, MUST_NOT_EXIST);
    ENTRY_FILL(ObjValue, M_WRITE, MUST_EXIST);
    ENTRY_FILL(ObjValue, M_DELETE_R, MUST_NOT_EXIST);
    ENTRY_FILL(ObjValue, M_CANCELREAD, MUST_NOT_EXIST);
    ENTRY_FILL(ObjValue, M_CANCELREAD_R, MUST_NOT_EXIST);

    /* op_code */
    TAB_FILL(OpCode, MUST_EXIST);

    /* result */
    ENTRY_FILL(Result, M_CONNECT, MUST_NOT_EXIST);
    ENTRY_FILL(Result, M_RELEASE, MUST_NOT_EXIST);
    ENTRY_FILL(Result, M_CREATE, MUST_NOT_EXIST);
    ENTRY_FILL(Result, M_DELETE, MUST_NOT_EXIST);
    ENTRY_FILL(Result, M_READ, MUST_NOT_EXIST);
    ENTRY_FILL(Result, M_WRITE, MUST_NOT_EXIST);
    ENTRY_FILL(Result, M_START, MUST_NOT_EXIST);
    ENTRY_FILL(Result, M_STOP, MUST_NOT_EXIST);

    /* result_reason */
    ENTRY_FILL(ResultReason, M_CONNECT, MUST_NOT_EXIST);
    ENTRY_FILL(ResultReason, M_RELEASE, MUST_NOT_EXIST);
    ENTRY_FILL(ResultReason, M_CREATE, MUST_NOT_EXIST);
    ENTRY_FILL(ResultReason, M_DELETE, MUST_NOT_EXIST);
    ENTRY_FILL(ResultReason, M_READ, MUST_NOT_EXIST);
    ENTRY_FILL(ResultReason, M_WRITE, MUST_NOT_EXIST);
    ENTRY_FILL(ResultReason, M_START, MUST_NOT_EXIST);
    ENTRY_FILL(ResultReason, M_STOP, MUST_NOT_EXIST);

    /* scope */
    TAB_FILL(Scope, MUST_NOT_EXIST);
    ENTRY_FILL(Scope, M_CREATE, 0);
    ENTRY_FILL(Scope, M_DELETE, 0);
    ENTRY_FILL(Scope, M_READ, 0);
    ENTRY_FILL(Scope, M_WRITE, 0);
    ENTRY_FILL(Scope, M_START, 0);
    ENTRY_FILL(Scope, M_STOP, 0);

    /* version */
    ENTRY_FILL(Version, M_CONNECT, MUST_EXIST);
    ENTRY_FILL(Version, M_CONNECT_R, MUST_EXIST);
}

static struct CDAPValidationTable vt;

CDAPConn::CDAPConn(int arg_fd, long arg_version, unsigned int ds)
    : invoke_id_mgr(ds)
{
    discard_secs = ds;
    fd           = arg_fd;
    version      = arg_version;
    state        = ConnState::NONE;
}

CDAPConn::CDAPConn(const CDAPConn &o) { assert(0); }

CDAPConn::~CDAPConn() {}

void
CDAPConn::reset()
{
    state         = ConnState::NONE;
    local_appl    = string();
    remote_appl   = string();
    invoke_id_mgr = InvokeIdMgr(discard_secs);
    fprintf(stdout, "Connection reset to %s\n", conn_state_repr(state));
}

int
CDAPConn::connect(const std::string &src, const std::string &dst,
                  gpb::authTypes_t auth_mech,
                  const struct CDAPAuthValue *auth_value)
{
    CDAPMessage *rm;
    CDAPMessage m;

    m.m_connect(auth_mech, auth_value, src, dst);
    if (msg_send(&m, 0) < 0) {
        return -1;
    }

    rm = msg_recv();
    if (rm->op_code != gpb::M_CONNECT_R) {
        return -1;
    }
    delete rm;

    return 0;
}

CDAPMessage *
CDAPConn::accept()
{
    CDAPMessage *rm = msg_recv();
    CDAPMessage m;

    if (rm->op_code != gpb::M_CONNECT) {
        delete rm;
        return NULL;
    }

    m.m_connect_r(rm, 0, string());
    if (msg_send(&m, rm->invoke_id) < 0) {
        delete rm;
        return NULL;
    }

    return rm;
}

const char *
CDAPConn::conn_state_repr(ConnState st)
{
    switch (st) {
    case ConnState::NONE:
        return "NONE";

    case ConnState::AWAITCON:
        return "AWAITCON";

    case ConnState::CONNECTED:
        return "CONNECTED";

    case ConnState::AWAITCLOSE:
        return "AWAITCLOSE";
    }

    assert(0);
    return NULL;
}

InvokeIdMgr::InvokeIdMgr(unsigned int ds)
{
    discard_secs   = ds;
    invoke_id_next = 1;
}

/* Discard pending ids that have been there for too much time. */
void
InvokeIdMgr::__discard(unordered_set<Id, IdHasher> &pending)
{
    vector<unordered_set<Id, IdHasher>::iterator> torm;
    time_t now = time(NULL);

    for (auto i = pending.begin(); i != pending.end(); i++) {
        if (now - i->created > discard_secs) {
            torm.push_back(i);
        }
    }

    for (unsigned int j = 0; j < torm.size(); j++) {
        pending.erase(torm[j]);
    }
}

void
InvokeIdMgr::discard()
{
    if (discard_secs < ~0U) {
        __discard(pending_invoke_ids);
        __discard(pending_invoke_ids_remote);
    }
}

int
InvokeIdMgr::__put_invoke_id(unordered_set<Id, IdHasher> &pending,
                             int invoke_id)
{
    discard();

    if (!pending.count(Id(invoke_id, 0))) {
        return -1;
    }

    pending.erase(Id(invoke_id, 0));

    return 0;
}

int
InvokeIdMgr::get_invoke_id()
{
    discard();

    while (pending_invoke_ids.count(Id(invoke_id_next, 0))) {
        invoke_id_next++;
    }

    pending_invoke_ids.insert(Id(invoke_id_next, time(NULL)));

    return invoke_id_next;
}

int
InvokeIdMgr::put_invoke_id(int invoke_id)
{
    return __put_invoke_id(pending_invoke_ids, invoke_id);
}

int
InvokeIdMgr::get_invoke_id_remote(int invoke_id)
{
    discard();

    if (pending_invoke_ids_remote.count(Id(invoke_id, 0))) {
        return -1;
    }

    pending_invoke_ids_remote.insert(Id(invoke_id, time(NULL)));

    return 0;
}

int
InvokeIdMgr::put_invoke_id_remote(int invoke_id)
{
    return __put_invoke_id(pending_invoke_ids_remote, invoke_id);
}

CDAPMessage::CDAPMessage()
{
    abs_syntax   = 0;
    auth_mech    = gpb::AUTH_NONE;
    flags        = gpb::F_NO_FLAGS;
    invoke_id    = 0;
    obj_inst     = 0;
    op_code      = gpb::M_CONNECT;
    obj_value.ty = ObjValType::NONE;
    result       = 0;
    scope        = 0;
    version      = 0;
}

void
CDAPMessage::copy(const CDAPMessage &o)
{
    abs_syntax    = o.abs_syntax;
    auth_mech     = o.auth_mech;
    auth_value    = o.auth_value;
    src_appl      = o.src_appl;
    dst_appl      = o.dst_appl;
    filter        = o.filter;
    flags         = o.flags;
    invoke_id     = o.invoke_id;
    obj_class     = o.obj_class;
    obj_inst      = o.obj_inst;
    obj_name      = o.obj_name;
    op_code       = o.op_code;
    result        = o.result;
    result_reason = o.result_reason;
    scope         = o.scope;
    version       = o.version;

    switch (o.obj_value.ty) {
    case ObjValType::BYTES:
        obj_value.u.buf.owned = true;
        obj_value.u.buf.len   = o.obj_value.u.buf.len;
        obj_value.u.buf.ptr   = new char[obj_value.u.buf.len];
        memcpy(obj_value.u.buf.ptr, o.obj_value.u.buf.ptr, obj_value.u.buf.len);
        break;

    case ObjValType::NONE:
    case ObjValType::I32:
    case ObjValType::I64:
    case ObjValType::FLOAT:
    case ObjValType::DOUBLE:
    case ObjValType::BOOL:
    case ObjValType::STRING:
        obj_value = o.obj_value;
        break;
    }
}

void
CDAPMessage::destroy()
{
    src_appl.clear();
    dst_appl.clear();
    if (obj_value.ty == ObjValType::BYTES && obj_value.u.buf.owned &&
        obj_value.u.buf.ptr) {
        delete[] obj_value.u.buf.ptr;
    }
}

CDAPMessage::CDAPMessage(const CDAPMessage &o) { copy(o); }

CDAPMessage::~CDAPMessage() { destroy(); }

CDAPMessage &
CDAPMessage::operator=(const CDAPMessage &o)
{
    if (&o == this) {
        return *this;
    }

    destroy();
    copy(o);

    return *this;
}

bool
CDAPMessage::is_type(ObjValType tt) const
{
    return obj_value.ty == tt;
}

void
CDAPMessage::get_obj_value(int32_t &v) const
{
    v = 0;
    if (obj_value.ty == ObjValType::I32) {
        v = obj_value.u.i32;
    }
}

void
CDAPMessage::set_obj_value(int32_t v)
{
    obj_value.ty    = ObjValType::I32;
    obj_value.u.i32 = v;
}

void
CDAPMessage::get_obj_value(int64_t &v) const
{
    v = 0;
    if (obj_value.ty == ObjValType::I64) {
        v = obj_value.u.i64;
    }
}

void
CDAPMessage::set_obj_value(int64_t v)
{
    obj_value.ty    = ObjValType::I64;
    obj_value.u.i64 = v;
}

void
CDAPMessage::get_obj_value(float &v) const
{
    v = 0.0;
    if (obj_value.ty == ObjValType::FLOAT) {
        v = obj_value.u.fp_single;
    }
}

void
CDAPMessage::set_obj_value(float v)
{
    obj_value.ty          = ObjValType::FLOAT;
    obj_value.u.fp_single = v;
}

void
CDAPMessage::get_obj_value(double &v) const
{
    v = 0.0;
    if (obj_value.ty == ObjValType::DOUBLE) {
        v = obj_value.u.fp_double;
    }
}

void
CDAPMessage::set_obj_value(double v)
{
    obj_value.ty          = ObjValType::DOUBLE;
    obj_value.u.fp_double = v;
}

void
CDAPMessage::get_obj_value(bool &v) const
{
    v = false;
    if (obj_value.ty == ObjValType::BOOL) {
        v = obj_value.u.boolean;
    }
}

void
CDAPMessage::set_obj_value(bool v)
{
    obj_value.ty        = ObjValType::BOOL;
    obj_value.u.boolean = v;
}

void
CDAPMessage::get_obj_value(std::string &v) const
{
    v = std::string();
    if (obj_value.ty == ObjValType::STRING) {
        v = obj_value.str;
    }
}

void
CDAPMessage::set_obj_value(const std::string &v)
{
    obj_value.ty  = ObjValType::STRING;
    obj_value.str = v;
}

void
CDAPMessage::set_obj_value(const char *v)
{
    obj_value.ty  = ObjValType::STRING;
    obj_value.str = std::string(v);
}

void
CDAPMessage::get_obj_value(const char *&p, size_t &l) const
{
    p = NULL;
    l = 0;
    if (obj_value.ty == ObjValType::BYTES) {
        p = obj_value.u.buf.ptr;
        l = obj_value.u.buf.len;
    }
}

void
CDAPMessage::set_obj_value(const char *buf, size_t len)
{
    obj_value.ty          = ObjValType::BYTES;
    obj_value.u.buf.ptr   = const_cast<char *>(buf);
    obj_value.u.buf.len   = len;
    obj_value.u.buf.owned = false;
}

CDAPMessage::CDAPMessage(const gpb::CDAPMessage &gm)
{
    string apn, api, aen, aei;
    gpb::objVal_t objvalue = gm.objvalue();

    abs_syntax = gm.abssyntax();
    op_code    = gm.opcode();
    invoke_id  = gm.invokeid();
    flags      = gm.flags();
    obj_class  = gm.objclass();
    obj_name   = gm.objname();
    obj_inst   = gm.objinst();

    /* Convert object value. */
    if (objvalue.has_intval()) {
        obj_value.u.i32 = objvalue.intval();
        obj_value.ty    = ObjValType::I32;

    } else if (objvalue.has_sintval()) {
        obj_value.u.i32 = objvalue.sintval();
        obj_value.ty    = ObjValType::I32;

    } else if (objvalue.has_int64val()) {
        obj_value.u.i64 = objvalue.int64val();
        obj_value.ty    = ObjValType::I64;

    } else if (objvalue.has_sint64val()) {
        obj_value.u.i64 = objvalue.sint64val();
        obj_value.ty    = ObjValType::I64;

    } else if (objvalue.has_strval()) {
        obj_value.str = objvalue.strval();
        obj_value.ty  = ObjValType::STRING;

    } else if (objvalue.has_floatval()) {
        obj_value.u.fp_single = objvalue.floatval();
        obj_value.ty          = ObjValType::FLOAT;

    } else if (objvalue.has_doubleval()) {
        obj_value.u.fp_double = objvalue.doubleval();
        obj_value.ty          = ObjValType::DOUBLE;

    } else if (objvalue.has_boolval()) {
        obj_value.u.boolean = objvalue.boolval();
        obj_value.ty        = ObjValType::BOOL;

    } else if (objvalue.has_byteval()) {
        try {
            obj_value.u.buf.ptr = new char[objvalue.byteval().size()];
            memcpy(obj_value.u.buf.ptr, objvalue.byteval().data(),
                   objvalue.byteval().size());
            obj_value.u.buf.len   = objvalue.byteval().size();
            obj_value.u.buf.owned = true;
            obj_value.ty          = ObjValType::BYTES;

        } catch (std::bad_alloc) {
            fprintf(stderr, "BYTES object allocation failed\n");
            obj_value.ty = ObjValType::NONE;
        }

    } else {
        obj_value.ty = ObjValType::NONE;
    }

    result = gm.result();
    scope  = gm.scope();
    filter = gm.filter();

    auth_mech           = gm.authmech();
    auth_value.name     = gm.authvalue().authname();
    auth_value.password = gm.authvalue().authpassword();
    auth_value.other    = gm.authvalue().authother();

    apn      = gm.has_destapname() ? gm.destapname() : string();
    api      = gm.has_destapinst() ? gm.destapinst() : string();
    aen      = gm.has_destaename() ? gm.destaename() : string();
    aei      = gm.has_destaeinst() ? gm.destaeinst() : string();
    dst_appl = rina_string_from_components(apn, api, aen, aei);

    apn      = gm.has_srcapname() ? gm.srcapname() : string();
    api      = gm.has_srcapinst() ? gm.srcapinst() : string();
    aen      = gm.has_srcaename() ? gm.srcaename() : string();
    aei      = gm.has_srcaeinst() ? gm.srcaeinst() : string();
    src_appl = rina_string_from_components(apn, api, aen, aei);

    result_reason = gm.resultreason();
    version       = gm.version();
}

#define safe_c_string(_s) ((_s) ? (_s) : "")

CDAPMessage::operator gpb::CDAPMessage() const
{
    gpb::CDAPMessage gm;
    gpb::objVal_t *objvalue     = new gpb::objVal_t();
    gpb::authValue_t *authvalue = new gpb::authValue_t();
    string apn, api, aen, aei;

    gm.set_abssyntax(abs_syntax);
    gm.set_opcode(op_code);
    gm.set_invokeid(invoke_id);
    gm.set_flags(flags);
    gm.set_objclass(obj_class);
    gm.set_objname(obj_name);
    gm.set_objinst(obj_inst);

    /* Convert object value. */
    switch (obj_value.ty) {
    case ObjValType::I32:
        objvalue->set_intval(obj_value.u.i32);
        break;

    case ObjValType::I64:
        objvalue->set_int64val(obj_value.u.i64);
        break;

    case ObjValType::STRING:
        objvalue->set_strval(obj_value.str);
        break;

    case ObjValType::FLOAT:
        objvalue->set_floatval(obj_value.u.fp_single);
        break;

    case ObjValType::DOUBLE:
        objvalue->set_doubleval(obj_value.u.fp_double);
        break;

    case ObjValType::BOOL:
        objvalue->set_boolval(obj_value.u.boolean);
        break;

    case ObjValType::BYTES:
        objvalue->set_byteval(obj_value.u.buf.ptr, obj_value.u.buf.len);
        break;

    default:
        break;
    }

    if (obj_value.ty != ObjValType::NONE) {
        gm.set_allocated_objvalue(objvalue);
    } else {
        delete objvalue;
    }

    gm.set_result(result);
    gm.set_scope(scope);
    gm.set_filter(filter);
    gm.set_authmech(auth_mech);

    authvalue->set_authname(auth_value.name);
    authvalue->set_authpassword(auth_value.password);
    authvalue->set_authother(auth_value.other);
    gm.set_allocated_authvalue(authvalue);

    rina_components_from_string(dst_appl, apn, api, aen, aei);
    gm.set_destapname(apn);
    gm.set_destapinst(api);
    gm.set_destaename(aen);
    gm.set_destaeinst(aei);

    rina_components_from_string(src_appl, apn, api, aen, aei);
    gm.set_srcapname(apn);
    gm.set_srcapinst(api);
    gm.set_srcaename(aen);
    gm.set_srcaeinst(aei);

    gm.set_resultreason(result_reason);
    gm.set_version(version);

    return gm;
}

static int
rina_sername_valid(const char *str)
{
    const char *orig_str = str;
    int cnt              = 0;

    if (!str || strlen(str) == 0) {
        return 0;
    }

    while (*str != '\0') {
        if (*str == ':') {
            if (++cnt > 3) {
                return 0;
            }
        }
        str++;
    }

    return (*orig_str == ':') ? 0 : 1;
}

bool
CDAPMessage::valid(bool check_invoke_id) const
{
    bool ret = true;

    ret = ret &&
          vt.check(FLNUM(AbsSyntax), "abs_syntax", op_code, abs_syntax != 0);

    ret = ret && vt.check(FLNUM(AuthMech), "auth_mech", op_code,
                          auth_mech != gpb::AUTH_NONE);

    ret = ret && vt.check(FLNUM(AuthValue), "auth_value", op_code,
                          !auth_value.empty());

    ret = ret && vt.check(FLNUM(SrcApName), "src_appl", op_code,
                          rina_sername_valid(src_appl.c_str()));

    ret = ret && vt.check(FLNUM(DestApName), "dst_appl", op_code,
                          rina_sername_valid(dst_appl.c_str()));

    ret = ret && vt.check(FLNUM(Filter), "filter", op_code, filter != string());

    if (check_invoke_id) {
        ret = ret &&
              vt.check(FLNUM(InvokeID), "invoke_id", op_code, invoke_id != 0);
    }

    ret = ret && vt.check(FLNUM(ObjClass), "obj_class", op_code,
                          obj_class != string());

    ret = ret && vt.check(FLNUM(ObjInst), "obj_inst", op_code, obj_inst != 0);

    ret = ret &&
          vt.check(FLNUM(ObjName), "obj_name", op_code, obj_name != string());

    ret = ret && vt.check(FLNUM(ObjValue), "obj_value", op_code,
                          obj_value.ty != ObjValType::NONE);

    ret = ret && vt.check(FLNUM(Result), "result", op_code, result != 0);

    ret = ret && vt.check(FLNUM(ResultReason), "result_reason", op_code,
                          result_reason != string());

    ret = ret && vt.check(FLNUM(Scope), "scope", op_code, scope != 0);

    ret = ret && vt.check(FLNUM(Version), "version", op_code, version != 0);

    if ((obj_class != string()) != (obj_name != string())) {
        fprintf(stderr, "Invalid message: if obj_class is specified, also obj_name "
           "must be, and the other way around\n");
        return false;
    }

    return ret;
}

void
CDAPMessage::dump() const
{
    fprintf(stdout, "CDAP Message { ");
    fprintf(stdout, "abs_syntax: %d, ", abs_syntax);
    if (op_code <= MAX_CDAP_OPCODE) {
        fprintf(stdout, "op_code: %s, ", opcode_names_table[op_code]);
    }
    fprintf(stdout, "invoke_id: %d, ", invoke_id);

    if (flags != gpb::F_NO_FLAGS) {
        fprintf(stdout, "flags: %04x, ", flags);
    }

    if (obj_class != string()) {
        fprintf(stdout, "obj_class: %s, ", obj_class.c_str());
    }

    if (obj_name != string()) {
        fprintf(stdout, "obj_name: %s, ", obj_name.c_str());
    }

    if (obj_inst) {
        fprintf(stdout, "obj_inst: %ld, ", obj_inst);
    }

    /* Print object value. */
    switch (obj_value.ty) {
    case ObjValType::I32:
        fprintf(stdout, "obj_value: %d, ", obj_value.u.i32);
        break;

    case ObjValType::I64:
        fprintf(stdout, "obj_value: %lld, ", (long long)obj_value.u.i64);
        break;

    case ObjValType::STRING:
        fprintf(stdout, "obj_value: %s, ", obj_value.str.c_str());
        break;

    case ObjValType::FLOAT:
        fprintf(stdout, "obj_value: %f,", obj_value.u.fp_single);
        break;

    case ObjValType::DOUBLE:
        fprintf(stdout, "obj_value: %f, ", obj_value.u.fp_double);
        break;

    case ObjValType::BOOL:
        fprintf(stdout, "obj_value: %s, ", (obj_value.u.boolean ? "true" : "false"));
        break;

    case ObjValType::BYTES:
        fprintf(stdout, "obj_value: %d bytes at %p, ", (int)obj_value.u.buf.len,
             obj_value.u.buf.ptr);
        break;

    default:
        break;
    }

    if (result) {
        fprintf(stdout, "result: %d, ", result);
    }

    if (scope) {
        fprintf(stdout, "scope: %d, ", scope);
    }

    if (filter != string()) {
        fprintf(stdout, "filter: %s, ", filter.c_str());
    }

    if (auth_mech != gpb::AUTH_NONE) {
        fprintf(stdout, "auth_mech: %d, ", auth_mech);
        fprintf(stdout, "auth_value: name='%s' pwd='%s' other='%s', ",
             auth_value.name.c_str(), auth_value.password.c_str(),
             auth_value.other.c_str());
    }

    fprintf(stdout, "dst_appl: %s, ", dst_appl.c_str());

    fprintf(stdout, "src_appl: %s, ", src_appl.c_str());

    if (result_reason != string()) {
        fprintf(stdout, "result_reason: %s, ", result_reason.c_str());
    }

    fprintf(stdout, "version: %ld, ", version);

    fprintf(stdout, "}\n");
}

int
CDAPConn::conn_fsm_run(struct CDAPMessage *m, bool sender)
{
    const char *action  = sender ? "send" : "receive";
    ConnState old_state = state;

    if (m->op_code > MAX_CDAP_OPCODE) {
        fprintf(stderr, "Invalid opcode %d\n", m->op_code);
        return -1;
    }

    switch (m->op_code) {
    case gpb::M_CONNECT: {
        std::string local, remote;

        if (state != ConnState::NONE) {
            fprintf(stderr, "Cannot %s M_CONNECT message: Invalid state %s\n", action,
               conn_state_repr(state));
            return -1;
        }

        if (sender) {
            local  = m->src_appl;
            remote = m->dst_appl;
        } else {
            local  = m->dst_appl;
            remote = m->src_appl;
        }

        local_appl  = local;
        remote_appl = remote;

        state = ConnState::AWAITCON;
    } break;

    case gpb::M_CONNECT_R:
        if (state != ConnState::AWAITCON) {
            fprintf(stderr, "Cannot %s M_CONNECT_R message: Invalid state %s\n", action,
               conn_state_repr(state));
            return -1;
        }
        state = ConnState::CONNECTED;
        break;

    case gpb::M_RELEASE:
        if (state != ConnState::CONNECTED) {
            fprintf(stderr, "Cannot %s M_RELEASE message: Invalid state %s\n", action,
               conn_state_repr(state));
            return -1;
        }
        local_appl  = string();
        remote_appl = string();
        state       = ConnState::AWAITCLOSE;
        break;

    case gpb::M_RELEASE_R:
        if (state != ConnState::AWAITCLOSE) {
            fprintf(stderr, "Cannot %s M_RELEASE message: Invalid state %s\n", action,
               conn_state_repr(state));
            return -1;
        }
        state = ConnState::NONE;
        break;

    default:
        /* All the operational messages. */
        if (state != ConnState::CONNECTED) {
            fprintf(stderr, "Cannot %s %s message: Invalid state %s\n", action,
               opcode_names_table[m->op_code], conn_state_repr(state));
            return -1;
        }
    }

    if (old_state != state) {
        fprintf(stdout, "Connection state %s --> %s\n", conn_state_repr(old_state),
           conn_state_repr(state));
    }

    return 0;
}

int
msg_ser_stateless(struct CDAPMessage *m, char **buf, size_t *len)
{
    gpb::CDAPMessage gm;

    *buf = NULL;
    *len = 0;

    gm = static_cast<gpb::CDAPMessage>(*m);

    *len = gm.ByteSize();
    *buf = new char[*len];

    gm.SerializeToArray(*buf, *len);

    return 0;
}

int
CDAPConn::msg_ser(struct CDAPMessage *m, int invoke_id, char **buf, size_t *len)
{
    *buf = NULL;
    *len = 0;

    m->version = version;

    if (!m->valid(false)) {
        return -1;
    }

    /* Run CDAP connection state machine (sender side). */
    if (conn_fsm_run(m, true)) {
        return -1;
    }

    if (invoke_id == 0 && m->is_request()) {
        /* CDAP request message (M_*). */
        m->invoke_id = invoke_id_mgr.get_invoke_id();

    } else {
        /* CDAP response message (M_*_R). */
        m->invoke_id = invoke_id;
        if (invoke_id_mgr.put_invoke_id_remote(m->invoke_id)) {
            fprintf(stderr, "Invoke id %d does not match any pending request\n",
               m->invoke_id);
        }
    }

    return msg_ser_stateless(m, buf, len);
}

int
CDAPConn::msg_send(struct CDAPMessage *m, int invoke_id)
{
    size_t serlen;
    char *serbuf;
    ssize_t n;

    n = msg_ser(m, invoke_id, &serbuf, &serlen);
    if (n) {
        return 0;
    }

    n = write(fd, serbuf, serlen);
    if (n != (ssize_t)serlen) {
        if (n < 0) {
            perror("write(cdap_msg)");
        } else {
            fprintf(stderr, "Partial write %d/%d\n", (int)n, (int)serlen);
        }
        return -1;
    }

    delete serbuf;

    return n;
}

struct CDAPMessage *
msg_deser_stateless(const char *serbuf, size_t serlen)
{
    struct CDAPMessage *m;
    gpb::CDAPMessage gm;

    gm.ParseFromArray(serbuf, serlen);

    m = new CDAPMessage(gm);

    if (!m->valid(true)) {
        delete m;
        return NULL;
    }

    return m;
}

struct CDAPMessage *
CDAPConn::msg_deser(const char *serbuf, size_t serlen)
{
    struct CDAPMessage *m = msg_deser_stateless(serbuf, serlen);

    if (!m) {
        return NULL;
    }

    /* Run CDAP connection state machine (receiver side). */
    if (conn_fsm_run(m, false)) {
        delete m;
        return NULL;
    }

    if (m->is_response()) {
        /* CDAP response message (M_*_R). */
        if (invoke_id_mgr.put_invoke_id(m->invoke_id)) {
            fprintf(stderr, "Invoke id %d does not match any pending request\n",
               m->invoke_id);
            delete m;
            m = NULL;
        }

    } else {
        /* CDAP request message (M_*). */
        if (invoke_id_mgr.get_invoke_id_remote(m->invoke_id)) {
            fprintf(stderr, "Invoke id %d already used remotely\n", m->invoke_id);
            delete m;
            m = NULL;
        }
    }

    return m;
}

struct CDAPMessage *
CDAPConn::msg_recv()
{
    char serbuf[4096];
    int n;

    n = read(fd, serbuf, sizeof(serbuf));
    if (n < 0) {
        perror("read(cdap_msg)");
        return NULL;
    }

    return msg_deser(serbuf, n);
}

int
CDAPMessage::m_connect(gpb::authTypes_t auth_mech_,
                       const struct CDAPAuthValue *auth_value_,
                       const std::string &local_appl,
                       const std::string &remote_appl)
{
    clear();
    op_code    = gpb::M_CONNECT;
    abs_syntax = CDAP_ABS_SYNTAX;
    auth_mech  = auth_mech_;
    auth_value = auth_value_ ? *auth_value_ : CDAPAuthValue();
    src_appl   = local_appl;
    dst_appl   = remote_appl;

    return 0;
}

int
CDAPMessage::m_connect_r(const struct CDAPMessage *req, int result_,
                         const std::string &result_reason_)
{
    clear();
    op_code    = gpb::M_CONNECT_R;
    abs_syntax = CDAP_ABS_SYNTAX;
    auth_mech  = req->auth_mech;
    auth_value = req->auth_value;
    src_appl   = req->dst_appl;
    dst_appl   = req->src_appl;

    result        = result_;
    result_reason = result_reason_;

    return 0;
}

int
CDAPMessage::m_release(gpb::flagValues_t flags_)
{
    clear();
    op_code = gpb::M_RELEASE;
    flags   = flags_;

    return 0;
}

int
CDAPMessage::m_release_r(gpb::flagValues_t flags_, int result_,
                         const std::string &result_reason_)
{
    clear();
    op_code = gpb::M_RELEASE_R;
    flags   = flags_;

    result        = result_;
    result_reason = result_reason_;

    return 0;
}

int
CDAPMessage::m_common(gpb::flagValues_t flags_, const std::string &obj_class_,
                      const std::string &obj_name_, long obj_inst_, int scope_,
                      const std::string &filter_, gpb::opCode_t op_code_)
{
    clear();
    op_code   = op_code_;
    flags     = flags_;
    obj_class = obj_class_;
    obj_name  = obj_name_;
    obj_inst  = obj_inst_;
    scope     = scope_;
    filter    = filter_;

    return 0;
}

int
CDAPMessage::m_common_r(gpb::flagValues_t flags_, const std::string &obj_class_,
                        const std::string &obj_name_, long obj_inst_,
                        int result_, const std::string &result_reason_,
                        gpb::opCode_t op_code_)
{
    clear();
    op_code   = op_code_;
    flags     = flags_;
    obj_class = obj_class_;
    obj_name  = obj_name_;
    obj_inst  = obj_inst_;

    result        = result_;
    result_reason = result_reason_;

    return 0;
}

int
CDAPMessage::m_create(gpb::flagValues_t flags, const std::string &obj_class,
                      const std::string &obj_name, long obj_inst, int scope,
                      const std::string &filter)
{
    return m_common(flags, obj_class, obj_name, obj_inst, scope, filter,
                    gpb::M_CREATE);
}

int
CDAPMessage::m_create_r(gpb::flagValues_t flags, const std::string &obj_class,
                        const std::string &obj_name, long obj_inst, int result,
                        const std::string &result_reason)
{
    return m_common_r(flags, obj_class, obj_name, obj_inst, result,
                      result_reason, gpb::M_CREATE_R);
}

int
CDAPMessage::m_delete(gpb::flagValues_t flags, const std::string &obj_class,
                      const std::string &obj_name, long obj_inst, int scope,
                      const std::string &filter)
{
    return m_common(flags, obj_class, obj_name, obj_inst, scope, filter,
                    gpb::M_DELETE);
}

int
CDAPMessage::m_delete_r(gpb::flagValues_t flags, const std::string &obj_class,
                        const std::string &obj_name, long obj_inst, int result,
                        const std::string &result_reason)
{
    return m_common_r(flags, obj_class, obj_name, obj_inst, result,
                      result_reason, gpb::M_DELETE_R);
}

int
CDAPMessage::m_read(gpb::flagValues_t flags, const std::string &obj_class,
                    const std::string &obj_name, long obj_inst, int scope,
                    const std::string &filter)
{
    return m_common(flags, obj_class, obj_name, obj_inst, scope, filter,
                    gpb::M_READ);
}

int
CDAPMessage::m_read_r(gpb::flagValues_t flags, const std::string &obj_class,
                      const std::string &obj_name, long obj_inst, int result,
                      const std::string &result_reason)
{
    return m_common_r(flags, obj_class, obj_name, obj_inst, result,
                      result_reason, gpb::M_READ_R);
}

int
CDAPMessage::m_write(gpb::flagValues_t flags_, const std::string &obj_class_,
                     const std::string &obj_name_, long obj_inst_, int scope_,
                     const std::string &filter_)
{
    clear();
    op_code   = gpb::M_WRITE;
    flags     = flags_;
    obj_class = obj_class_;
    obj_name  = obj_name_;
    obj_inst  = obj_inst_;
    scope     = scope_;
    filter    = filter_;

    return 0;
}

int
CDAPMessage::m_write_r(gpb::flagValues_t flags_, int result_,
                       const std::string &result_reason_)
{
    clear();
    op_code = gpb::M_WRITE_R;
    flags   = flags_;

    result        = result_;
    result_reason = result_reason_;

    return 0;
}

int
CDAPMessage::m_cancelread(gpb::flagValues_t flags_)
{
    clear();
    op_code = gpb::M_CANCELREAD;
    flags   = flags_;

    return 0;
}

int
CDAPMessage::m_cancelread_r(gpb::flagValues_t flags_, int result_,
                            const std::string &result_reason_)
{
    clear();
    op_code = gpb::M_CANCELREAD_R;
    flags   = flags_;

    result        = result_;
    result_reason = result_reason_;

    return 0;
}

int
CDAPMessage::m_start(gpb::flagValues_t flags, const std::string &obj_class,
                     const std::string &obj_name, long obj_inst, int scope,
                     const std::string &filter)
{
    return m_common(flags, obj_class, obj_name, obj_inst, scope, filter,
                    gpb::M_START);
}

int
CDAPMessage::m_start_r(gpb::flagValues_t flags_, int result_,
                       const std::string &result_reason_)
{
    clear();
    op_code = gpb::M_START_R;
    flags   = flags_;

    result        = result_;
    result_reason = result_reason_;

    return 0;
}

int
CDAPMessage::m_stop(gpb::flagValues_t flags, const std::string &obj_class,
                    const std::string &obj_name, long obj_inst, int scope,
                    const std::string &filter)
{
    return m_common(flags, obj_class, obj_name, obj_inst, scope, filter,
                    gpb::M_STOP);
}

int
CDAPMessage::m_stop_r(gpb::flagValues_t flags_, int result_,
                      const std::string &result_reason_)
{
    clear();
    op_code = gpb::M_STOP_R;
    flags   = flags_;

    result        = result_;
    result_reason = result_reason_;

    return 0;
}

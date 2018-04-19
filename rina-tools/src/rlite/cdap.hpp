/*
 * libcdap API for applications.
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

#ifndef __CDAP_H__
#define __CDAP_H__

#include <string>
#include <unordered_set>
#include <ctime>

#include "CDAP.pb.h"

struct CDAPAuthValue {
    std::string name;
    std::string password;
    std::string other;

    bool empty() const
    {
        return name == std::string() && password == std::string() &&
               other == std::string();
    }
};

#define CDAP_DISCARD_SECS_DFLT 15

class InvokeIdMgr {
    struct Id {
        int iid;
        time_t created;

        Id(int i, time_t t) : iid(i), created(t) {}
        Id(const Id &o) : iid(o.iid), created(o.created) {}
        bool operator==(const Id &o) const { return iid == o.iid; }
        bool operator!=(const Id &o) const { return iid != o.iid; }
        bool operator<(const Id &o) const { return iid < o.iid; }
        bool operator>(const Id &o) const { return iid > o.iid; }
    };

    /* Hasher class to allow for struct Id to be used as key type for
     * unordered maps and unordered sets. */
    struct IdHasher {
        std::size_t operator()(const Id &i) const
        {
            return std::hash<int>()(i.iid);
        }
    };

    std::unordered_set<Id, IdHasher> pending_invoke_ids;
    int invoke_id_next;
    unsigned discard_secs;
    std::unordered_set<Id, IdHasher> pending_invoke_ids_remote;

    int __put_invoke_id(std::unordered_set<Id, IdHasher> &pending,
                        int invoke_id);
    void __discard(std::unordered_set<Id, IdHasher> &pending);
    void discard();

public:
    InvokeIdMgr(unsigned discard_secs = CDAP_DISCARD_SECS_DFLT);
    int get_invoke_id();
    int put_invoke_id(int invoke_id);
    int get_invoke_id_remote(int invoke_id);
    int put_invoke_id_remote(int invoke_id);
    unsigned size() const
    {
        return pending_invoke_ids.size() + pending_invoke_ids_remote.size();
    }
};

struct CDAPMessage;

class CDAPConn {
    InvokeIdMgr invoke_id_mgr;
    unsigned int discard_secs;

#ifndef SWIG
    enum class ConnState {
        NONE = 1,
        AWAITCON,
        CONNECTED,
        AWAITCLOSE,
    } state;
#endif /* SWIG */

    const char *conn_state_repr(ConnState st);
    int conn_fsm_run(CDAPMessage *m, bool sender);

    CDAPConn(const CDAPConn &o);

public:
    CDAPConn(int fd, long version = 1,
             unsigned int discard_secs = CDAP_DISCARD_SECS_DFLT);
    ~CDAPConn();

    /* @invoke_id is not meaningful for request messages. */
    int msg_send(CDAPMessage *m, int invoke_id);
    int msg_ser(CDAPMessage *m, int invoke_id, char **buf, size_t *len);

    std::unique_ptr<CDAPMessage> msg_recv();
    std::unique_ptr<CDAPMessage> msg_deser(const char *serbuf, size_t serlen);

    void reset();
    bool connected() const { return state == ConnState::CONNECTED; }
    void state_set(unsigned int s) { state = static_cast<ConnState>(s); }
    unsigned int state_get() { return static_cast<unsigned int>(state); }

    /* Helper function to send M_CONNECT and wait for the M_CONNECT_R
     * response. */
    int connect(const std::string &src, const std::string &dst,
                gpb::AuthType auth_mech,
                const struct CDAPAuthValue *auth_value);

    /* Helper function to wait for M_CONNECT and send the M_CONNECT_R
     * response. */
    std::unique_ptr<CDAPMessage> accept();

    std::string local_appl;
    std::string remote_appl;
    int fd;
    long version;
};

std::unique_ptr<CDAPMessage> msg_deser_stateless(const char *serbuf,
                                                 size_t serlen);

int msg_ser_stateless(CDAPMessage *m, char **buf, size_t *len);

/* Internal representation of a CDAP message. */
struct CDAPMessage {
    int abs_syntax          = 0;
    gpb::AuthType auth_mech = gpb::AUTH_NONE;
    CDAPAuthValue auth_value;
    std::string src_appl;
    std::string dst_appl;
    std::string filter;
    gpb::CDAPFlags flags = gpb::F_NO_FLAGS;
    int invoke_id        = 0;
    std::string obj_class;
    long obj_inst = 0;
    std::string obj_name;
    gpb::OpCode op_code = gpb::M_CONNECT;
    int result          = 0;
    std::string result_reason;
    int scope    = 0;
    long version = 0;

    bool is_request() const { return !is_response(); }
    bool is_response() const { return op_code & 0x1; }
    bool invoke_id_valid() const { return invoke_id != 0; }

    void dump() const;

    CDAPMessage();
    CDAPMessage(const CDAPMessage &o);
    CDAPMessage &operator=(const CDAPMessage &o);
    ~CDAPMessage();

    CDAPMessage(const gpb::CDAPMessage &gm);
    operator gpb::CDAPMessage() const;

    bool valid(bool check_invoke_id) const;

    void get_obj_value(int32_t &v) const;
    void set_obj_value(int32_t v);
    void get_obj_value(int64_t &v) const;
    void set_obj_value(int64_t v);
    void get_obj_value(float &v) const;
    void set_obj_value(float v);
    void get_obj_value(double &v) const;
    void set_obj_value(double v);
    void get_obj_value(bool &v) const;
    void set_obj_value(bool v);
    void get_obj_value(std::string &v) const;
    void set_obj_value(const std::string &v);
    void set_obj_value(const char *v);
    void get_obj_value(const char *&p, size_t &l) const;
    void set_obj_value(const char *buf, size_t len); /* borrow */
#ifndef SWIG
    void set_obj_value(std::unique_ptr<char[]> buf, size_t len); /* ownership */
#endif

    int m_connect(gpb::AuthType auth_mech,
                  const struct CDAPAuthValue *auth_value,
                  const std::string &local_appl,
                  const std::string &remote_appl);

    int m_connect_r(const CDAPMessage *req, int result = 0,
                    const std::string &result_reason = std::string());

    int m_release();

    int m_release_r(int result                       = 0,
                    const std::string &result_reason = std::string());

    int m_create(const std::string &obj_class, const std::string &obj_name,
                 long obj_inst = 0, int scope = 0,
                 const std::string &filter = std::string());

    int m_create_r(const std::string &obj_class, const std::string &obj_name,
                   long obj_inst = 0, int result = 0,
                   const std::string &result_reason = std::string());

    int m_delete(const std::string &obj_class, const std::string &obj_name,
                 long obj_inst = 0, int scope = 0,
                 const std::string &filter = std::string());

    int m_delete_r(const std::string &obj_class, const std::string &obj_name,
                   long obj_inst = 0, int result = 0,
                   const std::string &result_reason = std::string());

    int m_read(const std::string &obj_class, const std::string &obj_name,
               long obj_inst = 0, int scope = 0,
               const std::string &filter = std::string());

    int m_read_r(const std::string &obj_class, const std::string &obj_name,
                 long obj_inst = 0, int result = 0,
                 const std::string &result_reason = std::string());

    int m_write(const std::string &obj_class, const std::string &obj_name,
                long obj_inst = 0, int scope = 0,
                const std::string &filter = std::string());

    int m_write_r(int result                       = 0,
                  const std::string &result_reason = std::string());

    int m_cancelread();

    int m_cancelread_r(int result                       = 0,
                       const std::string &result_reason = std::string());

    int m_start(const std::string &obj_class, const std::string &obj_name,
                long obj_inst = 0, int scope = 0,
                const std::string &filter = std::string());

    int m_start_r(int result                       = 0,
                  const std::string &result_reason = std::string());

    int m_stop(const std::string &obj_class, const std::string &obj_name,
               long obj_inst = 0, int scope = 0,
               const std::string &filter = std::string());

    int m_stop_r(int result                       = 0,
                 const std::string &result_reason = std::string());

    void clear() { *this = CDAPMessage(); }

    static std::string opcode_repr(gpb::OpCode);

private:
    int m_common(const std::string &obj_class, const std::string &obj_name,
                 long obj_inst, int scope, const std::string &filter,
                 gpb::OpCode op_code);

    int m_common_r(const std::string &obj_class, const std::string &obj_name,
                   long obj_inst, int result, const std::string &result_reason,
                   gpb::OpCode op_code);

    int __m_write(const std::string &obj_class, const std::string &obj_name,
                  long obj_inst, int scope, const std::string &filter);

    void copy(const CDAPMessage &o);
    void destroy();

#ifndef SWIG
    enum class ObjValType {
        NONE,
        I32,
        I64,
        BYTES,
        FLOAT,
        DOUBLE,
        BOOL,
        STRING,
    };
#endif /* SWIG */

    bool is_type(ObjValType tt) const;
    /* Representation of the object value. */
    struct {
        ObjValType ty;
        union {
            int32_t i32; /* intval and sintval */
            int64_t i64; /* int64val and sint64val */
            float fp_single;
            double fp_double;
            bool boolean;
            struct {
                char *ptr;
                size_t len;
                bool owned;
            } buf; /* byteval */
        } u;
        std::string str; /* strval */
    } obj_value;
};

#endif /* __CDAP_H__ */

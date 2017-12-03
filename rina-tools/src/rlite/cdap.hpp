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

class CDAPConn {
    InvokeIdMgr invoke_id_mgr;
    unsigned int discard_secs;

    enum class ConnState {
        NONE = 1,
        AWAITCON,
        CONNECTED,
        AWAITCLOSE,
    } state;

    const char *conn_state_repr(ConnState st);
    int conn_fsm_run(struct CDAPMessage *m, bool sender);

    CDAPConn(const CDAPConn &o);

public:
    CDAPConn(int fd, long version = 1,
             unsigned int discard_secs = CDAP_DISCARD_SECS_DFLT);
    ~CDAPConn();

    /* @invoke_id is not meaningful for request messages. */
    int msg_send(struct CDAPMessage *m, int invoke_id);
    int msg_ser(struct CDAPMessage *m, int invoke_id, char **buf, size_t *len);

    struct CDAPMessage *msg_recv();
    struct CDAPMessage *msg_deser(const char *serbuf, size_t serlen);

    void reset();
    bool connected() const { return state == ConnState::CONNECTED; }
    void state_set(unsigned int s) { state = static_cast<ConnState>(s); }
    unsigned int state_get() { return static_cast<unsigned int>(state); }

    /* Helper function to send M_CONNECT and wait for the M_CONNECT_R
     * response. */
    int connect(const std::string &src, const std::string &dst,
                gpb::authTypes_t auth_mech,
                const struct CDAPAuthValue *auth_value);

    /* Helper function to wait for M_CONNECT and send the M_CONNECT_R
     * response. */
    CDAPMessage *accept();

    std::string local_appl;
    std::string remote_appl;
    int fd;
    long version;
};

struct CDAPMessage *msg_deser_stateless(const char *serbuf, size_t serlen);

int msg_ser_stateless(struct CDAPMessage *m, char **buf, size_t *len);

/* Internal representation of a CDAP message. */
struct CDAPMessage {
    int abs_syntax;
    gpb::authTypes_t auth_mech;
    CDAPAuthValue auth_value;
    std::string src_appl;
    std::string dst_appl;
    std::string filter;
    gpb::flagValues_t flags;
    int invoke_id;
    std::string obj_class;
    long obj_inst;
    std::string obj_name;
    gpb::opCode_t op_code;
    int result;
    std::string result_reason;
    int scope;
    long version;

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

    bool is_type(ObjValType tt) const;
    bool is_request() const { return !is_response(); }
    bool is_response() const { return op_code & 0x1; }

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
    void set_obj_value(const char *buf, size_t len);

    int m_connect(gpb::authTypes_t auth_mech,
                  const struct CDAPAuthValue *auth_value,
                  const std::string &local_appl,
                  const std::string &remote_appl);

    int m_connect_r(const struct CDAPMessage *req, int result,
                    const std::string &result_reason);

    int m_release(gpb::flagValues_t flags);

    int m_release_r(gpb::flagValues_t flags, int result,
                    const std::string &result_reason);

    int m_create(gpb::flagValues_t flags, const std::string &obj_class,
                 const std::string &obj_name, long obj_inst, int scope,
                 const std::string &filter);

    int m_create_r(gpb::flagValues_t flags, const std::string &obj_class,
                   const std::string &obj_name, long obj_inst, int result,
                   const std::string &result_reason);

    int m_delete(gpb::flagValues_t flags, const std::string &obj_class,
                 const std::string &obj_name, long obj_inst, int scope,
                 const std::string &filter);

    int m_delete_r(gpb::flagValues_t flags, const std::string &obj_class,
                   const std::string &obj_name, long obj_inst, int result,
                   const std::string &result_reason);

    int m_read(gpb::flagValues_t flags, const std::string &obj_class,
               const std::string &obj_name, long obj_inst, int scope,
               const std::string &filter);

    int m_read_r(gpb::flagValues_t flags, const std::string &obj_class,
                 const std::string &obj_name, long obj_inst, int result,
                 const std::string &result_reason);

    int m_write(gpb::flagValues_t flags, const std::string &obj_class,
                const std::string &obj_name, long obj_inst, int scope,
                const std::string &filter);

    int m_write_r(gpb::flagValues_t flags, int result,
                  const std::string &result_reason);

    int m_cancelread(gpb::flagValues_t flags);

    int m_cancelread_r(gpb::flagValues_t flags, int result,
                       const std::string &result_reason);

    int m_start(gpb::flagValues_t flags, const std::string &obj_class,
                const std::string &obj_name, long obj_inst, int scope,
                const std::string &filter);

    int m_start_r(gpb::flagValues_t flags, int result,
                  const std::string &result_reason);

    int m_stop(gpb::flagValues_t flags, const std::string &obj_class,
               const std::string &obj_name, long obj_inst, int scope,
               const std::string &filter);

    int m_stop_r(gpb::flagValues_t flags, int result,
                 const std::string &result_reason);

    void clear() { *this = CDAPMessage(); }

private:
    int m_common(gpb::flagValues_t flags, const std::string &obj_class,
                 const std::string &obj_name, long obj_inst, int scope,
                 const std::string &filter, gpb::opCode_t op_code);

    int m_common_r(gpb::flagValues_t flags, const std::string &obj_class,
                   const std::string &obj_name, long obj_inst, int result,
                   const std::string &result_reason, gpb::opCode_t op_code);

    int __m_write(gpb::flagValues_t flags, const std::string &obj_class,
                  const std::string &obj_name, long obj_inst, int scope,
                  const std::string &filter);

    void copy(const CDAPMessage &o);
    void destroy();

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

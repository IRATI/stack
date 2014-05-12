/*
 *  Shim IPC Process over TCP/UDP
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
 *    Douwe De Bock         <douwe.debock@ugent.be>
 *
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/workqueue.h>

#include <net/sock.h>

#define SHIM_NAME   "shim-tcp-udp"

#define RINA_PREFIX SHIM_NAME

#include "logs.h"
#include "common.h"
#include "kipcm.h"
#include "debug.h"
#include "utils.h"
#include "du.h"
#include "ipcp-utils.h"
#include "ipcp-factories.h"

static struct workqueue_struct * rcv_wq;
static struct work_struct        rcv_work;
static struct list_head          rcv_wq_data;
static DEFINE_SPINLOCK(rcv_wq_lock);


extern struct kipcm * default_kipcm;

struct ipcp_instance_data {
        struct list_head    list;
        ipc_process_id_t    id;

        /* IPC Process name */
        struct name *       name;
        struct name *       dif_name;

        struct flow_spec ** qos;

        /* Stores the state of flows indexed by port_id */
        spinlock_t          flow_lock;
        struct list_head    flows;

        spinlock_t          app_lock;
        struct list_head    apps;

        /* FIXME: Remove it as soon as the kipcm_kfa gets removed */
        struct kfa *        kfa;
};

/* FIXME Remove this */
static struct ipcp_instance_data *inst_data;

enum port_id_state {
        PORT_STATE_NULL = 1,
        PORT_STATE_PENDING,
        PORT_STATE_ALLOCATED
};

struct app_data {
        struct list_head list;

        struct socket *  lsock;
        struct socket *  udpsock;

        struct name *    app_name;
        int              port;
};

struct shim_tcp_udp_flow {
        struct list_head   list;

        port_id_t          port_id;
        enum port_id_state port_id_state;

        struct name *      app_name;

        spinlock_t         lock;

        int                fspec_id;

        struct socket *    sock;
        struct sockaddr_in addr;

        struct rfifo *     sdu_queue;
};

struct rcv_data {
        struct list_head list;
        struct sock *    sk;
};

static struct shim_tcp_udp_flow * find_flow(struct ipcp_instance_data *data,
                                            port_id_t id)
{
        struct shim_tcp_udp_flow * flow;

        spin_lock(&data->flow_lock);

        list_for_each_entry(flow, &data->flows, list) {
                if (flow->port_id == id) {
                        spin_unlock(&data->flow_lock);
                        return flow;
                }
        }

        spin_unlock(&data->flow_lock);

        return NULL;
}

static struct shim_tcp_udp_flow *
find_tcp_flow_by_socket(struct ipcp_instance_data * data,
                        const struct socket *       sock)
{
        struct shim_tcp_udp_flow * flow;

        if (!data)
                return NULL;

        list_for_each_entry(flow, &data->flows, list) {
                if (flow->sock == sock) {
                        return flow;
                }
        }

        return NULL;
}

/*
 * FIXME: static bool instead of static int, check if this functionality is
 * already present elsewhere
 */
static int compare_addr(const struct sockaddr_in * f,
                        const struct sockaddr_in * s)
{
        /* FIXME: ASSERT() on input parameters */
        if (f->sin_family == s->sin_family &&
            f->sin_port == s->sin_port &&
            f->sin_addr.s_addr == s->sin_addr.s_addr)
                return 1;
        else
                return 0;
}

static struct shim_tcp_udp_flow *
find_udp_flow(struct ipcp_instance_data * data,
              const struct sockaddr_in *  addr,
              const struct socket *       sock)
{
        struct shim_tcp_udp_flow * flow;

        if (!data)
                return NULL;

        list_for_each_entry(flow, &data->flows, list) {
                if (flow->sock == sock &&
                    compare_addr(addr, &flow->addr)) {
                        return flow;
                }
        }

        return NULL;
}

static struct app_data *
find_app_by_socket(struct ipcp_instance_data * data,
                   const struct socket *       sock)
{
        struct app_data * app;

        if (!data)
                return NULL;

        if (sock->type == SOCK_DGRAM) {
                list_for_each_entry(app, &data->apps, list) {
                        if (app->udpsock == sock) {
                                return app;
                        }
                }
        } else {
                list_for_each_entry(app, &data->apps, list) {
                        if (app->lsock == sock) {
                                return app;
                        }
                }
        }

        return NULL;
}

static struct app_data * find_app_by_name(struct ipcp_instance_data * data,
                                          const struct name *         name)
{
        struct app_data *app;

        if (!data)
                return NULL;

        list_for_each_entry(app, &data->apps, list) {
                if (name_is_equal(app->app_name, name)) {
                        return app;
                }
        }

        return NULL;
}

static struct ipcp_instance_data * get_instance_data(void)
{ return inst_data; }

static int flow_destroy(struct ipcp_instance_data * data,
                        struct shim_tcp_udp_flow *  flow)
{
        if (!data || !flow) {
                LOG_ERR("Couldn't destroy flow.");
                return -1;
        }

        spin_lock(&data->flow_lock);
        if (!list_empty(&flow->list)) {
                list_del(&flow->list);
        }
        spin_unlock(&data->flow_lock);

        rkfree(flow);

        return 0;
}

static void deallocate_and_destroy_flow(struct ipcp_instance_data * data,
                                        struct shim_tcp_udp_flow *  flow)
{
        /* FIXME: ASSERT on data and flow here */
        if (kfa_flow_deallocate(data->kfa, flow->port_id))
                LOG_ERR("Failed to destroy KFA flow");
        if (flow_destroy(data, flow))
                LOG_ERR("Failed to destroy shim-tcp-udp flow");
}

static void tcp_deallocate_and_destroy_flow(struct ipcp_instance_data *data,
                                            struct shim_tcp_udp_flow *flow)
{
        sock_release(flow->sock);
        deallocate_and_destroy_flow(data, flow);
}

static void tcp_udp_rcv(struct sock * sk, int bytes)
{
        struct rcv_data * recvd;

        LOG_DBG("callback on %p", sk->sk_socket);

        recvd = rkmalloc(sizeof(struct rcv_data), GFP_ATOMIC);
        if (!recvd) {
                LOG_ERR("could not allocata rcv_data");
                return;
        }

        recvd->sk = sk;
        INIT_LIST_HEAD(&recvd->list);

        spin_lock(&rcv_wq_lock);
        list_add_tail(&recvd->list, &rcv_wq_data);
        spin_unlock(&rcv_wq_lock);

        queue_work(rcv_wq, &rcv_work);
}

static int
tcp_udp_flow_allocate_request(struct ipcp_instance_data * data,
                              const struct name *         source,
                              const struct name *         dest,
                              const struct flow_spec *    fspec,
                              port_id_t                   id)
{
        struct shim_tcp_udp_flow * flow;
        struct app_data *          app;
        int                        ip, err;

        LOG_DBG("tcp_udp allocate request");

        ASSERT(data);
        ASSERT(source);
        ASSERT(dest);

        app = find_app_by_name(data, source);
        if (!app) {
                LOG_ERR("app not registered, so can't send allocate request");
                return -1;
        }

        flow = find_flow(data, id);
        if (!flow) {
                flow = rkzalloc(sizeof(*flow), GFP_KERNEL);
                if (!flow) {
                        LOG_ERR("could not allocate mem for flow");
                        return -1;
                }

                flow->port_id       = id;
                flow->port_id_state = PORT_STATE_PENDING;
                flow->app_name      = app->app_name;

                INIT_LIST_HEAD(&flow->list);
                spin_lock(&data->flow_lock);
                list_add(&flow->list, &data->flows);
                spin_unlock(&data->flow_lock);
                LOG_DBG("allocate request flow added");

                /* this should be done with DNS or DHT */
                ip = (10 << 24) | (1 << 16) | (1 << 8) | (3);
                flow->addr.sin_addr.s_addr = htonl(ip);
                flow->addr.sin_family      = AF_INET;
                flow->addr.sin_port        = htons(2325); /* FIXME: Define ! */

                if (!fspec->ordered_delivery) {
                        LOG_DBG("unreliable flow requested");
                        flow->sock = app->udpsock;
                        flow->fspec_id = 0;
                } else {
                        LOG_DBG("reliable flow requested");
                        flow->fspec_id = 1;
                        err = sock_create_kern(PF_INET, SOCK_STREAM,
                                               IPPROTO_TCP, &flow->sock);
                        if (err < 0) {
                                LOG_ERR("could not create tcp socket");
                                return -1;
                        }

                        err = kernel_connect(flow->sock,
                                             (struct sockaddr*)&flow->addr,
                                             sizeof(struct sockaddr), 0);
                        if (err < 0) {
                                LOG_ERR("could not connect tcp socket");
				sock_release(flow->sock);
				deallocate_and_destroy_flow(data, flow);
                                return -1;
                        }

                        /* FIXME: Check if ASSERTs are needed */
                        write_lock_bh(&flow->sock->sk->sk_callback_lock);
                        flow->sock->sk->sk_user_data =
                                flow->sock->sk->sk_data_ready;
                        flow->sock->sk->sk_data_ready =
                                tcp_udp_rcv;
                        write_unlock_bh(&flow->sock->sk->sk_callback_lock);
                }

                flow->port_id_state = PORT_STATE_ALLOCATED;
                if (kipcm_flow_commit(default_kipcm, data->id,
                                      flow->port_id)) {
                        LOG_ERR("Cannot add flow");
                        if (fspec->ordered_delivery) {
                                kernel_sock_shutdown(flow->sock, SHUT_RDWR);
                                sock_release(flow->sock);
                        }
			deallocate_and_destroy_flow(data, flow);

                        return -1;
                }

                flow->sdu_queue = NULL;

                if (kipcm_notify_flow_alloc_req_result(default_kipcm, data->id,
                                                       flow->port_id, 0)) {
                        LOG_ERR("couldn't tell flow is allocated to KIPCM");
                        if (fspec->ordered_delivery) {
                                kernel_sock_shutdown(flow->sock, SHUT_RDWR);
                                sock_release(flow->sock);
                        }
			deallocate_and_destroy_flow(data, flow);
                        return -1;
                }
        } else if (flow->port_id_state == PORT_STATE_PENDING) {
                LOG_ERR("Port-id state is already pending ...");
        } else {
                LOG_ERR("Allocate called in a wrong state, how dare you!");
                return -1;
        }

        return 0;
}

static int tcp_udp_flow_allocate_response(struct ipcp_instance_data * data,
                                          port_id_t                   port_id,
                                          int                         result)
{
        struct shim_tcp_udp_flow * flow;

        LOG_DBG("tcp_udp allocate response");

        ASSERT(data);
        ASSERT(is_port_id_ok(port_id));

        flow = find_flow(data, port_id);
        if (!flow) {
                LOG_ERR("Flow does not exist, you shouldn't call this");
                return -1;
        }

        spin_lock(&data->flow_lock);
        if (flow->port_id_state != PORT_STATE_PENDING) {
                LOG_ERR("Flow is already allocated");
                spin_unlock(&data->flow_lock);
                return -1;
        }
        spin_unlock(&data->flow_lock);

        /* On positive response, flow should transition to allocated state */
        if (!result) {
                if (kipcm_flow_commit(default_kipcm, data->id,
                                      flow->port_id)) {
                        LOG_ERR("KIPCM flow add failed");
                        deallocate_and_destroy_flow(data, flow);
                        return -1;
                }

                spin_lock(&data->flow_lock);
                flow->port_id_state = PORT_STATE_ALLOCATED;
                spin_unlock(&data->flow_lock);

                ASSERT(flow->sdu_queue);

                while (!rfifo_is_empty(flow->sdu_queue)) {
                        struct sdu * tmp = NULL;

                        tmp = rfifo_pop(flow->sdu_queue);
                        ASSERT(tmp);

                        LOG_DBG("Got a new element from the fifo");

                        if (kfa_sdu_post(data->kfa, flow->port_id, tmp)) {
                                LOG_ERR("Couldn't post SDU to KFA ...");
                                return -1;
                        }
                }

                rfifo_destroy(flow->sdu_queue, (void (*)(void *)) pdu_destroy);
                flow->sdu_queue = NULL;

        } else {
                spin_lock(&data->flow_lock);
                flow->port_id_state = PORT_STATE_NULL;
                spin_unlock(&data->flow_lock);

                if (flow->fspec_id == 1 &&
                    flow->port_id_state == PORT_STATE_ALLOCATED) {
                        kernel_sock_shutdown(flow->sock, SHUT_RDWR);
                        sock_release(flow->sock);
                }

                /*
                 *  If we would destroy the flow, the application
                 *  we refused would constantly try to allocate
                 *  a flow again. This should only be allowed if
                 *  the IPC manager deallocates the NULL state flow first.
                 */
                ASSERT(flow->sdu_queue);
                rfifo_destroy(flow->sdu_queue, (void (*)(void *)) pdu_destroy);
                flow->sdu_queue = NULL;
        }

        return 0;
}

static int tcp_udp_flow_deallocate(struct ipcp_instance_data *data,
                                   port_id_t id)
{
        struct shim_tcp_udp_flow * flow;

        LOG_DBG("tcp_udp deallocate");
        ASSERT(data);

        flow = find_flow(data, id);
        if (!flow) {
                LOG_ERR("Flow does not exist, cannot remove");
                return -1;
        }

        if (flow->fspec_id == 1 && flow->port_id == PORT_STATE_ALLOCATED) {
                LOG_DBG("closing socket");
                kernel_sock_shutdown(flow->sock, SHUT_RDWR);
                sock_release(flow->sock);
        }

        if (kfa_flow_deallocate(data->kfa, flow->port_id)) {
                LOG_ERR("Failed to deallocate flow in KFA");
                return -1;
        }

        if (flow_destroy(data, flow)) {
                LOG_ERR("Failed to destroy shim-tcp-udp flow");
                return -1;
        }

        return 0;
}

int recv_msg(struct socket *      sock,
             struct sockaddr_in * other,
             unsigned char *      buf,
             int                  len,
             int                  encapsulate)
{
        struct msghdr msg;
        struct kvec   iov;
        int           size = 0;
        char          sbuf[2];
        int           length;
        __be16        nlen;

        msg.msg_control    = NULL;
        msg.msg_controllen = 0;
        msg.msg_flags      = MSG_DONTWAIT;

        if (encapsulate) {
                iov.iov_base    = sbuf;
                iov.iov_len     = 2;

                msg.msg_name    = NULL;
                msg.msg_namelen = 0;

                size = kernel_recvmsg(sock, &msg, &iov, 1, 2, msg.msg_flags);
                if (size == 0) {
                        return 0;
                }
                if (size != 2) {
                        LOG_ERR("could not read size: %d", size);
                        return -1;
                }

                memcpy(&nlen, &sbuf[0], 2);
                length = (int)ntohs(nlen);

                LOG_DBG("incoming message is %d bytes long", length);

                iov.iov_base = buf;
                iov.iov_len = len;

                size = kernel_recvmsg(sock, &msg, &iov, 1,
                                      length, msg.msg_flags);
                if (size == 0) {
                        return 0;
                }
                if (size != length) {
                        LOG_ERR("received size != specified size in message");
                        LOG_ERR("message received: %d bytes and should be: %d",
                                size, length);
                        return -1;
                } else {
                        LOG_DBG("message received: %d bytes", size);
                }
        } else {
                iov.iov_base    = buf;
                iov.iov_len     = len;

                msg.msg_name    = other;
                msg.msg_namelen = sizeof(struct sockaddr_in);

                size = kernel_recvmsg(sock, &msg, &iov, 1,
                                      len, msg.msg_flags);
                if (size == 0) {
                        return 0;
                } else if (size > 0) {
                        LOG_ERR("message received: %d", size);
                }
        }

        return size;
}

int send_msg(struct socket *      sock,
             struct sockaddr_in * other,
             char *               buf,
             int                  len,
             int                  encapsulate)
{
        struct msghdr msg;
        struct kvec   iov;
        char          nbuf[len+2];
        int           size;
        __be16        length;

        LOG_DBG("sending messages with %d bytes", len);

        msg.msg_control    = NULL;
        msg.msg_controllen = 0;
        msg.msg_flags      = 0;

        if (encapsulate) {
                length = htons((short)len);

                /* FIXME: avoid this extra memcpy */
                memcpy(&nbuf[0], &length, sizeof(__be16));
                memcpy(&nbuf[2], buf, len);

                iov.iov_base    = nbuf;
                iov.iov_len     = len + 2;

                msg.msg_name    = NULL;
                msg.msg_namelen = 0;

                size            = kernel_sendmsg(sock, &msg, &iov, 1, len + 2);
        } else {
                iov.iov_base    = buf;
                iov.iov_len     = len;

                msg.msg_name    = other;
                msg.msg_namelen = sizeof(struct sockaddr_in);

                size            = kernel_sendmsg(sock, &msg, &iov, 1, len);
        }

        if (size > 0)
                LOG_DBG("message send: %d bytes", size);

        return size;
}

static int udp_process_msg(struct socket * sock)
{
        struct ipcp_instance_data * data;
        struct shim_tcp_udp_flow *  flow;
        struct sockaddr_in          addr;
        struct app_data *           app;
        struct buffer *             sdubuf;
        struct name *               sname;
        struct sdu *                du;
        char                        buf[512]; /* FIXME: Use a define */
        int                         size;

        LOG_DBG("udp_process_msg");

        memset(&addr, 0, sizeof(struct sockaddr_in));
        size = recv_msg(sock, &addr, buf, 512, 0); /* use sizeof() */
        if (size < 0) {
                LOG_ERR("error during udp recv");
                return -1;
        }

        sdubuf = buffer_create_from(buf, size);
        if (!sdubuf) {
                LOG_ERR("could not create buffer");
                return -1;
        }

        du = sdu_create_buffer_with_ni(sdubuf);
        if (!du) {
                LOG_ERR("Couldn't create sdu");
                buffer_destroy(sdubuf);
                return -1;
        }

        data = get_instance_data();

        spin_lock(&data->flow_lock);
        flow = find_udp_flow(data, &addr, sock);
        if (!flow) {
                LOG_DBG("udp_process_msg: no flow found, creating it");

                flow = rkzalloc(sizeof(*flow), GFP_KERNEL);
                if (!flow) {
                        LOG_ERR("could not allocate flow");
                        spin_unlock(&data->flow_lock);
                        sdu_destroy(du);
                        return -1;
                }

                flow->port_id_state = PORT_STATE_PENDING;

                INIT_LIST_HEAD(&flow->list);
                list_add(&flow->list, &data->flows);
                LOG_DBG("added udp flow");

                flow->port_id  = kfa_port_id_reserve(data->kfa, data->id);
                flow->sock     = sock;
                flow->fspec_id = 0;

                memset(&flow->addr, 0, sizeof(struct sockaddr_in));
                flow->addr.sin_port        = addr.sin_port;
                flow->addr.sin_family      = addr.sin_family;
                flow->addr.sin_addr.s_addr = addr.sin_addr.s_addr;

                if (!is_port_id_ok(flow->port_id)) {
                        LOG_ERR("Port id is not ok");
                        flow->port_id_state = PORT_STATE_NULL;
                        spin_unlock(&data->flow_lock);
                        sdu_destroy(du);
                        if (flow_destroy(data, flow))
                                LOG_ERR("Problems destroying shim-tcp-udp "
                                        "flow");
                        return -1;
                }

                if (kfa_flow_create(data->kfa, data->id, flow->port_id)) {
                        LOG_ERR("Could not create flow in KFA");
                        flow->port_id_state = PORT_STATE_NULL;
                        kfa_port_id_release(data->kfa, flow->port_id);
                        spin_unlock(&data->flow_lock);
                        sdu_destroy(du);
                        if (flow_destroy(data, flow))
                                LOG_ERR("Problems destroying shim-tcp-udp "
                                        "flow");
                        return -1;
                }

                flow->sdu_queue = rfifo_create();
                if (!flow->sdu_queue) {
                        LOG_ERR("Couldn't create the sdu queue "
                                "for a new flow");
                        spin_unlock(&data->flow_lock);
                        sdu_destroy(du);
                        deallocate_and_destroy_flow(data, flow);
                        return -1;
                }

                LOG_DBG("Created the queue");

                /* Store SDU in queue */
                if (rfifo_push(flow->sdu_queue, du)) {
                        LOG_ERR("Could not write %zd bytes into the fifo",
                                sizeof(struct sdu *));
                        spin_unlock(&data->flow_lock);
                        sdu_destroy(du);
                        deallocate_and_destroy_flow(data, flow);
                        return -1;
                }

                spin_unlock(&data->flow_lock);

                app            = find_app_by_socket(data, sock);
                ASSERT(app);

                flow->app_name = app->app_name;
                sname          = name_create_ni();
                if (!name_init_from_ni(sname,
                                       "Unknown app", "", "", "")) {
                        name_destroy(sname);
                        return -1;
                }

                if (kipcm_flow_arrived(default_kipcm,
                                       data->id,
                                       flow->port_id,
                                       data->dif_name,
                                       sname,
                                       app->app_name,
                                       data->qos[0])) {
                        LOG_ERR("Couldn't tell the KIPCM about the flow");
                        deallocate_and_destroy_flow(data, flow);
                        name_destroy(sname);
                        return -1;
                }
                name_destroy(sname);
                LOG_DBG("udp_process_msg: flow created");

        } else {
                LOG_DBG("Flow exists, queueing or delivering or dropping");
                if (flow->port_id_state == PORT_STATE_ALLOCATED) {
                        spin_unlock(&data->flow_lock);

                        if (kfa_sdu_post(data->kfa, flow->port_id, du))
                                return -1;

                } else if (flow->port_id_state == PORT_STATE_PENDING) {
                        LOG_DBG("Queueing frame");

                        if (rfifo_push(flow->sdu_queue, du)) {
                                LOG_ERR("Failed to write %zd bytes"
                                        "into the fifo",
                                        sizeof(struct sdu *));
                                spin_unlock(&data->flow_lock);
                                sdu_destroy(du);
                                return -1;
                        }

                        spin_unlock(&data->flow_lock);
                } else if (flow->port_id_state == PORT_STATE_NULL) {
                        spin_unlock(&data->flow_lock);
                        sdu_destroy(du);
                }
        }

        return 0;
}

static int tcp_process_msg(struct ipcp_instance_data * data,
                           struct socket *             sock)
{
        struct shim_tcp_udp_flow *flow;
        struct buffer *sdubuf;
        struct sdu *du;
        char buf[512]; /* FIXME: Use a define */
        int size;

        flow = find_tcp_flow_by_socket(data, sock);
        ASSERT(flow);

        size = recv_msg(sock, &flow->addr, buf, 512, 1); /* Use sifeof() */
        if (size < 0) {
                LOG_ERR("error during tcp recv");
                return -1;
        }

        if (size == 0 && flow->port_id_state == PORT_STATE_ALLOCATED) {
                LOG_DBG("closing flow");
                kipcm_notify_flow_dealloc(data->id, 0, flow->port_id, 1);
                sock_release(flow->sock);
                return 0;
        }

        sdubuf = buffer_create_from(buf, size);
        if (!sdubuf) {
                LOG_ERR("could not create buffer");
                return -1;
        }

        du = sdu_create_buffer_with_ni(sdubuf);
        if (!du) {
                LOG_ERR("Couldn't create sdu");
                buffer_destroy(sdubuf);
                return -1;
        }

        spin_lock(&data->flow_lock);
        if (flow->port_id_state == PORT_STATE_ALLOCATED) {
                spin_unlock(&data->flow_lock);

                if (kfa_sdu_post(data->kfa, flow->port_id, du))
                        return -1;

        } else if (flow->port_id_state == PORT_STATE_PENDING) {
                LOG_DBG("Queueing frame");

                if (rfifo_push(flow->sdu_queue, du)) {
                        LOG_ERR("Failed to write %zd bytes"
                                "into the fifo",
                                sizeof(struct sdu *));
                        spin_unlock(&data->flow_lock);
                        sdu_destroy(du);
                        return -1;
                }

                spin_unlock(&data->flow_lock);
        } else if (flow->port_id_state == PORT_STATE_NULL) {
                spin_unlock(&data->flow_lock);
                sdu_destroy(du);
        }

        return size;
}

static int tcp_process(struct socket *sock)
{
        struct ipcp_instance_data *data;
        struct shim_tcp_udp_flow *flow;
        struct app_data *app;
        struct socket *acsock;
        struct name *sname;
        int err;

        LOG_DBG("tcp process on %p", sock);

        data = get_instance_data();

        app = find_app_by_socket(data, sock);

        if (!app) {
                //connection exists
                while(tcp_process_msg(data, sock) > 0) {
                }

                return 0;
        } else {
                //accept connection
                err = kernel_accept(app->lsock, &acsock, O_NONBLOCK);
                if (err < 0) {
                        LOG_ERR("could not accept socket");
                        return -1;
                }
                LOG_DBG("accepted socket");

                write_lock_bh(&acsock->sk->sk_callback_lock);
                acsock->sk->sk_user_data = acsock->sk->sk_data_ready;
                acsock->sk->sk_data_ready = tcp_udp_rcv;
                write_unlock_bh(&acsock->sk->sk_callback_lock);

                spin_lock(&data->flow_lock);

                flow = rkzalloc(sizeof(*flow), GFP_KERNEL);
                if (!flow) {
                        LOG_ERR("could not allocate flow");
                        spin_unlock(&data->flow_lock);
                        sock_release(acsock);
                        return -1;
                }

                flow->port_id_state = PORT_STATE_PENDING;
                flow->fspec_id = 1;

                flow->port_id = kfa_port_id_reserve(data->kfa, data->id);
                flow->sock = acsock;

                INIT_LIST_HEAD(&flow->list);
                list_add(&flow->list, &data->flows);
                LOG_DBG("tcp flow added");

                memset(&flow->addr, 0, sizeof(struct sockaddr_in));

                if (!is_port_id_ok(flow->port_id)) {
                        LOG_ERR("Port id is not ok");
                        flow->port_id_state = PORT_STATE_NULL;
                        spin_unlock(&data->flow_lock);
                        sock_release(acsock);
                        if (flow_destroy(data, flow))
                                LOG_ERR("Problems destroying shim-tcp-udp "
                                        "flow");
                        return -1;
                }

                if (kfa_flow_create(data->kfa, data->id, flow->port_id)) {
                        LOG_ERR("Could not create flow in KFA");
                        flow->port_id_state = PORT_STATE_NULL;
                        kfa_port_id_release(data->kfa, flow->port_id);
                        spin_unlock(&data->flow_lock);
                        sock_release(acsock);
                        if (flow_destroy(data, flow))
                                LOG_ERR("Problems destroying shim-tcp-udp "
                                        "flow");
                        return -1;
                }

                LOG_DBG("Added flow to the list");

                flow->sdu_queue = rfifo_create();
                if (!flow->sdu_queue) {
                        LOG_ERR("Couldn't create the sdu queue "
                                "for a new flow");
                        spin_unlock(&data->flow_lock);
                        tcp_deallocate_and_destroy_flow(data, flow);
                        return -1;
                }

                LOG_DBG("Created the queue");

                spin_unlock(&data->flow_lock);

                sname = name_create_ni();
                if (!name_init_from_ni(sname, "Unknown app", "", "", "")) {
                        name_destroy(sname);
                        tcp_deallocate_and_destroy_flow(data, flow);
                        return -1;
                }

                if (kipcm_flow_arrived(default_kipcm,
                                       data->id,
                                       flow->port_id,
                                       data->dif_name,
                                       sname,
                                       app->app_name,
                                       data->qos[1])) {
                        LOG_ERR("Couldn't tell the KIPCM about the flow");
                        tcp_deallocate_and_destroy_flow(data, flow);
                        name_destroy(sname);
                        return -1;
                }
                name_destroy(sname);
                LOG_DBG("tcp_process: flow created");

                return 0;
        }
}

static int tcp_udp_rcv_process_msg(struct sock *sk)
{
        if (sk->sk_socket->type == SOCK_DGRAM) {
                return udp_process_msg(sk->sk_socket);
        } else {
                return tcp_process(sk->sk_socket);
        }
}

static void tcp_udp_rcv_worker(struct work_struct *work)
{
        struct rcv_data *recvd, *next;
        unsigned long flags;

        spin_lock_irqsave(&rcv_wq_lock, flags);
        list_for_each_entry_safe(recvd, next, &rcv_wq_data, list) {
                spin_unlock_irqrestore(&rcv_wq_lock, flags);

                LOG_DBG("worker on %p", recvd->sk->sk_socket);
                if (tcp_udp_rcv_process_msg(recvd->sk))
                        LOG_ERR("failed to process packet");

                spin_lock_irqsave(&rcv_wq_lock, flags);
                list_del(&recvd->list);
                spin_unlock_irqrestore(&rcv_wq_lock, flags);

                rkfree(recvd);

                spin_lock_irqsave(&rcv_wq_lock, flags);
        }
        spin_unlock_irqrestore(&rcv_wq_lock, flags);

        LOG_DBG("Worker finished for now");
}

static int tcp_udp_application_register(struct ipcp_instance_data *data,
                                        const struct name *name)
{
        struct app_data *app;
        struct sockaddr_in sin;
        int err;

        LOG_DBG("tcp_udp app register");
        ASSERT(data);
        ASSERT(name);

        app = find_app_by_name(data, name);
        if (app) {
                LOG_ERR("application already registered with shim dif");
                return -1;
        }

        app = rkzalloc(sizeof(struct app_data), GFP_KERNEL);
        if (!app) {
                LOG_ERR("could not allocate mem for application data");
                return -1;
        }

        app->app_name = name_dup(name);
        if (!app->app_name) {
                char * tmp = name_tostring(name);
                LOG_ERR("Application %s registration has failed", tmp);
                if (tmp) rkfree(tmp);
                rkfree(app);
                return -1;
        }

        app->port = 2325;

        err = sock_create_kern(PF_INET, SOCK_DGRAM, IPPROTO_UDP,
                               &app->udpsock);
        if (err < 0) {
                LOG_ERR("could not create udp socket for registration");
                name_destroy(app->app_name);
                rkfree(app);
                return -1;
        }

        sin.sin_addr.s_addr = htonl(INADDR_ANY);
        sin.sin_family = AF_INET;
        sin.sin_port = htons(app->port);

        err = kernel_bind(app->udpsock, (struct sockaddr*)&sin, sizeof(sin));
        if (err < 0) {
                LOG_ERR("could not bind udp socket for registration");
                sock_release(app->udpsock);
                name_destroy(app->app_name);
                rkfree(app);
                return -1;
        }

        write_lock_bh(&app->udpsock->sk->sk_callback_lock);
        app->udpsock->sk->sk_user_data = app->udpsock->sk->sk_data_ready;
        app->udpsock->sk->sk_data_ready = tcp_udp_rcv;
        write_unlock_bh(&app->udpsock->sk->sk_callback_lock);

        LOG_DBG("udp socket ready");

        err = sock_create_kern(PF_INET, SOCK_STREAM, IPPROTO_TCP, &app->lsock);
        if (err < 0) {
                LOG_ERR("could not create tcp socket for registration");
                sock_release(app->udpsock);
                name_destroy(app->app_name);
                rkfree(app);
                return -1;
        }

        err = kernel_bind(app->lsock, (struct sockaddr*)&sin, sizeof(sin));
        if (err < 0) {
                LOG_ERR("could not bind tcp socket for registration");
                sock_release(app->lsock);
                sock_release(app->udpsock);
                name_destroy(app->app_name);
                rkfree(app);
                return -1;
        }

        err = kernel_listen(app->lsock, 5);
        if (err < 0) {
                LOG_ERR("could not listen on tcp socket for registration");
                sock_release(app->lsock);
                sock_release(app->udpsock);
                name_destroy(app->app_name);
                rkfree(app);
                return -1;
        }

        write_lock_bh(&app->lsock->sk->sk_callback_lock);
        app->lsock->sk->sk_user_data = app->lsock->sk->sk_data_ready;
        app->lsock->sk->sk_data_ready = tcp_udp_rcv;
        write_unlock_bh(&app->lsock->sk->sk_callback_lock);

        LOG_DBG("tcp socket ready");

        INIT_LIST_HEAD(&(app->list));

        spin_lock(&data->app_lock);
        list_add(&(app->list), &(data->apps));
        spin_unlock(&data->app_lock);

        return 0;
}

static int tcp_udp_application_unregister(struct ipcp_instance_data *data,
                                          const struct name *name)
{
        struct app_data *app;

        LOG_DBG("tcp_udp app unregister");
        ASSERT(data);
        ASSERT(name);

        app = find_app_by_name(data, name);
        if (!app) {
                LOG_ERR("app is not registered, so can't unregister");
                return -1;
        }

        /* FIXME: Add checks (for NULLs) in order to avoid crashing here */

        lock_sock(app->udpsock->sk);
        write_lock_bh(&app->udpsock->sk->sk_callback_lock);
        app->udpsock->sk->sk_data_ready = app->udpsock->sk->sk_user_data;
        app->udpsock->sk->sk_user_data = NULL;
        write_unlock_bh(&app->udpsock->sk->sk_callback_lock);
        release_sock(app->udpsock->sk);

        sock_release(app->udpsock);

        LOG_DBG("udp socket destroyed");

        lock_sock(app->lsock->sk);
        write_lock_bh(&app->lsock->sk->sk_callback_lock);
        app->lsock->sk->sk_data_ready = app->lsock->sk->sk_user_data;
        app->lsock->sk->sk_user_data = NULL;
        write_unlock_bh(&app->lsock->sk->sk_callback_lock);
        release_sock(app->lsock->sk);

        kernel_sock_shutdown(app->lsock, SHUT_RDWR);
        sock_release(app->lsock);

        LOG_DBG("tcp socket destroyed");

        name_destroy(app->app_name);

        spin_lock(&data->app_lock);
        list_del(&app->list);
        spin_unlock(&data->app_lock);

        rkfree(app);

        return 0;
}

static int tcp_udp_assign_to_dif(struct ipcp_instance_data * data,
                                 const struct dif_info *     dif_information)
{
        LOG_DBG("tcp_udp assign to dif");
        ASSERT(data);
        ASSERT(dif_information);

        if (data->dif_name) {
                ASSERT(data->dif_name->process_name);

                LOG_ERR("This IPC Process is already assigned to the DIF %s. ",
                        data->dif_name->process_name);
                return -1;
        }

        data->dif_name = name_dup(dif_information->dif_name);
        if (!data->dif_name) {
                LOG_ERR("Error duplicating name, bailing out");
                return -1;
        }

        /* FIXME: bind dif to device */

        return 0;
}

static int tcp_udp_update_dif_config(struct ipcp_instance_data * data,
                                     const struct dif_config *   new_config)
{
        LOG_DBG("tcp_udp update dif config");

        ASSERT(data);
        ASSERT(new_config);

        return 0;
}

static int tcp_udp_sdu_write(struct ipcp_instance_data * data,
                             port_id_t                   id,
                             struct sdu *                sdu)
{
        struct shim_tcp_udp_flow *flow;
        int size;

        ASSERT(data);
        ASSERT(sdu);

        LOG_DBG("Entered the sdu-write");

        flow = find_flow(data, id);
        if (!flow) {
                LOG_ERR("could not find flow with specified port-id");
                sdu_destroy(sdu);
                return -1;
        }

        spin_lock(&data->flow_lock);
        if (flow->port_id_state != PORT_STATE_ALLOCATED) {
                LOG_ERR("Flow is not in the right state to call this");
                sdu_destroy(sdu);
                spin_unlock(&data->flow_lock);
                return -1;
        }
        spin_unlock(&data->flow_lock);

        size = send_msg(flow->sock, &flow->addr,
                        (char*)buffer_data_rw(sdu->buffer),
                        buffer_length(sdu->buffer),
                        flow->fspec_id);
        if (size < 0) {
                LOG_DBG("error during sdu write");
                sdu_destroy(sdu);
                return -1;
        } else if (size < buffer_length(sdu->buffer)) {
                LOG_DBG("could not completly send sdu");
                sdu_destroy(sdu);
                return -1;
        }

        LOG_DBG("Packet sent");
        sdu_destroy(sdu);

        return 0;
}

static const struct name * tcp_udp_ipcp_name(struct ipcp_instance_data *data)
{
        LOG_DBG("tcp_udp name");
        ASSERT(data);
        ASSERT(name_is_ok(data->name));

        return data->name;
}

static struct ipcp_instance_ops tcp_udp_instance_ops = {
        .flow_allocate_request     = tcp_udp_flow_allocate_request,
        .flow_allocate_response    = tcp_udp_flow_allocate_response,
        .flow_deallocate           = tcp_udp_flow_deallocate,
        .flow_binding_ipcp         = NULL,
        .flow_destroy              = NULL,

        .application_register      = tcp_udp_application_register,
        .application_unregister    = tcp_udp_application_unregister,

        .assign_to_dif             = tcp_udp_assign_to_dif,
        .update_dif_config         = tcp_udp_update_dif_config,

        .connection_create         = NULL,
        .connection_update         = NULL,
        .connection_destroy        = NULL,
        .connection_create_arrived = NULL,

        .sdu_enqueue               = NULL,
        .sdu_write                 = tcp_udp_sdu_write,

        .mgmt_sdu_read             = NULL,
        .mgmt_sdu_write            = NULL,
        .mgmt_sdu_post             = NULL,

        .pft_add                   = NULL,
        .pft_remove                = NULL,
        .pft_dump                  = NULL,
        .pft_flush                 = NULL,

        .ipcp_name                 = tcp_udp_ipcp_name,
};

static struct ipcp_factory_data {
        struct list_head instances;
} tcp_udp_data;

static int tcp_udp_init(struct ipcp_factory_data *data)
{
        LOG_DBG("tcp_udp_init");
        ASSERT(data);

        bzero(&tcp_udp_data, sizeof(tcp_udp_data));
        INIT_LIST_HEAD(&(data->instances));

        INIT_LIST_HEAD(&rcv_wq_data);

        INIT_WORK(&rcv_work, tcp_udp_rcv_worker);

        LOG_INFO("%s initialized", SHIM_NAME);

        return 0;
}

static int tcp_udp_fini(struct ipcp_factory_data *data)
{
        LOG_DBG("tcp_udp_fini");
        ASSERT(data);

        ASSERT(list_empty(&(data->instances)));

        return 0;
}

static struct ipcp_instance_data *
find_instance(struct ipcp_factory_data * data, ipc_process_id_t id)
{

        struct ipcp_instance_data * pos;

        list_for_each_entry(pos, &(data->instances), list) {
                if (pos->id == id) {
                        return pos;
                }
        }

        return NULL;

}

static struct ipcp_instance * tcp_udp_create(struct ipcp_factory_data * data,
                                             const struct name *        name,
                                             ipc_process_id_t           id)
{
        struct ipcp_instance * inst;

        LOG_DBG("tcp_udp_create");

        ASSERT(data);

        /* Check if there already is an instance with that id */
        if (find_instance(data,id)) {
                LOG_ERR("There's a shim instance with id %d already", id);
                return NULL;
        }

        /* Create an instance */
        inst = rkzalloc(sizeof(*inst), GFP_KERNEL);
        if (!inst) {
                LOG_ERR("could not allocate mem for ipcp instance");
                return NULL;
        }

        /* fill it properly */
        inst->ops  = &tcp_udp_instance_ops;
        inst->data = rkzalloc(sizeof(struct ipcp_instance_data), GFP_KERNEL);
        if (!inst->data) {
                LOG_ERR("could not allocate mem for inst data");
                rkfree(inst);
                return NULL;
        }

        inst->data->id = id;

        inst->data->name = name_dup(name);
        if (!inst->data->name) {
                LOG_ERR("Failed creation of qos cubes");
                rkfree(inst->data);
                rkfree(inst);
                return NULL;
        }

        inst->data->qos = rkzalloc(2*sizeof(struct flow_spec*), GFP_KERNEL);
        if (!inst->data->qos) {
                LOG_ERR("Failed creation of qos cube 1");
                name_destroy(inst->data->name);
                rkfree(inst->data);
                rkfree(inst);
                return NULL;
        }

        inst->data->qos[0] = rkzalloc(sizeof(struct flow_spec), GFP_KERNEL);
        if (!inst->data->qos[0]) {
                LOG_ERR("Failed creation of qos cube 2");
                rkfree(inst->data->qos);
                name_destroy(inst->data->name);
                rkfree(inst->data);
                rkfree(inst);
                return NULL;
        }

        inst->data->qos[1] = rkzalloc(sizeof(struct flow_spec), GFP_KERNEL);
        if (!inst->data->qos[1]) {
                LOG_ERR("Failed creation of ipc name");
                rkfree(inst->data->qos[0]);
                rkfree(inst->data->qos);
                name_destroy(inst->data->name);
                rkfree(inst->data);
                rkfree(inst);
                return NULL;
        }

        inst->data->qos[0]->average_bandwidth           = 0;
        inst->data->qos[0]->average_sdu_bandwidth       = 0;
        inst->data->qos[0]->delay                       = 0;
        inst->data->qos[0]->jitter                      = 0;
        inst->data->qos[0]->max_allowable_gap           = -1;
        inst->data->qos[0]->max_sdu_size                = 512; /* FIXME: !!! */
        inst->data->qos[0]->ordered_delivery            = 0;
        inst->data->qos[0]->partial_delivery            = 1;
        inst->data->qos[0]->peak_bandwidth_duration     = 0;
        inst->data->qos[0]->peak_sdu_bandwidth_duration = 0;
        inst->data->qos[0]->undetected_bit_error_rate   = 0;

        inst->data->qos[1]->average_bandwidth           = 0;
        inst->data->qos[1]->average_sdu_bandwidth       = 0;
        inst->data->qos[1]->delay                       = 0;
        inst->data->qos[1]->jitter                      = 0;
        inst->data->qos[1]->max_allowable_gap           = 0;
        inst->data->qos[1]->max_sdu_size                = 512; /* FIXME: !!! */
        inst->data->qos[1]->ordered_delivery            = 1;
        inst->data->qos[1]->partial_delivery            = 0;
        inst->data->qos[1]->peak_bandwidth_duration     = 0;
        inst->data->qos[1]->peak_sdu_bandwidth_duration = 0;
        inst->data->qos[1]->undetected_bit_error_rate   = 0;

        /* FIXME: Remove as soon as the kipcm_kfa gets removed*/
        inst->data->kfa = kipcm_kfa(default_kipcm);

        ASSERT(inst->data->kfa);

        LOG_DBG("KFA instance %pK bound to the shim tcp-udp", inst->data->kfa);

        inst_data = inst->data;

        spin_lock_init(&inst->data->flow_lock);
        spin_lock_init(&inst->data->app_lock);

        INIT_LIST_HEAD(&(inst->data->flows));
        INIT_LIST_HEAD(&(inst->data->apps));

        /*
         * Bind the shim-instance to the shims set, to keep all our data
         * structures linked (somewhat) together
         */
        INIT_LIST_HEAD(&(inst->data->list));
        list_add(&(inst->data->list), &(data->instances));

        return inst;
}

static int tcp_udp_destroy(struct ipcp_factory_data *data,
                           struct ipcp_instance *instance)
{
        struct ipcp_instance_data *pos, *next;

        LOG_DBG("tcp_udp_destroy");

        ASSERT(data);
        ASSERT(instance);

        LOG_DBG("Looking for the tcp-udp-ipcp-instance to destroy");

        list_for_each_entry_safe(pos, next, &data->instances, list) {
                if (pos->id == instance->data->id) {

                        /* Unbind from the instances set */
                        list_del(&pos->list);

                        /* Destroy it */
                        if (pos->qos[0])
                                rkfree(pos->qos[0]);
                        if (pos->qos[1])
                                rkfree(pos->qos[1]);
                        if (pos->qos)
                                rkfree(pos->qos);

                        if (pos->name)
                                name_destroy(pos->name);
                        if (pos->dif_name)
                                name_destroy(pos->dif_name);

                        rkfree(pos);
                        rkfree(instance);

                        LOG_DBG("tcp-udp shim instance destroyed, returning");

                        return 0;
                }
        }

        LOG_DBG("Didn't find instance, returning error");

        return -1;
}

static struct ipcp_factory_ops tcp_udp_ops = {
        .init    = tcp_udp_init,
        .fini    = tcp_udp_fini,
        .create  = tcp_udp_create,
        .destroy = tcp_udp_destroy,
};

static struct ipcp_factory *shim = NULL;

static int __init mod_init(void)
{
        LOG_DBG("init tcp-udp-shim dif module");

        rcv_wq = alloc_workqueue(SHIM_NAME,
                                 WQ_MEM_RECLAIM | WQ_HIGHPRI | WQ_UNBOUND, 1);
        if (!rcv_wq) {
                LOG_CRIT("Cannot create a workqueue for shim %s", SHIM_NAME);
                return -1;
        }

        shim = kipcm_ipcp_factory_register(default_kipcm, SHIM_NAME,
                                           &tcp_udp_data, &tcp_udp_ops);
        if (!shim)
                return -1;

        return 0;
}

static void __exit mod_exit(void)
{
        struct rcv_data *recvd;

        LOG_DBG("exit tcp-udp-shim dif module");

        flush_workqueue(rcv_wq);
        destroy_workqueue(rcv_wq);

        list_for_each_entry(recvd, &rcv_wq_data, list) {
                list_del(&recvd->list);
                rkfree(recvd);
        }

        kipcm_ipcp_factory_unregister(default_kipcm, shim);
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("RINA Shim IPC over TCP/UDP");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini@nextworks.it>");
MODULE_AUTHOR("Miquel Tarzan <miquel.tarzan@i2cat.net>");
MODULE_AUTHOR("Sander Vrijders <sander.vrijders@intec.ugent.be>");
MODULE_AUTHOR("Douwe De Bock <douwe.debock@ugent.be>");

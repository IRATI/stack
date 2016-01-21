/*
 *  Shim IPC Process over TCP/UDP
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
 *    Douwe De Bock         <douwe.debock@ugent.be>
 *
 * CONTRIBUTORS:
 *
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
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
#include <linux/mutex.h>
#include <net/sock.h>

#define SHIM_NAME     "shim-tcp-udp"
#define SHIM_NAME_RWQ SHIM_NAME "-rwq"
#define SHIM_NAME_WWQ SHIM_NAME "-wwq"

#define RINA_PREFIX SHIM_NAME

#include "logs.h"
#include "common.h"
#include "kipcm.h"
#include "debug.h"
#include "utils.h"
#include "du.h"
#include "ipcp-utils.h"
#include "ipcp-factories.h"
#include "rds/robjects.h"

#define CUBE_UNRELIABLE 0
#define CUBE_RELIABLE   1

static struct workqueue_struct * rcv_wq;
static struct workqueue_struct * snd_wq;
static struct work_struct        rcv_work;
static struct work_struct        snd_work;
static struct list_head          rcv_wq_data;
static struct list_head          snd_wq_data;
static DEFINE_SPINLOCK(rcv_wq_lock);
static DEFINE_SPINLOCK(snd_wq_lock);

/* Structure for the workqueue */
struct rcv_data {
        struct list_head list;
        struct sock *    sk;
};

struct snd_data {
        struct list_head            list;
        struct ipcp_instance_data * data;
        port_id_t                   id;
        struct sdu *                sdu;
};

/* FIXME: To be removed ABSOLUTELY */
extern struct kipcm * default_kipcm;

/*
 * Mapping of ipcp_instance_data to host_name
 * Needed for handling incomming SDUs
 */
struct host_ipcp_instance_mapping {
        struct list_head            list;

        struct ipcp_instance_data * data;
        __be32                      host_name;
};

static DEFINE_SPINLOCK(data_instances_lock);
static struct list_head data_instances_list;

enum port_id_state {
        PORT_STATE_NULL = 1,
        PORT_STATE_PENDING,
        PORT_STATE_ALLOCATED
};

/* Data for registrated applications */
struct reg_app_data {
        struct list_head list;

        struct socket *  tcpsock;
        struct socket *  udpsock;

        struct name *    app_name;
        int              port;
};

struct shim_tcp_udp_flow {
        struct list_head       list;

        port_id_t              port_id;
        enum port_id_state     port_id_state;

        spinlock_t             lock;

        int                    fspec_id;

        struct socket *        sock;
        struct sockaddr_in     addr;

        struct rfifo *         sdu_queue;

        int                    bytes_left;
        int                    lbuf;
        char *                 buf;

        struct ipcp_instance * user_ipcp;
};

struct ipcp_instance_data {
        struct list_head    list;
        ipc_process_id_t    id;

        /* IPC Process name */
        struct name *       name;
        struct name *       dif_name;

        __be32              host_name;

        struct flow_spec ** qos;

        /* Stores the state of flows indexed by port_id */
        struct list_head    flows;

        /* Holds reg_app_data, e.g. the registrated applications */
        struct list_head    reg_apps;

        struct list_head    directory;
        struct list_head    exp_regs;

        spinlock_t          lock;
        /* FIXME: Remove it as soon as the kipcm_kfa gets removed */
        struct kfa *        kfa;
};

/* Directory entry */
struct dir_entry {
        struct list_head list;

        struct name *    app_name;

        int              ip_address;
        int              port;
};

/* Expected application registration */
struct exp_reg {
        struct list_head list;

        struct name *    app_name;
        int              port;
};

static ssize_t shim_tcp_udp_ipcp_attr_show(struct robject *        robj,
					   struct robj_attribute * attr,
					   char *                  buf)
{
	struct ipcp_instance * instance;

	instance = container_of(robj, struct ipcp_instance, robj);
	if (!instance || !instance->data)
		return 0;

	if (strcmp(robject_attr_name(attr), "name") == 0)
		return sprintf(buf, "%s\n",
			name_tostring(instance->data->name));
	if (strcmp(robject_attr_name(attr), "dif") == 0)
		return sprintf(buf, "%s\n",
			name_tostring(instance->data->dif_name));
	if (strcmp(robject_attr_name(attr), "type") == 0)
		return sprintf(buf, "shim-tcp-udp\n");
	if (strcmp(robject_attr_name(attr), "host_name") == 0)
		return sprintf(buf, "%pI4\n", &instance->data->host_name);

	return 0;
}
RINA_SYSFS_OPS(shim_tcp_udp_ipcp);
RINA_ATTRS(shim_tcp_udp_ipcp, name, type, dif, host_name);
RINA_KTYPE(shim_tcp_udp_ipcp);

static struct host_ipcp_instance_mapping *
inst_data_mapping_get(__be32 host_name)
{
        struct host_ipcp_instance_mapping * mapping;

        ASSERT(host_name);

        spin_lock(&data_instances_lock);

        list_for_each_entry(mapping, &data_instances_list, list) {
                if (mapping->host_name == host_name) {
                        spin_unlock(&data_instances_lock);
                        return mapping;
                }
        }

        spin_unlock(&data_instances_lock);

        return NULL;
}

static struct dir_entry * find_dir_entry(struct ipcp_instance_data * data,
                                         const struct name *         app_name)
{
        struct dir_entry * entry;

        ASSERT(app_name);
        ASSERT(data);

        spin_lock(&data->lock);

        list_for_each_entry(entry, &data->directory, list) {
                if (name_cmp(NAME_CMP_APN | NAME_CMP_AEN,
                             entry->app_name, app_name)) {
                        spin_unlock(&data->lock);
                        return entry;
                }
        }

        spin_unlock(&data->lock);

        return NULL;
}

static struct exp_reg * find_exp_reg(struct ipcp_instance_data * data,
                                     const struct name *         app_name)
{
        struct exp_reg * exp;

        ASSERT(app_name);
        ASSERT(data);

        spin_lock(&data->lock);

        list_for_each_entry(exp, &data->exp_regs, list) {
                if (name_cmp(NAME_CMP_APN | NAME_CMP_AEN,
                             exp->app_name, app_name)) {
                        spin_unlock(&data->lock);
                        return exp;
                }
        }

        spin_unlock(&data->lock);

        return NULL;
}

static struct shim_tcp_udp_flow *
find_flow_by_port(struct ipcp_instance_data * data,
                  port_id_t                   id)
{
        struct shim_tcp_udp_flow * flow;

        LOG_HBEAT;

        ASSERT(data);
        ASSERT(is_port_id_ok(id));

        spin_lock(&data->lock);

        list_for_each_entry(flow, &data->flows, list) {
                if (flow->port_id == id) {
                        spin_unlock(&data->lock);
                        return flow;
                }
        }

        spin_unlock(&data->lock);

        return NULL;
}

static struct shim_tcp_udp_flow *
find_flow_by_socket(struct ipcp_instance_data * data,
                    const struct socket *       sock)
{
        struct shim_tcp_udp_flow * flow;

        LOG_HBEAT;

        ASSERT(data);

        spin_lock(&data->lock);

        list_for_each_entry(flow, &data->flows, list) {
                if (flow->sock == sock) {
                        spin_unlock(&data->lock);
                        return flow;
                }
        }

        spin_unlock(&data->lock);

        return NULL;
}

static bool compare_sockaddr_in(const struct sockaddr_in * f,
                                const struct sockaddr_in * s)
{
        ASSERT(f);
        ASSERT(s);

        return ((f->sin_family      == s->sin_family)      &&
                (f->sin_port        == s->sin_port)        &&
                (f->sin_addr.s_addr == s->sin_addr.s_addr));
}

/* No lock needed here, called only when already holding a lock */
static struct shim_tcp_udp_flow *
find_udp_flow(struct ipcp_instance_data * data,
              const struct sockaddr_in *  addr,
              const struct socket *       sock)
{
        struct shim_tcp_udp_flow * flow;

        LOG_HBEAT;

        ASSERT(data);
        ASSERT(addr);
        ASSERT(sock);

        list_for_each_entry(flow, &data->flows, list) {
                if (flow->sock == sock &&
                    compare_sockaddr_in(addr, &flow->addr)) {
                        return flow;
                }
        }

        return NULL;
}

static struct reg_app_data *
find_app_by_socket(struct ipcp_instance_data * data,
                   const struct socket *       sock)
{
        struct reg_app_data * app;

        LOG_HBEAT;

        ASSERT(data);
        ASSERT(sock);

        spin_lock(&data->lock);

        /* FIXME: Shrink this code */
        if (sock->type == SOCK_DGRAM) {
                list_for_each_entry(app, &data->reg_apps, list) {
                        if (app->udpsock == sock) {
                                spin_unlock(&data->lock);
                                return app;
                        }
                }
        } else {
                list_for_each_entry(app, &data->reg_apps, list) {
                        if (app->tcpsock == sock) {
                                spin_unlock(&data->lock);
                                return app;
                        }
                }
        }

        spin_unlock(&data->lock);

        return NULL;
}

static struct reg_app_data * find_app_by_name(struct ipcp_instance_data * data,
                                              const struct name *         name)
{
        struct reg_app_data * app;

        LOG_HBEAT;

        ASSERT(data);
        ASSERT(name);

        spin_lock(&data->lock);

        list_for_each_entry(app, &data->reg_apps, list) {
                if (name_cmp(NAME_CMP_APN | NAME_CMP_AEN,
                             app->app_name, name)) {
                        spin_unlock(&data->lock);
                        return app;
                }
        }

        spin_unlock(&data->lock);

        return NULL;
}

static int flow_destroy(struct ipcp_instance_data * data,
                        struct shim_tcp_udp_flow *  flow)
{
        LOG_HBEAT;

        ASSERT(data);
        ASSERT(flow);

        spin_lock(&data->lock);
        if (!list_empty(&flow->list))
                list_del(&flow->list);
        spin_unlock(&data->lock);

        /* FIXME: Check for leaks */
        rkfree(flow);

        return 0;
}

static int unbind_and_destroy_flow(struct ipcp_instance_data * data,
                                   struct shim_tcp_udp_flow *  flow)
{
        LOG_HBEAT;

        ASSERT(data);

        ASSERT(flow);

        if (flow->user_ipcp) {
                ASSERT(flow->user_ipcp->ops);
                ASSERT(flow->user_ipcp->ops->flow_unbinding_ipcp);

                flow->user_ipcp->ops->flow_unbinding_ipcp(flow->user_ipcp->data,
                                                          flow->port_id);
        }

        if (flow_destroy(data, flow)) {
                LOG_ERR("Failed to destroy a flow");
                return -1;
        }

        return 0;
}

static int tcp_udp_unbind_user_ipcp(struct ipcp_instance_data * data,
                                    port_id_t                   id)
{
        struct shim_tcp_udp_flow * flow;

        flow = find_flow_by_port(data, id);
        if (!flow)
                return -1;

        spin_lock(&data->lock);
        if (flow->user_ipcp) {
                flow->user_ipcp = NULL;
        }
        spin_unlock(&data->lock);

        return 0;
}

static int tcp_unbind_and_destroy_flow(struct ipcp_instance_data * data,
                                       struct shim_tcp_udp_flow *  flow)
{
        LOG_HBEAT;

        ASSERT(data);
        ASSERT(flow);

        sock_release(flow->sock);

        return unbind_and_destroy_flow(data, flow);
}

static void tcp_udp_rcv(struct sock * sk)
{
        struct rcv_data * recvd;

        if (!sk) {
                LOG_ERR("Bad socket passed to callback, bailing out");
                return;
        }
        LOG_DBG("Callback on socket %pK", sk->sk_socket);

        recvd = rkmalloc(sizeof(struct rcv_data), GFP_ATOMIC);
        if (!recvd) {
                LOG_ERR("Could not allocate rcv_data");
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
                              struct ipcp_instance *      user_ipcp,
                              const struct name *         source,
                              const struct name *         dest,
                              const struct flow_spec *    fspec,
                              port_id_t                   id)
{
        struct shim_tcp_udp_flow * flow;
        struct sockaddr_in         sin;
        struct dir_entry *         entry;
        int                        err;
        struct ipcp_instance *     ipcp;

        LOG_HBEAT;

        ASSERT(data);
        ASSERT(source);
        ASSERT(dest);
        ASSERT(user_ipcp);

        flow = find_flow_by_port(data, id);
        if (!flow) {
                flow = rkzalloc(sizeof(*flow), GFP_KERNEL);
                if (!flow) {
                        LOG_ERR("Could not allocate memory for flow");
                        return -1;
                }

                flow->port_id       = id;
                flow->port_id_state = PORT_STATE_PENDING;
                flow->user_ipcp     = user_ipcp;

                INIT_LIST_HEAD(&flow->list);
                spin_lock(&data->lock);
                list_add(&flow->list, &data->flows);
                spin_unlock(&data->lock);
                LOG_DBG("Allocate request flow added");

                /* FIXME: This should be done with DNS or DHT */
                entry = find_dir_entry(data, dest);
                if (!entry) {
                        LOG_ERR("Directory entry not found");
                        list_del(&flow->list);
                        rkfree(flow);
                        return -1;
                }
                LOG_DBG("Directory entry found");

                flow->addr.sin_addr.s_addr = htonl(entry->ip_address);
                flow->addr.sin_family      = AF_INET;
                flow->addr.sin_port        = htons(entry->port);

                LOG_DBG("Max allowable gap is %d", fspec->max_allowable_gap);
                if (!fspec->max_allowable_gap == 0) {
                        LOG_DBG("Unreliable flow requested");
                        flow->fspec_id = 0;

                        err = sock_create_kern(PF_INET, SOCK_DGRAM,
                                               IPPROTO_UDP, &flow->sock);
                        if (err < 0) {
                                LOG_ERR("Could not create UDP socket");
                                unbind_and_destroy_flow(data, flow);
                                return -1;
                        }

                        sin.sin_addr.s_addr = htonl(data->host_name);
                        sin.sin_family      = AF_INET;
                        sin.sin_port        = htons(0);

                        err = kernel_bind(flow->sock, (struct sockaddr*) &sin,
                                          sizeof(sin));
                        if (err < 0) {
                                LOG_ERR("Could not bind UDP socket for alloc");
                                sock_release(flow->sock);
                                unbind_and_destroy_flow(data, flow);
                                return -1;
                        }

                        write_lock_bh(&flow->sock->sk->sk_callback_lock);
                        flow->sock->sk->sk_user_data  =
                                flow->sock->sk->sk_data_ready;
                        flow->sock->sk->sk_data_ready =
                                tcp_udp_rcv;
                        write_unlock_bh(&flow->sock->sk->sk_callback_lock);
                } else {
                        LOG_DBG("Reliable flow requested");
                        flow->fspec_id = 1;

                        err = sock_create_kern(PF_INET, SOCK_STREAM,
                                               IPPROTO_TCP, &flow->sock);
                        if (err < 0) {
                                LOG_ERR("Could not create TCP socket");
                                unbind_and_destroy_flow(data, flow);
                                return -1;
                        }

                        err = kernel_connect(flow->sock,
                                             (struct sockaddr*)&flow->addr,
                                             sizeof(struct sockaddr), 0);
                        if (err < 0) {
                                LOG_ERR("Could not connect TCP socket");
                                sock_release(flow->sock);
                                unbind_and_destroy_flow(data, flow);
                                return -1;
                        }

                        write_lock_bh(&flow->sock->sk->sk_callback_lock);
                        flow->sock->sk->sk_user_data  =
                                flow->sock->sk->sk_data_ready;
                        flow->sock->sk->sk_data_ready =
                                tcp_udp_rcv;
                        write_unlock_bh(&flow->sock->sk->sk_callback_lock);
                }

                flow->port_id_state = PORT_STATE_ALLOCATED;
                ipcp = kipcm_find_ipcp(default_kipcm, data->id);
                if (!ipcp) {
                        LOG_ERR("KIPCM could not retrieve this IPCP");
                        if (fspec->ordered_delivery) {
                                kernel_sock_shutdown(flow->sock, SHUT_RDWR);
                                sock_release(flow->sock);
                        }
                        unbind_and_destroy_flow(data, flow);
                        return -1;
                }

                ASSERT(user_ipcp);
                ASSERT(user_ipcp->ops);
                ASSERT(user_ipcp->ops->flow_binding_ipcp);

                if (user_ipcp->ops->flow_binding_ipcp(user_ipcp->data,
                                                      flow->port_id,
                                                      ipcp)) {
                        LOG_ERR("Could not bind flow with user_ipcp");
                        if (fspec->ordered_delivery) {
                                kernel_sock_shutdown(flow->sock, SHUT_RDWR);
                                sock_release(flow->sock);
                        }
                        unbind_and_destroy_flow(data, flow);
                        return -1;
                }

                if (flow->sdu_queue)
                        if (!rfifo_is_empty(flow->sdu_queue)) {
                                LOG_ERR("We are going to leak ...");
                        }
                flow->sdu_queue = NULL;

                if (kipcm_notify_flow_alloc_req_result(default_kipcm, data->id,
                                                       flow->port_id, 0)) {
                        LOG_ERR("Couldn't tell flow is allocated to KIPCM");
                        if (fspec->ordered_delivery) {
                                kernel_sock_shutdown(flow->sock, SHUT_RDWR);
                                sock_release(flow->sock);
                        }
                        unbind_and_destroy_flow(data, flow);
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

static int
tcp_udp_flow_allocate_response(struct ipcp_instance_data * data,
                               struct ipcp_instance *      user_ipcp,
                               port_id_t                   port_id,
                               int                         result)
{
        struct shim_tcp_udp_flow * flow;
        struct reg_app_data *      app;
        struct ipcp_instance *     ipcp;

        LOG_HBEAT;

        ASSERT(data);
        ASSERT(is_port_id_ok(port_id));

        if (!user_ipcp) {
                LOG_ERR("Wrong user_ipcp passed, bailing out");
                kfa_port_id_release(data->kfa, port_id);
                return -1;
        }

        flow = find_flow_by_port(data, port_id);
        if (!flow) {
                LOG_ERR("Flow does not exist, you shouldn't call this");
                kfa_port_id_release(data->kfa, port_id);
                return -1;
        }

        spin_lock(&data->lock);
        if (flow->port_id_state != PORT_STATE_PENDING) {
                spin_unlock(&data->lock);
                LOG_ERR("Flow is not pending");
                return -1;
        }
        spin_unlock(&data->lock);

        /* On positive response, flow should transition to allocated state */
        if (!result) {
                spin_lock(&data->lock);
                flow->port_id_state = PORT_STATE_ALLOCATED;
                flow->user_ipcp     = user_ipcp;
                spin_unlock(&data->lock);

                ipcp = kipcm_find_ipcp(default_kipcm, data->id);
                if (!ipcp) {
                        LOG_ERR("KIPCM could not retrieve this IPCP");
                        kfa_port_id_release(data->kfa, port_id);
                        unbind_and_destroy_flow(data, flow);
                        return -1;
                }

                ASSERT(user_ipcp);
                ASSERT(user_ipcp->ops);
                ASSERT(user_ipcp->ops->flow_binding_ipcp);

                if (user_ipcp->ops->flow_binding_ipcp(user_ipcp->data,
                                                      flow->port_id,
                                                      ipcp)) {
                        LOG_ERR("Could not bind flow with user_ipcp");
                        kfa_port_id_release(data->kfa, port_id);
                        unbind_and_destroy_flow(data, flow);
                        return -1;
                }

                LOG_DBG("Moving SDUs from sdu-queue to user-IPCP");

                ASSERT(flow->sdu_queue);
                while (!rfifo_is_empty(flow->sdu_queue)) {
                        struct sdu * tmp = NULL;

                        tmp = rfifo_pop(flow->sdu_queue);
                        ASSERT(tmp);

                        LOG_DBG("Got a new element from the fifo, "
                                "enqueuing into user-IPCP");

                        ASSERT(flow->user_ipcp->ops);
                        ASSERT(flow->user_ipcp->ops->sdu_enqueue);
                        if (flow->user_ipcp->ops->
                            sdu_enqueue(flow->user_ipcp->data,
                                        flow->port_id,
                                        tmp)) {
                                LOG_ERR("Couldn't enqueue SDU to user-IPCP");
                                return -1;
                        }
                }

                rfifo_destroy(flow->sdu_queue, (void (*)(void *)) pdu_destroy);
                flow->sdu_queue = NULL;

        } else {
                spin_lock(&data->lock);
                flow->port_id_state = PORT_STATE_NULL;
                spin_unlock(&data->lock);

                app = find_app_by_socket(data, flow->sock);

                /* FIXME: wq cleanup */
                if (flow->fspec_id == 1 &&
                    flow->port_id_state == PORT_STATE_ALLOCATED) {
                        kernel_sock_shutdown(flow->sock, SHUT_RDWR);
                }

                /*
                 * UDP flows on server side use the application socket, so we
                 * don't want to close this socket
                 */
                if (!app)
                        sock_release(flow->sock);

                /*
                 *  If we would destroy the flow, the application
                 *  we refused would constantly try to allocate
                 *  a flow again. This should only be allowed if
                 *  the IPC manager deallocates the NULL state flow first.
                 */
                /* FIXME: This is true for UDP. For TCP too? */
                ASSERT(flow->sdu_queue);
                rfifo_destroy(flow->sdu_queue, (void (*)(void *)) pdu_destroy);
                flow->sdu_queue = NULL;
        }

        return 0;
}

static int tcp_udp_flow_deallocate(struct ipcp_instance_data * data,
                                   port_id_t                   id)
{
        struct shim_tcp_udp_flow * flow;
        struct reg_app_data *      app;
        struct rcv_data *          recvd;
        unsigned long              flags;

        LOG_HBEAT;

        ASSERT(data);

        flow = find_flow_by_port(data, id);
        if (!flow) {
                LOG_ERR("Flow does not exist, cannot remove");
                return -1;
        }

        app = find_app_by_socket(data, flow->sock);

        if ( (flow->fspec_id == 1 || (flow->fspec_id == 0 && !app)) &&
            flow->port_id_state == PORT_STATE_ALLOCATED) {

                /* FIXME: better cleanup (= removing from list) */
                spin_lock_irqsave(&rcv_wq_lock, flags);
                list_for_each_entry(recvd, &rcv_wq_data, list) {
                        if (recvd->sk->sk_socket == flow->sock) {
                                LOG_DBG("Setting socket to NULL");
                                recvd->sk = NULL;
                        }
                }
                spin_unlock_irqrestore(&rcv_wq_lock, flags);

                LOG_DBG("Closing socket");
                kernel_sock_shutdown(flow->sock, SHUT_RDWR);
        }

        if (!app)
                sock_release(flow->sock);

        unbind_and_destroy_flow(data, flow);

        return 0;
}

int recv_msg(struct socket *      sock,
             struct sockaddr_in * other,
             int                  lother,
             unsigned char *      buf,
             int                  len)
{
        struct msghdr msg;
        struct kvec   iov;
        int           size;

        iov.iov_base = buf;
        iov.iov_len  = len;

        msg.msg_control    = NULL;
        msg.msg_controllen = 0;
        msg.msg_flags      = MSG_DONTWAIT;
        msg.msg_name       = other;
        msg.msg_namelen    = lother;

        size = kernel_recvmsg(sock, &msg, &iov, 1, len, msg.msg_flags);
        if (size < 0) {
                if (size == -EAGAIN) {
                        LOG_DBG("I should try receiving the message again");
                } else if (size == -EWOULDBLOCK) {
                        LOG_DBG("Operation would block ...");
                } else {
                        LOG_ERR("Problems receiving message (%d)", size);
                }
        } else {
                LOG_DBG("Received message is %d byte(s) long", size);
        }

        return size;
}

int send_msg(struct socket *      sock,
             struct sockaddr_in * other,
             int                  lother,
             char *               buf,
             int                  len)
{
        struct msghdr msg;
        struct kvec   iov;
        int           size;

        iov.iov_base = buf;
        iov.iov_len  = len;

        msg.msg_control    = NULL;
        msg.msg_controllen = 0;
        msg.msg_flags      = 0;
        msg.msg_name       = other;
        msg.msg_namelen    = lother;

        size = kernel_sendmsg(sock, &msg, &iov, 1, len);
        if (size > 0)
                LOG_DBG("Sent message with %d bytes", size);
        else
                LOG_ERR("Problems sending message");

        return size;
}

static int udp_process_msg(struct ipcp_instance_data * data,
                           struct socket *             sock)
{
        struct shim_tcp_udp_flow *  flow;
        struct sockaddr_in          addr;
        struct reg_app_data *       app;
        struct buffer *             sdubuf;
        struct name *               sname;
        struct sdu *                du;
        char *                      buf;
        int                         size;
        struct ipcp_instance      * ipcp, * user_ipcp;

        LOG_HBEAT;

        buf = rkmalloc(CONFIG_RINA_SHIM_TCP_UDP_BUFFER_SIZE, GFP_KERNEL);
        if(!buf)
                return -1;

        memset(&addr, 0, sizeof(struct sockaddr_in));
        if ((size = recv_msg(sock, &addr, sizeof(addr),
                             buf, CONFIG_RINA_SHIM_TCP_UDP_BUFFER_SIZE)) < 0) {
                if (size != -EAGAIN)
                        LOG_ERR("Error during UDP recv: %d", size);
                rkfree(buf);
                return -1;
        }

        sdubuf = buffer_create_from(buf, size);
        if (!sdubuf) {
                LOG_ERR("Could not create buffer");
                rkfree(buf);
                return -1;
        }

        rkfree(buf);

        du = sdu_create_buffer_with(sdubuf);
        if (!du) {
                LOG_ERR("Couldn't create sdu");
                buffer_destroy(sdubuf);
                return -1;
        }

        spin_lock(&data->lock);
        flow = find_udp_flow(data, &addr, sock);
        if (!flow) {
                spin_unlock(&data->lock);
                LOG_DBG("No flow found, creating it");

                app = find_app_by_socket(data, sock);
                if (!app) {
                        LOG_ERR("No app registered yet! "
                                "Someone is doing something bad "
                                "on the network");
                        sdu_destroy(du);
                        return -1;
                }

                user_ipcp = kipcm_find_ipcp_by_name(default_kipcm,
                                                    app->app_name);

                ASSERT(data);

                if (!user_ipcp)
                        user_ipcp = kfa_ipcp_instance(data->kfa);
                ipcp = kipcm_find_ipcp(default_kipcm, data->id);
                if (!user_ipcp || !ipcp) {
                        LOG_ERR("Could not find required ipcps");
                        sdu_destroy(du);
                        return -1;
                }

                ASSERT(user_ipcp);
                ASSERT(ipcp);

                flow = rkzalloc(sizeof(*flow), GFP_ATOMIC);
                if (!flow) {
                        LOG_ERR("Could not allocate flow");
                        sdu_destroy(du);
                        return -1;
                }

                flow->port_id_state = PORT_STATE_PENDING;
                flow->port_id       = kfa_port_id_reserve(data->kfa, data->id);
                flow->user_ipcp     = user_ipcp;
                flow->sock          = sock;
                flow->fspec_id      = 0;

                flow->addr.sin_port        = addr.sin_port;
                flow->addr.sin_family      = addr.sin_family;
                flow->addr.sin_addr.s_addr = addr.sin_addr.s_addr;

                if (!is_port_id_ok(flow->port_id)) {
                        LOG_ERR("Port id is not ok");
                        sdu_destroy(du);
                        if (flow_destroy(data, flow))
                                LOG_ERR("Problems destroying flow");
                        return -1;
                }

                spin_lock(&data->lock);
                INIT_LIST_HEAD(&flow->list);
                list_add(&flow->list, &data->flows);
                spin_unlock(&data->lock);
                LOG_DBG("Added UDP flow");

                if (!user_ipcp->ops->ipcp_name(user_ipcp->data)) {
                        LOG_DBG("This flow goes for an app");
                        if (kfa_flow_create(data->kfa, flow->port_id, ipcp)) {
                                LOG_ERR("Could not create flow in KFA");
                                sdu_destroy(du);
                                kfa_port_id_release(data->kfa, flow->port_id);
                                if (flow_destroy(data, flow))
                                        LOG_ERR("Problems destroying flow");
                                return -1;
                        }
                }

                flow->sdu_queue = rfifo_create();
                if (!flow->sdu_queue) {
                        flow->port_id_state = PORT_STATE_NULL;

                        LOG_ERR("Couldn't create the sdu queue "
                                "for a new flow");

                        sdu_destroy(du);
                        kfa_port_id_release(data->kfa, flow->port_id);
                        unbind_and_destroy_flow(data, flow);
                        return -1;
                }

                LOG_DBG("Flow's SDU queue created");

                /* Store SDU in queue */

                LOG_DBG("Queueing frame in SDU queue");
                if (rfifo_push(flow->sdu_queue, du)) {
                        flow->port_id_state = PORT_STATE_NULL;

                        LOG_ERR("Could not write %zd bytes into the fifo",
                                sizeof(struct sdu *));

                        sdu_destroy(du);
                        kfa_port_id_release(data->kfa, flow->port_id);
                        unbind_and_destroy_flow(data, flow);
                        return -1;
                }

                /* FIXME: This sets the name to the server? */
                sname = name_create_ni();
                if (!name_init_from_ni(sname, "Unknown app", "", "", "")) {
                        name_destroy(sname);
                        kfa_port_id_release(data->kfa, flow->port_id);
                        unbind_and_destroy_flow(data, flow);
                        return -1;
                }

                if (kipcm_flow_arrived(default_kipcm,
                                       data->id,
                                       flow->port_id,
                                       data->dif_name,
                                       sname,
                                       app->app_name,
                                       data->qos[CUBE_UNRELIABLE])) {
                        LOG_ERR("Couldn't tell the KIPCM about the flow");
                        kfa_port_id_release(data->kfa, flow->port_id);
                        unbind_and_destroy_flow(data, flow);
                        name_destroy(sname);
                        return -1;
                }
                name_destroy(sname);
        } else {
                LOG_DBG("Flow exists, handling SDU");

                if (flow->port_id_state == PORT_STATE_ALLOCATED) {
                        spin_unlock(&data->lock);

                        LOG_DBG("Port is ALLOCATED, "
                                "queueing frame in user-IPCP");

                        ASSERT(flow->user_ipcp);
                        ASSERT(flow->user_ipcp->ops);
                        ASSERT(flow->user_ipcp->ops->sdu_enqueue);

                        if (flow->user_ipcp->ops->
                            sdu_enqueue(flow->user_ipcp->data,
                                        flow->port_id,
                                        du)) {
                                LOG_ERR("Couldn't enqueue SDU to user IPCP");
                                return -1;
                        }
                } else if (flow->port_id_state == PORT_STATE_PENDING) {
                        LOG_DBG("Port is PENDING, "
                                "queueing frame in SDU queue");

                        if (rfifo_push(flow->sdu_queue, du)) {
                                spin_unlock(&data->lock);

                                LOG_ERR("Failed to write %zd bytes"
                                        "into the fifo",
                                        sizeof(struct sdu *));

                                sdu_destroy(du);
                                return -1;
                        }

                        spin_unlock(&data->lock);
                } else if (flow->port_id_state == PORT_STATE_NULL) {
                        spin_unlock(&data->lock);

                        LOG_DBG("Port is NULL, "
                                "dropping SDU");

                        sdu_destroy(du);
                } else {
                        spin_unlock(&data->lock);
                        LOG_ERR("Port state is unhandled");
                }
        }

        return size;
}

static int tcp_recv_new_message(struct ipcp_instance_data * data,
                                struct socket *             sock,
                                struct shim_tcp_udp_flow *  flow)
{
        struct buffer * sdubuf;
        struct sdu *    du;
        char *          buf;
        char            sbuf[2];
        int             size;
        __be16          nlen;

        LOG_HBEAT;

        size = recv_msg(sock, NULL, 0, sbuf, 2);
        if (size <= 0) {
                return size;
        }

        if (size != 2) {
                LOG_DBG("Didn't read both bytes for size %d", size);

                /*
                 * Shim can't function correct when only one size byte is read,
                 * loop till the second byte is received
                 */
                size = -EAGAIN;
                while (size == -EAGAIN) {
                        size = recv_msg(sock, NULL, 0, &sbuf[1], 1);
                        if (size != -EAGAIN && size <= 0) {
                                LOG_ERR("Can't read 2nd size byte %d", size);
                                return size;
                        }
                }
        }

        memcpy(&nlen, &sbuf[0], 2);
        flow->bytes_left = (int) ntohs(nlen);
        LOG_DBG("Incoming message is %d bytes long", flow->bytes_left);

        buf = rkmalloc(flow->bytes_left, GFP_KERNEL);
        if(!buf)
                return -1; /* FIXME: Check this return value */

        size = recv_msg(sock, NULL, 0, buf, flow->bytes_left);
        if (size <= 0) {
                if (size != -EAGAIN) {
                        LOG_ERR("Error during TCP receive (%d)", size);
                        rkfree(buf);
                }
                return size;
        }

        if (size == flow->bytes_left) {
                flow->bytes_left = 0;
                sdubuf = buffer_create_with(buf, size);
                if (!sdubuf) {
                        LOG_ERR("Could not create buffer");
                        rkfree(buf);
                        return -1;
                }

                du = sdu_create_buffer_with(sdubuf);
                if (!du) {
                        LOG_ERR("Couldn't create sdu");
                        buffer_destroy(sdubuf);
                        return -1;
                }

                spin_lock(&data->lock);
                if (flow->port_id_state == PORT_STATE_ALLOCATED) {
                        spin_unlock(&data->lock);

                        ASSERT(flow->user_ipcp);
                        ASSERT(flow->user_ipcp->ops);
                        ASSERT(flow->user_ipcp->ops->sdu_enqueue);

                        if (flow->user_ipcp->ops->
                            sdu_enqueue(flow->user_ipcp->data,
                                        flow->port_id,
                                        du)) {
                                LOG_ERR("Couldn't enqueue SDU to user IPCP");
                                return -1;
                        }
                } else if (flow->port_id_state == PORT_STATE_PENDING) {
                        LOG_DBG("Port is PENDING, "
                                "queueing frame in SDU queue");

                        if (rfifo_push_ni(flow->sdu_queue, du)) {
                                spin_unlock(&data->lock);

                                LOG_ERR("Failed to write %zd bytes"
                                        "into the fifo",
                                        sizeof(struct sdu *));

                                sdu_destroy(du);
                                return -1;
                        }

                        spin_unlock(&data->lock);
                } else if (flow->port_id_state == PORT_STATE_NULL) {
                        spin_unlock(&data->lock);
                        sdu_destroy(du);
                }

                return size;
        } else {
                LOG_DBG("Didn't receive complete message");

                flow->buf = buf;
                flow->lbuf = flow->bytes_left;
                flow->bytes_left = flow->bytes_left - size;

                return -1;
        }
}

static int tcp_recv_partial_message(struct ipcp_instance_data * data,
                                    struct socket *             sock,
                                    struct shim_tcp_udp_flow *  flow)
{
        struct buffer * sdubuf;
        struct sdu *    du;
        int             start, size;

        LOG_HBEAT;

        start = flow->lbuf - flow->bytes_left;

        size = recv_msg(sock, NULL, 0, &flow->buf[start], flow->bytes_left);
        if (size <= 0) {
                if (size != -EAGAIN)
                        LOG_ERR("Error during TCP receive (%d)", size);
                return size;
        }

        if (size == flow->bytes_left) {
                flow->bytes_left = 0;
                sdubuf = buffer_create_with(flow->buf, flow->lbuf);
                if (!sdubuf) {
                        rkfree(flow->buf);
                        LOG_ERR("Could not create buffer");
                        return -1;
                }

                du = sdu_create_buffer_with(sdubuf);
                if (!du) {
                        LOG_ERR("Couldn't create sdu");
                        buffer_destroy(sdubuf);
                        return -1;
                }

                spin_lock(&data->lock);
                if (flow->port_id_state == PORT_STATE_ALLOCATED) {
                        spin_unlock(&data->lock);

                        ASSERT(flow->user_ipcp);
                        ASSERT(flow->user_ipcp->ops);
                        ASSERT(flow->user_ipcp->ops->sdu_enqueue);

                        if (flow->user_ipcp->ops->
                            sdu_enqueue(flow->user_ipcp->data,
                                        flow->port_id,
                                        du)) {
                                LOG_ERR("Couldn't enqueue SDU to user IPCP");
                                return -1;
                        }
                } else if (flow->port_id_state == PORT_STATE_PENDING) {
                        LOG_DBG("Port is PENDING, "
                                "queueing frame in SDU queue");

                        if (rfifo_push_ni(flow->sdu_queue, du)) {
                                spin_unlock(&data->lock);

                                LOG_ERR("Failed to write %zd bytes"
                                        "into the fifo",
                                        sizeof(struct sdu *));

                                sdu_destroy(du);
                                return -1;
                        }

                        spin_unlock(&data->lock);
                } else if (flow->port_id_state == PORT_STATE_NULL) {
                        spin_unlock(&data->lock);
                        sdu_destroy(du);
                }

                return size;
        } else {
                LOG_DBG("Still didn't receive complete message");

                flow->bytes_left = flow->bytes_left - size;

                return -1;
        }
}

static int tcp_process_msg(struct ipcp_instance_data * data,
                           struct socket *             sock)
{
        struct shim_tcp_udp_flow * flow;
        struct rcv_data *          recvd;
        unsigned long              flags;
        int                        size;

        LOG_HBEAT;

        ASSERT(data);
        ASSERT(sock);

        flow = find_flow_by_socket(data, sock);
        if (!flow) {
                LOG_ERR("Cannot find the flow (data = %pK, socket = %pK)",
                        data, sock);
                return -1;
        }

        if (flow->bytes_left == 0)
                size = tcp_recv_new_message(data, sock, flow);
        else
                size = tcp_recv_partial_message(data, sock, flow);

        if (size == 0 && (flow->port_id_state == PORT_STATE_ALLOCATED ||
                          flow->port_id_state == PORT_STATE_PENDING)) {
                LOG_DBG("Got 0 size message, closing flow");

                if (flow->port_id_state == PORT_STATE_ALLOCATED) {
                        LOG_DBG("Port was ALLOCATED, moving to NULL");
                        flow->port_id_state = PORT_STATE_NULL;
                } else {
                        LOG_DBG("Port was PENDING");
                }

                write_lock_bh(&flow->sock->sk->sk_callback_lock);
                flow->sock->sk->sk_data_ready = flow->sock->sk->sk_user_data;
                flow->sock->sk->sk_user_data  = NULL;
                write_unlock_bh(&flow->sock->sk->sk_callback_lock);

                /* FIXME: better cleanup */
                spin_lock_irqsave(&rcv_wq_lock, flags);
                list_for_each_entry(recvd, &rcv_wq_data, list) {
                        if (recvd->sk->sk_socket == flow->sock)
                                recvd->sk = NULL;
                }
                spin_unlock_irqrestore(&rcv_wq_lock, flags);

                sock_release(flow->sock);

                /* FIXME: remove the msleep */
                while (flow->sdu_queue != NULL) {
                        LOG_DBG("Waiting the SDU queue to become empty ...");
                        msleep(2);
                }

                LOG_DBG("Notifying KIPCM about deallocate");
                kipcm_notify_flow_dealloc(data->id, 0, flow->port_id, 1);
                kfa_port_id_release(data->kfa, flow->port_id);
                unbind_and_destroy_flow(data, flow);

                return 0;
        }

        return size;
}

static int tcp_process(struct ipcp_instance_data * data, struct socket * sock)
{
        struct shim_tcp_udp_flow * flow;
        struct reg_app_data *      app;
        struct socket *            acsock;
        struct name *              sname;
        int                        err;
        struct ipcp_instance     * ipcp, * user_ipcp;

        LOG_HBEAT;

        ASSERT(sock);

        LOG_DBG("Processing TCP socket %pK", sock);

        app = find_app_by_socket(data, sock);
        if (!app) {
                /* connection exists */
                err = tcp_process_msg(data, sock);
                while (err > 0)
                        err = tcp_process_msg(data, sock);
                return err;
        } else {
                /* accept connection */
                err = kernel_accept(app->tcpsock, &acsock, O_NONBLOCK);
                if (err < 0) {
                        LOG_ERR("Could not accept socket");
                        return -1;
                }
                LOG_DBG("Socket accepted");

                write_lock_bh(&acsock->sk->sk_callback_lock);
                acsock->sk->sk_user_data  = acsock->sk->sk_data_ready;
                acsock->sk->sk_data_ready = tcp_udp_rcv;
                write_unlock_bh(&acsock->sk->sk_callback_lock);

                flow = rkzalloc(sizeof(*flow), GFP_KERNEL);
                if (!flow) {
                        LOG_ERR("Could not allocate flow");

                        sock_release(acsock);
                        return -1;
                }

                user_ipcp = kipcm_find_ipcp_by_name(default_kipcm,
                                                    app->app_name);
                if (!user_ipcp)
                        user_ipcp = kfa_ipcp_instance(data->kfa);
                ASSERT(user_ipcp);

                ipcp = kipcm_find_ipcp(default_kipcm, data->id);

                flow->port_id_state = PORT_STATE_PENDING;
                flow->fspec_id      = 1;
                flow->port_id       = kfa_port_id_reserve(data->kfa, data->id);
                flow->sock          = acsock;

                spin_lock(&data->lock);
                INIT_LIST_HEAD(&flow->list);
                list_add(&flow->list, &data->flows);
                spin_unlock(&data->lock);
                LOG_DBG("TCP flow added");

                memset(&flow->addr, 0, sizeof(struct sockaddr_in));

                if (!is_port_id_ok(flow->port_id)) {
                        flow->port_id_state = PORT_STATE_NULL;
                        LOG_ERR("Port id is not ok");

                        sock_release(acsock);
                        if (flow_destroy(data, flow))
                                LOG_ERR("Problems destroying flow");

                        return -1;
                }
                LOG_DBG("Added flow to the list");

                if (!user_ipcp->ops->ipcp_name(user_ipcp->data)) {
                        LOG_DBG("This flow goes for an app");
                        if (kfa_flow_create(data->kfa, flow->port_id, ipcp)) {
                                LOG_ERR("Could not create flow in KFA");
                                kfa_port_id_release(data->kfa, flow->port_id);
                                if (flow_destroy(data, flow))
                                        LOG_ERR("Problems destroying flow");
                                return -1;
                        }
                }

                flow->sdu_queue = rfifo_create_ni();
                if (!flow->sdu_queue) {
                        LOG_ERR("Couldn't create the sdu queue "
                                "for a new flow");
                        kfa_port_id_release(data->kfa, flow->port_id);
                        tcp_unbind_and_destroy_flow(data, flow);
                        return -1;
                }

                LOG_DBG("Queue has been created");

                sname = name_create_ni();
                if (!name_init_from_ni(sname, "Unknown app", "", "", "")) {
                        name_destroy(sname);
                        kfa_port_id_release(data->kfa, flow->port_id);
                        tcp_unbind_and_destroy_flow(data, flow);
                        return -1;
                }

                if (kipcm_flow_arrived(default_kipcm,
                                       data->id,
                                       flow->port_id,
                                       data->dif_name,
                                       sname,
                                       app->app_name,
                                       data->qos[CUBE_RELIABLE])) {
                        LOG_ERR("Couldn't tell the KIPCM about the flow");
                        kfa_port_id_release(data->kfa, flow->port_id);
                        tcp_unbind_and_destroy_flow(data, flow);
                        name_destroy(sname);
                        return -1;
                }

                name_destroy(sname);
                LOG_DBG("TCP flow created");

                return 0;
        }
}

static int tcp_udp_rcv_process_msg(struct sock * sk)
{
        struct host_ipcp_instance_mapping * mapping;
        struct sockaddr_in                  own;
        struct socket *                     sock;
        int                                 res, len;

        LOG_HBEAT;

        ASSERT(sk);

        sock = sk->sk_socket;
        ASSERT(sock);

        len = sizeof(struct sockaddr_in);
        if (kernel_getsockname(sock, (struct sockaddr*) &own, &len)) {
                LOG_ERR("Couldn't retrieve hostname");
                return -1;
        }
        LOG_DBG("Found sockname (%d)", ntohl(own.sin_addr.s_addr));

        mapping = inst_data_mapping_get(ntohl(own.sin_addr.s_addr));
        ASSERT(mapping);

        if (sk->sk_socket->type == SOCK_DGRAM) {
                res = udp_process_msg(mapping->data, sock);
                while (res > 0)
                        res = udp_process_msg(mapping->data, sock);
                return res;
        } else
                return tcp_process(mapping->data, sock);
}

static void tcp_udp_rcv_worker(struct work_struct * work)
{
        struct rcv_data * recvd, * next;
        unsigned long     flags;

        LOG_HBEAT;

        /* FIXME: more efficient locking and better cleanup */
        spin_lock_irqsave(&rcv_wq_lock, flags);
        list_for_each_entry_safe(recvd, next, &rcv_wq_data, list) {
                list_del(&recvd->list);
                spin_unlock_irqrestore(&rcv_wq_lock, flags);

                LOG_DBG("Worker on %pK", recvd->sk);

                if (recvd->sk != NULL) {
                        int res = tcp_udp_rcv_process_msg(recvd->sk);
                        if (res <= 0)
                                LOG_DBG("TCP/UDP processing returned %d", res);
                }

                rkfree(recvd);

                spin_lock_irqsave(&rcv_wq_lock, flags);
        }
        spin_unlock_irqrestore(&rcv_wq_lock, flags);

        LOG_DBG("Worker finished for now");
}

static int tcp_udp_application_register(struct ipcp_instance_data * data,
                                        const struct name *         name)
{
        struct reg_app_data * app;
        struct sockaddr_in    sin;
        struct exp_reg *      exp_reg;
        int                   err;

        LOG_HBEAT;

        ASSERT(data);
        ASSERT(name);

        LOG_DBG("Handling application registration");

        app = find_app_by_name(data, name);
        if (app) {
                LOG_ERR("Application is already registered");
                return -1;
        }

        app = rkzalloc(sizeof(struct reg_app_data), GFP_KERNEL);
        if (!app) {
                LOG_ERR("Could not allocate mem for application data");
                return -1;
        }

        app->app_name = name_dup(name);
        if (!app->app_name) {
                LOG_ERR("Application registration has failed");
                rkfree(app);
                return -1;
        }

        exp_reg = find_exp_reg(data, name);
        if (!exp_reg) {
                LOG_ERR("That application is not expected to register");
                rkfree(app);
                return -1;
        }
        LOG_DBG("Application found in exp_reg");

        app->port = exp_reg->port;

        err = sock_create_kern(PF_INET, SOCK_DGRAM, IPPROTO_UDP,
                               &app->udpsock);
        if (err < 0) {
                LOG_ERR("Could not create UDP socket for registration");
                name_destroy(app->app_name);
                rkfree(app);
                return -1;
        }

        sin.sin_addr.s_addr = htonl(data->host_name);
        sin.sin_family      = AF_INET;
        sin.sin_port        = htons(app->port);

        err = kernel_bind(app->udpsock, (struct sockaddr*) &sin, sizeof(sin));
        if (err < 0) {
                LOG_ERR("Could not bind UDP socket for registration");
                sock_release(app->udpsock);
                name_destroy(app->app_name);
                rkfree(app);
                return -1;
        }

        write_lock_bh(&app->udpsock->sk->sk_callback_lock);
        app->udpsock->sk->sk_user_data = app->udpsock->sk->sk_data_ready;
        app->udpsock->sk->sk_data_ready = tcp_udp_rcv;
        write_unlock_bh(&app->udpsock->sk->sk_callback_lock);

        LOG_DBG("UDP socket ready");

        err = sock_create_kern(PF_INET, SOCK_STREAM, IPPROTO_TCP,
                               &app->tcpsock);
        if (err < 0) {
                LOG_ERR("could not create TCP socket for registration");
                sock_release(app->udpsock);
                name_destroy(app->app_name);
                rkfree(app);
                return -1;
        }

        err = kernel_bind(app->tcpsock, (struct sockaddr*) &sin, sizeof(sin));
        if (err < 0) {
                LOG_ERR("Could not bind TCP socket for registration");
                sock_release(app->tcpsock);
                sock_release(app->udpsock);
                name_destroy(app->app_name);
                rkfree(app);
                return -1;
        }

        err = kernel_listen(app->tcpsock, 5);
        if (err < 0) {
                LOG_ERR("Could not listen on TCP socket for registration");
                sock_release(app->tcpsock);
                sock_release(app->udpsock);
                name_destroy(app->app_name);
                rkfree(app);
                return -1;
        }

        write_lock_bh(&app->tcpsock->sk->sk_callback_lock);
        app->tcpsock->sk->sk_user_data  = app->tcpsock->sk->sk_data_ready;
        app->tcpsock->sk->sk_data_ready = tcp_udp_rcv;
        write_unlock_bh(&app->tcpsock->sk->sk_callback_lock);

        LOG_DBG("TCP socket ready");

        INIT_LIST_HEAD(&(app->list));

        spin_lock(&data->lock);
        list_add(&(app->list), &(data->reg_apps));
        spin_unlock(&data->lock);

        return 0;
}

static int tcp_udp_application_unregister(struct ipcp_instance_data * data,
                                          const struct name *         name)
{
        struct reg_app_data * app;

        LOG_HBEAT;

        ASSERT(data);
        ASSERT(name);

        LOG_DBG("Handling application un-registration");

        app = find_app_by_name(data, name);
        if (!app) {
                LOG_ERR("Application is not registered, so can't unregister");
                return -1;
        }

        lock_sock(app->udpsock->sk);
        write_lock_bh(&app->udpsock->sk->sk_callback_lock);
        app->udpsock->sk->sk_data_ready = app->udpsock->sk->sk_user_data;
        app->udpsock->sk->sk_user_data  = NULL;
        write_unlock_bh(&app->udpsock->sk->sk_callback_lock);
        release_sock(app->udpsock->sk);

        sock_release(app->udpsock);

        LOG_DBG("UDP socket destroyed");

        lock_sock(app->tcpsock->sk);
        write_lock_bh(&app->tcpsock->sk->sk_callback_lock);
        app->tcpsock->sk->sk_data_ready = app->tcpsock->sk->sk_user_data;
        app->tcpsock->sk->sk_user_data  = NULL;
        write_unlock_bh(&app->tcpsock->sk->sk_callback_lock);
        release_sock(app->tcpsock->sk);

        kernel_sock_shutdown(app->tcpsock, SHUT_RDWR);
        sock_release(app->tcpsock);

        LOG_DBG("TCP socket destroyed");

        name_destroy(app->app_name);

        spin_lock(&data->lock);
        list_del(&app->list);
        spin_unlock(&data->lock);

        rkfree(app);

        return 0;
}

static int get_nxt_len(char ** enc,
                       int *   len)
{
        char * tmp;

        ASSERT(enc);

        tmp = strsep(enc, ":");
        if (!enc) {
                LOG_ERR("No separator found!");
                return -1;
        }

        if (kstrtouint(tmp, 10, len)) {
                LOG_ERR("Failed to convert int");
                return -1;
        }

        return 0;
}

static int eat_substr(char ** dst,
                      char ** src,
                      int *   len)
{
        ASSERT(len);
        ASSERT(dst);
        ASSERT(src);

        *dst = rkmalloc((*len) + 1, GFP_KERNEL);
        if (!*dst) {
                LOG_ERR("Failed to get memory");
                return -1;
        }

        memcpy(*dst, *src, *len);
        (*dst)[*len] = '\0';
        *src += *len;

        return 0;
}

static int get_nxt_val(char ** dst,
                       char ** val,
                       int  *  len)
{
        ASSERT(len);
        ASSERT(dst);
        ASSERT(val);

        if (get_nxt_len(val, len)) {
                LOG_ERR("get_nxt_len failed");
                return -1;
        }

        if (eat_substr(dst, val, len)) {
                LOG_ERR("eat_substr failed");
                return -1;
        }

        return 0;
}

/* Note: Assumes an IPv4 address */
static int ip_string_to_int(char *   ip_s,
                            __be32 * ip_a)
{
        char *       tmp;
        unsigned int nr, i;

        ASSERT(ip_s);
        ASSERT(ip_a);

        *ip_a = 0;
        nr    = 0;
        for (i = 0; i < 4; i++) {
                tmp = strsep(&ip_s, ".");
                if (kstrtouint(tmp, 10, &nr)) {
                        LOG_ERR("Failed to convert int");
                        return -1;
                }
                *ip_a |= nr << (8 * (3-i));
        }

        return 0;
}

static void clear_directory(struct ipcp_instance_data * data)
{
        struct dir_entry * entry, * next;

        LOG_HBEAT;

        ASSERT(data);

        list_for_each_entry_safe(entry, next, &data->directory, list) {
                list_del(&entry->list);
                name_destroy(entry->app_name);
                rkfree(entry);
        }
}

static void clear_exp_reg(struct ipcp_instance_data * data)
{
        struct exp_reg * entry, * next;

        ASSERT(data);

        list_for_each_entry_safe(entry, next, &data->exp_regs, list) {
                list_del(&entry->list);
                name_destroy(entry->app_name);
                rkfree(entry);
        }
}

static void undo_assignment(struct ipcp_instance_data * data)
{
        LOG_HBEAT;

        clear_directory(data);
        clear_exp_reg(data);
}

static int parse_dir_entry(struct ipcp_instance_data * data, char **blob)
{
        unsigned int       len, port_nr;
        __be32             ip_addr;
        char               * ap, * ae, * ip, * port;
        struct name *      app_name;
        struct dir_entry * dir_entry;

        LOG_HBEAT;

        ASSERT(*blob);

        dir_entry = rkmalloc(sizeof(struct dir_entry), GFP_KERNEL);
        if (!dir_entry)
                return -1;

        /* len:aplen:aelen:iplen:port */
        /* Get AP name */
        len = 0;
        ap  = 0;
        if (get_nxt_val(&ap, blob, &len)) {
                LOG_ERR("Failed to get next value");
                rkfree(dir_entry);
                return -1;
        }

        /* Get AE name */
        ae = 0;
        if (get_nxt_val(&ae, blob, &len)) {
                LOG_ERR("Failed to get next value");
                rkfree(ap);
                rkfree(dir_entry);
                return -1;
        }

        /* Get IP address */
        ip = 0;
        if (get_nxt_val(&ip, blob, &len)) {
                LOG_ERR("Failed to get next value");
                rkfree(ae);
                rkfree(ap);
                rkfree(dir_entry);
                return -1;
        }

        if (ip_string_to_int(ip, &ip_addr)) {
                LOG_ERR("Failed to convert ip to int");
                rkfree(ip);
                rkfree(ae);
                rkfree(ap);
                rkfree(dir_entry);
                return -1;
        }
        rkfree(ip);

        /* Get port number */
        port = 0;
        if (get_nxt_val(&port, blob, &len)) {
                LOG_ERR("Failed to get next value");
                rkfree(ae);
                rkfree(ap);
                rkfree(dir_entry);
                return -1;
        }

        if (kstrtouint(port, 10, &port_nr)) {
                LOG_ERR("Failed to convert int");
                rkfree(port);
                rkfree(ae);
                rkfree(ap);
                rkfree(dir_entry);
                return -1;
        }
        rkfree(port);

        app_name = name_create();
        if (!name_init_with(app_name,
                            ap, rkstrdup_ni(""),
                            ae, rkstrdup_ni(""))) {
                LOG_ERR("Failed to init name");
                rkfree(ae);
                rkfree(ap);
                rkfree(dir_entry);
                return -1;
        }

        INIT_LIST_HEAD(&dir_entry->list);
        dir_entry->app_name   = app_name;
        dir_entry->ip_address = ip_addr;
        dir_entry->port       = port_nr;

        spin_lock(&data->lock);
        list_add(&dir_entry->list, &data->directory);
        spin_unlock(&data->lock);

        LOG_DBG("Added a new dir entry");

        return 0;
}

static int parse_exp_reg_entry(struct ipcp_instance_data * data, char ** blob)
{
        struct exp_reg * exp_reg;
        unsigned int     len, port_nr;
        char           * ap, * ae, * port;
        struct name *    app_name;

        ASSERT(*blob);

        exp_reg = rkmalloc(sizeof(struct exp_reg), GFP_KERNEL);
        if (!exp_reg)
                return -1;

        /* len:aplen:aelen:port */
        /* Get AP name */
        len = 0;
        ap  = 0;
        if (get_nxt_val(&ap, blob, &len)) {
                LOG_ERR("Failed to get next value");
                rkfree(exp_reg);
                return -1;
        }

        /* Get AE name */
        ae = 0;
        if (get_nxt_val(&ae, blob, &len)) {
                LOG_ERR("Failed to get next value");
                rkfree(ap);
                rkfree(exp_reg);
                return -1;
        }

        /* Get port number */
        port = 0;
        if (get_nxt_val(&port, blob, &len)) {
                LOG_ERR("Failed to get next value");
                rkfree(ae);
                rkfree(ap);
                rkfree(exp_reg);
                return -1;
        }

        if (kstrtouint(port, 10, &port_nr)) {
                LOG_ERR("Failed to convert int");
                rkfree(port);
                rkfree(ae);
                rkfree(ap);
                rkfree(exp_reg);
                return -1;
        }
        rkfree(port);

        INIT_LIST_HEAD(&exp_reg->list);
        app_name = name_create();
        if (!name_init_with(app_name,
                            ap, rkstrdup_ni(""),
                            ae, rkstrdup_ni(""))) {
                LOG_ERR("Failed to init name");
                rkfree(ae);
                rkfree(ap);
                rkfree(exp_reg);
                return -1;
        }
        exp_reg->app_name = app_name;
        exp_reg->port     = port_nr;

        spin_lock(&data->lock);
        list_add(&exp_reg->list, &data->exp_regs);
        spin_unlock(&data->lock);

        LOG_DBG("Added a new exp reg entry");

        return 0;
}

static int parse_assign_conf(struct ipcp_instance_data * data,
                             const struct dif_config *   config)
{
        struct ipcp_config * tmp;

        LOG_HBEAT;

        ASSERT(data);
        ASSERT(config);

        list_for_each_entry(tmp, &(config->ipcp_config_entries), next) {
                const struct ipcp_config_entry * entry = tmp->entry;
                /*
                 *  Want to get an interface name here
                 *  Directory entries and exp registrations
                 */
                if (!strcmp(entry->name, "hostname")) {
                        char * copy;
                        __be32 ip_addr;

                        ASSERT(entry->value);

                        copy = rkstrdup_ni(entry->value);
                        if (!copy) {
                                LOG_ERR("Failed to dup value");
                                return -1;
                        }

                        ip_addr = 0;
                        if (ip_string_to_int(copy, &ip_addr)) {
                                LOG_ERR("Failed to convert ip to int");
                                rkfree(copy);
                                return -1;
                        }

                        data->host_name = ip_addr;

                        rkfree(copy);

                        LOG_DBG("Got hostname %u", data->host_name);
                } else if (!strcmp(entry->name, "dirEntry")) {
                        int  count;
                        char * val, * copy;

                        ASSERT(entry->value);

                        copy = rkstrdup_ni(entry->value);
                        if (!copy) {
                                LOG_ERR("Failed to dup value");
                                return -1;
                        }
                        val = copy;

                        if (get_nxt_len(&val, &count)) {
                                LOG_ERR("Failed to get number of objects");
                                rkfree(copy);
                                return -1;
                        }

                        while (count-- > 0) {
                                if (parse_dir_entry(data, &val) < 0) {
                                        rkfree(copy);
                                        return -1;
                                }
                        }

                        rkfree(copy);
                } else if (!strcmp(entry->name, "expReg")) {
                        char * val, * copy;
                        int  count;

                        ASSERT(entry->value);

                        copy = rkstrdup_ni(entry->value);
                        if (!copy) {
                                LOG_ERR("Failed to dup value");
                                return -1;
                        }
                        val = copy;

                        if (get_nxt_len(&val, &count)) {
                                LOG_ERR("Failed to get number of objects");
                                rkfree(copy);
                                return -1;
                        }

                        while (count-- > 0) {
                                if (parse_exp_reg_entry(data, &val) < 0) {
                                        rkfree(copy);
                                        return -1;
                                }
                        }

                        rkfree(copy);
                } else
                        LOG_WARN("Unknown config parameter '%s'", entry->name);
        }

        return 0;
}

static int tcp_udp_assign_to_dif(struct ipcp_instance_data * data,
                                 const struct dif_info *     dif_information)
{
        struct host_ipcp_instance_mapping * mapping;

        LOG_HBEAT;

        ASSERT(data);
        ASSERT(dif_information);

        if (data->dif_name) {
                ASSERT(data->dif_name->process_name);

                LOG_ERR("This IPC Process is already assigned to the DIF %s",
                        data->dif_name->process_name);
                return -1;
        }

        data->dif_name = name_dup(dif_information->dif_name);
        if (!data->dif_name) {
                LOG_ERR("Error duplicating name, bailing out");
                return -1;
        }

        if (parse_assign_conf(data,
                              dif_information->configuration)) {
                LOG_ERR("Failed to parse configuration");
                name_destroy(data->dif_name);
                data->dif_name = NULL;
                undo_assignment(data);
                return -1;
        }

        mapping = rkmalloc(sizeof(struct host_ipcp_instance_mapping),
                           GFP_KERNEL);
        if (!mapping) {
                LOG_ERR("Failed to allocate memory");
                name_destroy(data->dif_name);
                data->dif_name = NULL;
                undo_assignment(data);
                return -1;
        }

        mapping->host_name = data->host_name;
        mapping->data = data;
        INIT_LIST_HEAD(&mapping->list);

        spin_lock(&data_instances_lock);
        list_add(&mapping->list, &data_instances_list);
        spin_unlock(&data_instances_lock);

        return 0;
}

static int tcp_udp_update_dif_config(struct ipcp_instance_data * data,
                                     const struct dif_config *   new_config)
{
        struct host_ipcp_instance_mapping * mapping;

        LOG_HBEAT;

        ASSERT(data);
        ASSERT(new_config);

        /*
         * FIXME: Update configuration here
         *
         * Close all sockets, destroy all flows
         */

        undo_assignment(data);

        mapping = inst_data_mapping_get(data->host_name);
        if (mapping) {
                spin_lock(&data_instances_lock);
                list_del(&mapping->list);
                spin_unlock(&data_instances_lock);
                kfree(mapping);
        }

        if (parse_assign_conf(data, new_config)) {
                LOG_ERR("Failed to update configuration");
                undo_assignment(data);
                /* FIXME: If this fails, DIF is no longer functional */
                return -1;
        }

        mapping = rkmalloc(sizeof(struct host_ipcp_instance_mapping),
                           GFP_KERNEL);
        if (!mapping) {
                LOG_ERR("Failed to allocate memory");
                undo_assignment(data);
                return -1;
        }

        mapping->host_name = data->host_name;
        mapping->data = data;
        INIT_LIST_HEAD(&mapping->list);

        spin_lock(&data_instances_lock);
        list_add(&mapping->list, &data_instances_list);
        spin_unlock(&data_instances_lock);

        return 0;
}

static int tcp_sdu_write(struct shim_tcp_udp_flow * flow,
                         int                        len,
                         char *                     sbuf)
{
        __be16 length;
        int    size, total;
        char * buf;

        LOG_HBEAT;

        ASSERT(flow);
        ASSERT(len);
        ASSERT(sbuf);

        buf = rkmalloc(len + sizeof(__be16), GFP_ATOMIC);
        if (!buf)
                return -1; /* FIXME: Check this return value */

        length = htons((short)len);

        memcpy(&buf[0], &length, sizeof(__be16));
        memcpy(&buf[sizeof(__be16)], &sbuf[0], len);

        total = 0;
        while (total < len + sizeof(__be16)) {
                size = send_msg(flow->sock, NULL, 0, &buf[total],
                                len + sizeof(__be16) - total);
                if (size < 0) {
                        LOG_ERR("error during sdu write (tcp): %d", size);
                        return -1;
                }
                total += size;
        }

        rkfree(buf);

        return 0;
}

static int tcp_udp_sdu_write(struct ipcp_instance_data * data,
                             port_id_t                   id,
                             struct sdu *                sdu)
{
        struct snd_data * snd_data;
        unsigned long     flags;

        LOG_DBG("Callback on tcp_udp_sdu_write");

        snd_data = rkmalloc(sizeof(*snd_data), GFP_ATOMIC);
        if (!snd_data) {
                LOG_ERR("Could not allocate snd_data");
                return -1;
        }

        snd_data->data = data;
        snd_data->id   = id;
        snd_data->sdu  = sdu;
        INIT_LIST_HEAD(&snd_data->list);

        spin_lock_irqsave(&snd_wq_lock, flags);
        list_add_tail(&snd_data->list, &snd_wq_data);
        spin_unlock_irqrestore(&snd_wq_lock, flags);

        queue_work(snd_wq, &snd_work);
        return 0;
}

static int __tcp_udp_sdu_write(struct ipcp_instance_data * data,
                               port_id_t                   id,
                               struct sdu *                sdu)
{
        struct shim_tcp_udp_flow * flow;
        int                        size;

        ASSERT(data);
        ASSERT(sdu);
        ASSERT(is_port_id_ok(id));

        LOG_HBEAT;

        flow = find_flow_by_port(data, id);
        if (!flow) {
                LOG_ERR("Could not find flow with specified port-id");
                sdu_destroy(sdu);
                return -1;
        }

        spin_lock(&data->lock);
        if (flow->port_id_state != PORT_STATE_ALLOCATED) {
                sdu_destroy(sdu);
                spin_unlock(&data->lock);

                LOG_ERR("Flow is not in the right state to call this");

                return -1;
        }
        spin_unlock(&data->lock);

        if (flow->fspec_id == 0) {
                /* We are sending an UDP message */
                size = send_msg(flow->sock, &flow->addr, sizeof(flow->addr),
                                (char*) buffer_data_rw(sdu->buffer),
                                buffer_length(sdu->buffer));
                if (size < 0) {
                        LOG_ERR("Error during SDU write (udp): %d", size);
                        sdu_destroy(sdu);
                        return -1;
                } else if (size < buffer_length(sdu->buffer)) {
                        LOG_ERR("Could not completely send SDU");
                        sdu_destroy(sdu);
                        return -1;
                }
        } else {
                /* We are sending a TCP message */
                if (tcp_sdu_write(flow, buffer_length(sdu->buffer),
                                  (char*) buffer_data_rw(sdu->buffer))) {
                        LOG_ERR("Could not send SDU on TCP flow");
                        sdu_destroy(sdu);
                        return -1;
                }
        }

        LOG_DBG("SDU sent");
        sdu_destroy(sdu);

        return 0;
}

static void tcp_udp_write_worker(struct work_struct * w)
{
        struct snd_data * snd_data, * next;
        unsigned long     flags;

        /* FIXME: more efficient locking and better cleanup */
        spin_lock_irqsave(&snd_wq_lock, flags);
        list_for_each_entry_safe(snd_data, next, &snd_wq_data, list) {
                list_del(&snd_data->list);
                spin_unlock_irqrestore(&snd_wq_lock, flags);

                __tcp_udp_sdu_write(snd_data->data,
                                    snd_data->id,
                                    snd_data->sdu);

                rkfree(snd_data);

                spin_lock_irqsave(&snd_wq_lock, flags);
        }
        spin_unlock_irqrestore(&snd_wq_lock, flags);

        LOG_DBG("Writer worker finished for now");
}

static const struct name * tcp_udp_ipcp_name(struct ipcp_instance_data * data)
{
        LOG_HBEAT;

        ASSERT(data);
        ASSERT(name_is_ok(data->name));

        return data->name;
}

static const struct name * tcp_udp_dif_name(struct ipcp_instance_data * data)
{
        LOG_HBEAT;

        ASSERT(data);
        ASSERT(name_is_ok(data->dif_name));

        return data->dif_name;
}

static int tcp_udp_query_rib(struct ipcp_instance_data * data,
                             struct list_head *          entries,
                             const string_t *            object_class,
                             const string_t *            object_name,
                             uint64_t                    object_instance,
                             uint32_t                    scope,
                             const string_t *            filter)
{
	LOG_MISSING;
	return -1;
}

static struct ipcp_instance_ops tcp_udp_instance_ops = {
        .flow_allocate_request     = tcp_udp_flow_allocate_request,
        .flow_allocate_response    = tcp_udp_flow_allocate_response,
        .flow_deallocate           = tcp_udp_flow_deallocate,
        .flow_prebind              = NULL,
        .flow_binding_ipcp         = NULL,
        .flow_unbinding_ipcp       = NULL,
        .flow_unbinding_user_ipcp  = tcp_udp_unbind_user_ipcp,
	.nm1_flow_state_change	   = NULL,

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

        .pff_add                   = NULL,
        .pff_remove                = NULL,
        .pff_dump                  = NULL,
        .pff_flush                 = NULL,

        .query_rib	           = tcp_udp_query_rib,

        .ipcp_name                 = tcp_udp_ipcp_name,
        .dif_name                  = tcp_udp_dif_name,

        .set_policy_set_param      = NULL,
        .select_policy_set         = NULL,
        .update_crypto_state	   = NULL,
        .dif_name		   = tcp_udp_dif_name
};

static struct ipcp_factory_data {
        spinlock_t lock;
        struct list_head instances;
} tcp_udp_data;

static int tcp_udp_init(struct ipcp_factory_data * data)
{
        LOG_HBEAT;

        ASSERT(data);

        bzero(&tcp_udp_data, sizeof(tcp_udp_data));
        INIT_LIST_HEAD(&(data->instances));

        INIT_LIST_HEAD(&rcv_wq_data);
        INIT_LIST_HEAD(&snd_wq_data);

        INIT_LIST_HEAD(&data_instances_list);

        spin_lock_init(&data->lock);

        INIT_WORK(&rcv_work, tcp_udp_rcv_worker);
        INIT_WORK(&snd_work, tcp_udp_write_worker);

        LOG_INFO("%s initialized", SHIM_NAME);

        return 0;
}

static int tcp_udp_fini(struct ipcp_factory_data * data)
{
        LOG_HBEAT;

        ASSERT(data);

        ASSERT(list_empty(&(data->instances)));

        return 0;
}

static struct ipcp_instance_data *
find_instance(struct ipcp_factory_data * data,
              ipc_process_id_t           id)
{
        struct ipcp_instance_data * pos;

        ASSERT(data);

        spin_lock(&data->lock);

        list_for_each_entry(pos, &(data->instances), list) {
                if (pos->id == id) {
                        return pos;
                }
        }

        spin_unlock(&data->lock);

        return NULL;
}

static void inst_cleanup(struct ipcp_instance * inst)
{
        ASSERT(inst);

        if (inst->data) {
                if (inst->data->qos) {
                        if (inst->data->qos[CUBE_UNRELIABLE])
                                rkfree(inst->data->qos[CUBE_UNRELIABLE]);
                        if (inst->data->qos[CUBE_RELIABLE])
                                rkfree(inst->data->qos[CUBE_RELIABLE]);

                        rkfree(inst->data->qos);
                }
                if (inst->data->name)
                        name_destroy(inst->data->name);

                rkfree(inst->data);
        }

        rkfree(inst);
}

static struct ipcp_instance * tcp_udp_create(struct ipcp_factory_data * data,
                                             const struct name *        name,
                                             ipc_process_id_t           id)
{
        struct ipcp_instance * inst;

        LOG_HBEAT;

        ASSERT(data);

        /* Check if there already is an instance with that id */
        if (find_instance(data,id)) {
                LOG_ERR("There's a shim instance with id %d already", id);
                return NULL;
        }

        /* Create an instance */
        inst = rkzalloc(sizeof(*inst), GFP_KERNEL);
        if (!inst) {
                LOG_ERR("Could not allocate memory for IPCP instance");
                return NULL;
        }

        /* fill it properly */
        inst->ops  = &tcp_udp_instance_ops;

	if (robject_rset_init_and_add(&inst->robj,
				      &shim_tcp_udp_ipcp_rtype,
				      kipcm_rset(default_kipcm),
				      "%u",
				      id)) {
		rkfree(inst);
		return NULL;
	}


        inst->data = rkzalloc(sizeof(struct ipcp_instance_data), GFP_KERNEL);
        if (!inst->data) {
                LOG_ERR("Could not allocate memory for IPCP instance data");
                inst_cleanup(inst);
                return NULL;
        }

        inst->data->id = id;
        inst->data->name = name_dup(name);
        if (!inst->data->name) {
                LOG_ERR("Failed creation of IPC name");
                inst_cleanup(inst);
                return NULL;
        }

        inst->data->qos = rkzalloc(2 * sizeof(struct flow_spec *), GFP_KERNEL);
        if (!inst->data->qos) {
                LOG_ERR("Failed creation of QoS cubes");
                inst_cleanup(inst);
                return NULL;
        }

        inst->data->qos[CUBE_UNRELIABLE] =
                rkzalloc(sizeof(struct flow_spec), GFP_KERNEL);
        if (!inst->data->qos[CUBE_UNRELIABLE]) {
                LOG_ERR("Failed creation of QoS cube %d", CUBE_UNRELIABLE);
                inst_cleanup(inst);
                return NULL;
        }

        inst->data->qos[CUBE_RELIABLE] =
                rkzalloc(sizeof(struct flow_spec), GFP_KERNEL);
        if (!inst->data->qos[CUBE_RELIABLE]) {
                LOG_ERR("Failed creation of QoS cube %d", CUBE_RELIABLE);
                inst_cleanup(inst);
                return NULL;
        }

        inst->data->host_name = INADDR_ANY;

        BUILD_BUG_ON(CONFIG_RINA_SHIM_TCP_UDP_BUFFER_SIZE < 2);
        BUILD_BUG_ON(CONFIG_RINA_SHIM_TCP_UDP_BUFFER_SIZE > 65535);

        inst->data->qos[CUBE_UNRELIABLE]->average_bandwidth           = 0;
        inst->data->qos[CUBE_UNRELIABLE]->average_sdu_bandwidth       = 0;
        inst->data->qos[CUBE_UNRELIABLE]->delay                       = 0;
        inst->data->qos[CUBE_UNRELIABLE]->jitter                      = 0;
        inst->data->qos[CUBE_UNRELIABLE]->max_allowable_gap           = -1;
        inst->data->qos[CUBE_UNRELIABLE]->max_sdu_size                =
                CONFIG_RINA_SHIM_TCP_UDP_BUFFER_SIZE;

        inst->data->qos[CUBE_UNRELIABLE]->ordered_delivery            = 0;
        inst->data->qos[CUBE_UNRELIABLE]->partial_delivery            = 1;
        inst->data->qos[CUBE_UNRELIABLE]->peak_bandwidth_duration     = 0;
        inst->data->qos[CUBE_UNRELIABLE]->peak_sdu_bandwidth_duration = 0;
        inst->data->qos[CUBE_UNRELIABLE]->undetected_bit_error_rate   = 0;

        inst->data->qos[CUBE_RELIABLE]->average_bandwidth             = 0;
        inst->data->qos[CUBE_RELIABLE]->average_sdu_bandwidth         = 0;
        inst->data->qos[CUBE_RELIABLE]->delay                         = 0;
        inst->data->qos[CUBE_RELIABLE]->jitter                        = 0;
        inst->data->qos[CUBE_RELIABLE]->max_allowable_gap             = 0;
        inst->data->qos[CUBE_RELIABLE]->max_sdu_size                  =
                CONFIG_RINA_SHIM_TCP_UDP_BUFFER_SIZE - sizeof(__be16);
        inst->data->qos[CUBE_RELIABLE]->ordered_delivery              = 1;
        inst->data->qos[CUBE_RELIABLE]->partial_delivery              = 0;
        inst->data->qos[CUBE_RELIABLE]->peak_bandwidth_duration       = 0;
        inst->data->qos[CUBE_RELIABLE]->peak_sdu_bandwidth_duration   = 0;
        inst->data->qos[CUBE_RELIABLE]->undetected_bit_error_rate     = 0;

        /* FIXME: Remove as soon as the kipcm_kfa gets removed*/
        inst->data->kfa = kipcm_kfa(default_kipcm);

        ASSERT(inst->data->kfa);

        LOG_DBG("KFA instance %pK bound", inst->data->kfa);

        spin_lock_init(&inst->data->lock);

        INIT_LIST_HEAD(&(inst->data->flows));
        INIT_LIST_HEAD(&(inst->data->reg_apps));
        INIT_LIST_HEAD(&(inst->data->directory));
        INIT_LIST_HEAD(&(inst->data->exp_regs));

        /*
         * Bind the shim-instance to the shims set, to keep all our data
         * structures linked (somewhat) together
         */
        INIT_LIST_HEAD(&(inst->data->list));
        spin_lock(&data->lock);
        list_add(&(inst->data->list), &(data->instances));
        spin_unlock(&data->lock);

        return inst;
}

static int tcp_udp_destroy(struct ipcp_factory_data * data,
                           struct ipcp_instance *     instance)
{
        struct host_ipcp_instance_mapping * mapping;
        struct ipcp_instance_data         * pos, * next;

        LOG_HBEAT;

        ASSERT(data);
        ASSERT(instance);

        LOG_DBG("Looking for the instance to destroy");

        list_for_each_entry_safe(pos, next, &data->instances, list) {
                if (pos->id == instance->data->id) {
                        LOG_DBG("Instance is %pK", pos);

                        /* Unbind from the instances set */
                        spin_lock(&data->lock);
                        list_del(&pos->list);
                        spin_unlock(&data->lock);

                        /* Destroy it */
                        if (pos->qos[CUBE_UNRELIABLE])
                                rkfree(pos->qos[CUBE_UNRELIABLE]);
                        if (pos->qos[CUBE_RELIABLE])
                                rkfree(pos->qos[CUBE_RELIABLE]);
                        if (pos->qos)
                                rkfree(pos->qos);

                        if (pos->name)
                                name_destroy(pos->name);
                        if (pos->dif_name)
                                name_destroy(pos->dif_name);

                        mapping = inst_data_mapping_get(pos->host_name);
                        if (mapping) {
                                LOG_DBG("Removing mapping from list");
                                spin_lock(&data_instances_lock);
                                list_del(&mapping->list);
                                spin_unlock(&data_instances_lock);
                                kfree(mapping);
                        }

                        robject_del(&instance->robj);

                        rkfree(pos);
                        rkfree(instance);

                        LOG_DBG("Instance destroyed, returning");

                        return 0;
                }
        }

        LOG_DBG("Didn't find the instance I was lookig for, returning error");

        return -1;
}

static struct ipcp_factory_ops tcp_udp_ops = {
        .init    = tcp_udp_init,
        .fini    = tcp_udp_fini,
        .create  = tcp_udp_create,
        .destroy = tcp_udp_destroy,
};

static struct ipcp_factory * shim = NULL;

static int __init mod_init(void)
{
        BUILD_BUG_ON(CONFIG_RINA_SHIM_TCP_UDP_BUFFER_SIZE <= 0);

        rcv_wq = alloc_workqueue(SHIM_NAME_RWQ,
                                 WQ_MEM_RECLAIM | WQ_HIGHPRI | WQ_UNBOUND, 1);
        if (!rcv_wq) {
                LOG_CRIT("Cannot create the receiver-wq");
                return -1;
        }

        snd_wq = alloc_workqueue(SHIM_NAME_WWQ,
                                 WQ_MEM_RECLAIM | WQ_HIGHPRI | WQ_UNBOUND, 1);
        if (!snd_wq) {
                LOG_CRIT("Cannot create the sender-wq");
                destroy_workqueue(rcv_wq);
                return -1;
        }

        shim = kipcm_ipcp_factory_register(default_kipcm,
                                           SHIM_NAME,
                                           &tcp_udp_data, &tcp_udp_ops);
        if (!shim) {
                destroy_workqueue(snd_wq);
                destroy_workqueue(rcv_wq);
                return -1;
        }

        return 0;
}

static void __exit mod_exit(void)
{
        struct rcv_data * recvd, * nxt_r;
        struct rcv_data * sendd, * nxt_s;

        LOG_DBG("Disposing receiver-wq");
        flush_workqueue(rcv_wq);
        destroy_workqueue(rcv_wq);
        list_for_each_entry_safe(recvd, nxt_r, &rcv_wq_data, list) {
                LOG_DBG("Disposing stale data in receiver-wq");
                list_del(&recvd->list);
                rkfree(recvd);
        }

        LOG_DBG("Disposing sender-wq");
        flush_workqueue(snd_wq);
        destroy_workqueue(snd_wq);
        list_for_each_entry_safe(sendd, nxt_s, &snd_wq_data, list) {
                LOG_DBG("Disposing stale data in sender-wq");
                list_del(&sendd->list);
                rkfree(sendd);
        }

        kipcm_ipcp_factory_unregister(default_kipcm, shim);
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("RINA Shim IPC Process over TCP/UDP");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini@nextworks.it>");
MODULE_AUTHOR("Sander Vrijders <sander.vrijders@intec.ugent.be>");
MODULE_AUTHOR("Douwe De Bock <douwe.debock@ugent.be>");

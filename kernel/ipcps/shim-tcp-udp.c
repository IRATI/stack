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
#include <linux/inet.h>
#include <net/sock.h>
#include <linux/version.h>

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
#define SEND_WQ_MAX_SIZE 1000

static struct workqueue_struct * rcv_wq;
static struct workqueue_struct * snd_wq;
static struct work_struct        rcv_work;
static struct work_struct        snd_work;
static struct list_head          rcv_wq_data;
static struct list_head          snd_wq_data;
static int snd_wq_size;
static DEFINE_SPINLOCK(rcv_wq_lock);
static DEFINE_SPINLOCK(snd_wq_lock);

static int parse_assign_conf(struct ipcp_instance_data * data,
                             const struct dif_config *   config);

/* Structure for the workqueue */
struct rcv_data {
        struct list_head list;
        struct sock *    sk;
};

struct snd_data {
        struct list_head            list;
        struct ipcp_instance_data * data;
        port_id_t                   id;
        struct du *                 du;
};

/* FIXME: To be removed ABSOLUTELY */
extern struct kipcm * default_kipcm;

union address {
	sa_family_t                 family;
	struct sockaddr             sa;
	struct sockaddr_in          in;
	struct sockaddr_in6         in6;
};

struct hostname {
	sa_family_t family;
	union {
		struct in_addr      in;
		struct in6_addr     in6;
	};
};

enum port_id_state {
        PORT_STATE_NULL = 1,
        PORT_STATE_PENDING,
        PORT_STATE_ALLOCATED
};

/* Data for registered applications */
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
        union address          addr;

        struct rfifo *         sdu_queue;

        int                    bytes_left;
        int                    lbuf;
        struct du *            du;

        struct ipcp_instance * user_ipcp;
};

struct ipcp_instance_data {
        struct list_head    list;
        ipc_process_id_t    id;

        /* IPC Process name */
        struct name *       name;
        struct name *       dif_name;

        struct hostname     host_name;

        struct flow_spec ** qos;

        /* Stores the state of flows indexed by port_id */
        struct list_head    flows;

        /* Holds reg_app_data, e.g. the registered applications */
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

	union address    addr;
};

/* Expected application registration */
struct exp_reg {
        struct list_head list;

        struct name *    app_name;
        int              port;
};

static struct ipcp_factory_data {
        spinlock_t lock;
        struct list_head instances;
} tcp_udp_data;


static void hostname_init(struct hostname *     host_name,
			  const union address * sa)
{
	host_name->family = sa->family;
	switch (host_name->family) {
	case AF_INET:
		host_name->in = sa->in.sin_addr;
		break;
	case AF_INET6:
		host_name->in6 = sa->in6.sin6_addr;
		break;
	default:
		unreachable();
	}
}

static bool hostname_is_equal(const struct hostname * a,
			      const struct hostname * b)
{
	/*if (!a)
		return !b;*/
	if (a->family == b->family) {
		switch (a->family) {
		case AF_INET:
			return a->in.s_addr == b->in.s_addr;
		case AF_INET6:
			return !memcmp(&a->in6, &b->in6, 16);
		}
		unreachable();
	}
	return false;
}

static bool sockaddr_is_equal(const union address * a,
			      const union address * b)
{
	/*if (!a)
		return !b;*/
	if (a->family == b->family) {
		switch (a->family) {
		case AF_INET:
			return a->in.sin_port == b->in.sin_port
			    && a->in.sin_addr.s_addr == b->in.sin_addr.s_addr;
		case AF_INET6:
			return a->in6.sin6_port == b->in6.sin6_port
			    && a->in6.sin6_flowinfo == b->in6.sin6_flowinfo
			    && !memcmp(&a->in6.sin6_addr, &b->in6.sin6_addr, 16)
			    && a->in6.sin6_scope_id == b->in6.sin6_scope_id;
		}
		unreachable();
	}
	return false;
}

static unsigned sockaddr_init(union address *         sa,
			      const struct hostname * host_name,
			      int                     port)
{
	sa->family = host_name->family;
	switch (sa->family) {
	case AF_INET:
		sa->in.sin_port = htons(port);
		sa->in.sin_addr = host_name->in;
		return sizeof sa->in;
	case AF_INET6:
		sa->in6.sin6_port = htons(port);
		sa->in6.sin6_addr = host_name->in6;
		sa->in6.sin6_flowinfo = sa->in6.sin6_scope_id = 0;
		return sizeof sa->in6;
	}
	unreachable();
}

static unsigned sockaddr_copy(union address * src, union address * dst)
{
	switch (src->family) {
	case AF_INET:
		dst->in = src->in;
		return sizeof dst->in;
	case AF_INET6:
		dst->in6 = src->in6;
		return sizeof dst->in6;
	}
	unreachable();
}

static ssize_t shim_tcp_udp_ipcp_sysfs_show(struct kobject *   kobj,
					    struct attribute * attr,
					    char *             buf)
{
	struct robject * robj = container_of(kobj, struct robject, kobj);
	struct ipcp_instance * instance;
	char *tmp;
	int size;

	instance = container_of(robj, struct ipcp_instance, robj);
	if (!instance || !instance->data)
		return -EIO;

	if (!strcmp(attr->name, "type"))
		return sprintf(buf, "shim-tcp-udp\n");
	if (!strcmp(attr->name, "host_name"))
		switch(instance->data->host_name.family) {
		case AF_INET:
			return sprintf(buf, "%pI4\n",
				&instance->data->host_name.in);
		case AF_INET6:
			return sprintf(buf, "%pI6c\n",
				&instance->data->host_name.in6);
		default:
			unreachable();
		case AF_UNSPEC:
			return 0;
		}
	if (!strcmp(attr->name, "name"))
		tmp = name_tostring(instance->data->name);
	else if (!strcmp(attr->name, "dif"))
		tmp = name_tostring(instance->data->dif_name);
	else
		BUG();

	if (!tmp)
		return -EIO;
	size = sprintf(buf, "%s\n", tmp);
	rkfree(tmp);
	return size;
}

static ssize_t shim_tcp_udp_ipcp_sysfs_store(struct kobject *   kobj,
					     struct attribute * attr,
					     const char *       buf,
					     size_t             size)
{
	struct robject * robj = container_of(kobj, struct robject, kobj);
	struct ipcp_instance * instance;

	instance = container_of(robj, struct ipcp_instance, robj);
	if (!instance || !instance->data)
		return -EIO;

	if (!strcmp(attr->name, "config")) {
		struct ipcp_instance_data *data = instance->data;
		struct ipcp_config_entry entry;
		struct ipcp_config config;
		struct dif_config dif_config;
		const char *p = buf, *q = buf + size;
		INIT_LIST_HEAD(&dif_config.ipcp_config_entries);
		list_add(&config.next, &dif_config.ipcp_config_entries);
		config.entry = &entry;
		for (; p < q; p++) {
			if (!strcmp(p, "hostname")
			 && !list_empty(&data->reg_apps)) {
				LOG_ERR("Applications are already registered");
			} else if (!(entry.name = (char *) p,
				   p = memchr(p, '\0', q - p))
			      || !(entry.value = (char *) ++p,
				   p = memchr(p, '\0', q - p))) {
				LOG_ERR("Error parsing config update");
			} else if (!parse_assign_conf(data, &dif_config)) {
				continue;
			}
			return -EINVAL;
		}
		return size;
	}

	BUG();
}

static const struct sysfs_ops shim_tcp_udp_ipcp_sysfs_ops = {
	.show = shim_tcp_udp_ipcp_sysfs_show,
	.store = shim_tcp_udp_ipcp_sysfs_store,
};
static struct attribute _shim_tcp_udp_ipcp_attrs[] = {
	{"name",      S_IRUGO},
	{"type",      S_IRUGO},
	{"dif",       S_IRUGO},
	{"host_name", S_IRUGO},
	{"config",    S_IWUSR},
};
static struct attribute *shim_tcp_udp_ipcp_attrs[] = {
	_shim_tcp_udp_ipcp_attrs+0,
	_shim_tcp_udp_ipcp_attrs+1,
	_shim_tcp_udp_ipcp_attrs+2,
	_shim_tcp_udp_ipcp_attrs+3,
	_shim_tcp_udp_ipcp_attrs+4,
	NULL
};

RINA_KTYPE(shim_tcp_udp_ipcp);

static struct ipcp_instance_data *
find_instance_by_hostname(const struct hostname * host_name)
{
        struct ipcp_instance_data * data;

        ASSERT(host_name);

        spin_lock(&tcp_udp_data.lock);

        list_for_each_entry(data, &tcp_udp_data.instances, list) {
                if (hostname_is_equal(host_name, &data->host_name)) {
                        spin_unlock(&tcp_udp_data.lock);
                        return data;
                }
        }

        spin_unlock(&tcp_udp_data.lock);

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

        ASSERT(data);
        ASSERT(is_port_id_ok(id));

        spin_lock_bh(&data->lock);

        list_for_each_entry(flow, &data->flows, list) {
                if (flow->port_id == id) {
                        spin_unlock_bh(&data->lock);
                        return flow;
                }
        }

        spin_unlock_bh(&data->lock);

        return NULL;
}

static struct shim_tcp_udp_flow *
find_flow_by_socket(struct ipcp_instance_data * data,
                    const struct socket *       sock)
{
        struct shim_tcp_udp_flow * flow;

        ASSERT(data);

        spin_lock_bh(&data->lock);

        list_for_each_entry(flow, &data->flows, list) {
                if (flow->sock == sock) {
                        spin_unlock_bh(&data->lock);
                        return flow;
                }
        }

        spin_unlock_bh(&data->lock);

        return NULL;
}

/* No lock needed here, called only when already holding a lock */
static struct shim_tcp_udp_flow *
find_udp_flow(struct ipcp_instance_data * data,
              const union address *       addr,
              const struct socket *       sock)
{
        struct shim_tcp_udp_flow * flow;

        ASSERT(data);
        ASSERT(addr);
        ASSERT(sock);

        list_for_each_entry(flow, &data->flows, list) {
                if (flow->sock == sock &&
                    sockaddr_is_equal(addr, &flow->addr)) {
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

        ASSERT(data);
        ASSERT(sock);

        spin_lock_bh(&data->lock);

        /* FIXME: Shrink this code */
        if (sock->type == SOCK_DGRAM) {
                list_for_each_entry(app, &data->reg_apps, list) {
                        if (app->udpsock == sock) {
                                spin_unlock_bh(&data->lock);
                                return app;
                        }
                }
        } else {
                list_for_each_entry(app, &data->reg_apps, list) {
                        if (app->tcpsock == sock) {
                                spin_unlock_bh(&data->lock);
                                return app;
                        }
                }
        }

        spin_unlock_bh(&data->lock);

        return NULL;
}

static struct reg_app_data * find_app_by_name(struct ipcp_instance_data * data,
                                              const struct name *         name)
{
        struct reg_app_data * app;

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
        ASSERT(data);
        ASSERT(flow);

        spin_lock(&data->lock);
        if (!list_empty(&flow->list))
                list_del(&flow->list);
        spin_unlock(&data->lock);

        /* FIXME: Check for leaks */
        if (flow->sdu_queue)
                rfifo_destroy(flow->sdu_queue, (void (*)(void *)) du_destroy);
        rkfree(flow);

        return 0;
}

static int unbind_and_destroy_flow(struct ipcp_instance_data * data,
                                   struct shim_tcp_udp_flow *  flow)
{
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
        union address              addr;
        struct dir_entry *         entry;
        int                        err;
        struct ipcp_instance *     ipcp;
        unsigned                   len;

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
                        LOG_ERR("Directory entry not found for <APN=%s AEN=%s>",
                                dest->process_name, dest->entity_name);
                        list_del(&flow->list);
                        rkfree(flow);
                        return -1;
                }
                LOG_DBG("Directory entry found");

                len = sockaddr_copy(&entry->addr, &flow->addr);

                LOG_DBG("Max allowable gap is %d", fspec->max_allowable_gap);
                if (fspec->max_allowable_gap != 0) {
                        LOG_DBG("Unreliable flow requested");
                        flow->fspec_id = 0;

                        len = sockaddr_init(&addr, &data->host_name, 0);
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,2,0)
                        err = sock_create_kern(addr.family, SOCK_DGRAM,
                                               IPPROTO_UDP, &flow->sock);
#else
                        err = sock_create_kern(&init_net, addr.family, SOCK_DGRAM,
                                               IPPROTO_UDP, &flow->sock);
#endif
                        if (err < 0) {
                                LOG_ERR("Could not create UDP socket");
                                unbind_and_destroy_flow(data, flow);
                                return -1;
                        }

                        err = kernel_bind(flow->sock, &addr.sa, len);
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

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,2,0)
                        err = sock_create_kern(flow->addr.family, SOCK_STREAM,
                                               IPPROTO_TCP, &flow->sock);
#else
                        err = sock_create_kern(&init_net, flow->addr.family, SOCK_STREAM,
                                               IPPROTO_TCP, &flow->sock);
#endif
                        if (err < 0) {
                                LOG_ERR("Could not create TCP socket");
                                unbind_and_destroy_flow(data, flow);
                                return -1;
                        }

                        err = kernel_connect(flow->sock, &flow->addr.sa,
                                             len, 0);
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
                        struct du * tmp = NULL;

                        tmp = rfifo_pop(flow->sdu_queue);
                        ASSERT(tmp);

                        LOG_DBG("Got a new element from the fifo, "
                                "enqueuing into user-IPCP");

                        if (!flow->user_ipcp) {
                        	LOG_ERR("Flow is being deallocated, dropping PDU");
                        	du_destroy(tmp);
                        	return -1;
                        }

                        if (flow->user_ipcp->ops->
                            du_enqueue(flow->user_ipcp->data,
                                        flow->port_id,
                                        tmp)) {
                                LOG_ERR("Couldn't enqueue SDU to user-IPCP");
                                return -1;
                        }
                }

                rfifo_destroy(flow->sdu_queue, (void (*)(void *)) du_destroy);
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
                rfifo_destroy(flow->sdu_queue, (void (*)(void *)) du_destroy);
                flow->sdu_queue = NULL;
        }

        return 0;
}

static int flow_deallocate(struct ipcp_instance_data * data,
			   struct shim_tcp_udp_flow * flow)
{
        struct reg_app_data *      app;
        struct rcv_data *          recvd;

	ASSERT(data);
	ASSERT(flow);

        app = find_app_by_socket(data, flow->sock);

        if ( (flow->fspec_id == 1 || (flow->fspec_id == 0 && !app)) &&
            flow->port_id_state == PORT_STATE_ALLOCATED) {

                /* FIXME: better cleanup (= removing from list) */
                spin_lock_bh(&rcv_wq_lock);
                list_for_each_entry(recvd, &rcv_wq_data, list) {
                        if (recvd->sk->sk_socket == flow->sock) {
                                LOG_DBG("Setting socket to NULL");
                                recvd->sk = NULL;
                        }
                }
                spin_unlock_bh(&rcv_wq_lock);

                LOG_DBG("Closing socket");
                kernel_sock_shutdown(flow->sock, SHUT_RDWR);
        }

        if (!app)
                sock_release(flow->sock);

        unbind_and_destroy_flow(data, flow);

        return 0;
}

static int tcp_udp_flow_deallocate(struct ipcp_instance_data * data,
                                   port_id_t                   id)
{
        struct shim_tcp_udp_flow * flow;

        ASSERT(data);

        flow = find_flow_by_port(data, id);
        if (!flow) {
                LOG_ERR("Flow does not exist, cannot remove");
                return -1;
        }

        return flow_deallocate(data, flow);
}

int recv_msg(struct socket * sock,
             union address * other,
             int             lother,
             unsigned char * buf,
             int             len)
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
             union address *      other,
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
        if (size > 0) {
                LOG_DBG("Sent message with %d bytes", size);
        } else {
                LOG_ERR("Problems sending message");
	}
        return size;
}

static int udp_process_msg(struct ipcp_instance_data * data,
                           struct socket *             sock)
{
        struct shim_tcp_udp_flow *  flow;
        union address               addr;
        struct reg_app_data *       app;
        struct name *               sname;
        struct du *                 du;
        int                         size;
        struct ipcp_instance      * ipcp, * user_ipcp;
        char			    api_string[12];

	/* Create SDU with max allowable size removing PCI and TAIL room */
	du = du_create_ni(CONFIG_RINA_SHIM_TCP_UDP_BUFFER_SIZE);
        if (!du) {
                LOG_ERR("Couldn't create sdu");
                return -1;
        }

        if ((size = recv_msg(sock, &addr, sizeof(addr),
                             du_buffer(du),
			     CONFIG_RINA_SHIM_TCP_UDP_BUFFER_SIZE)) < 0) {
                if (size != -EAGAIN)
                        LOG_ERR("Error during UDP recv: %d", size);
                du_destroy(du);
                return -1;
        }

        LOG_DBG("Received message of %d bytes", size);

	if (du_shrink(du, CONFIG_RINA_SHIM_TCP_UDP_BUFFER_SIZE - size)) {
		LOG_ERR("Could not shrink SDU");
		du_destroy(du);
		return -1;
	}

        spin_lock_bh(&data->lock);
        flow = find_udp_flow(data, &addr, sock);
        if (!flow) {
                spin_unlock_bh(&data->lock);
                LOG_DBG("No flow found, creating it");

                app = find_app_by_socket(data, sock);
                if (!app) {
                        LOG_ERR("No app registered yet! "
                                "Someone is doing something bad "
                                "on the network");
                        du_destroy(du);
                        return -1;
                }

                user_ipcp = kipcm_find_ipcp_by_name(default_kipcm,
                                                    app->app_name);

                if (!user_ipcp)
                        user_ipcp = kfa_ipcp_instance(data->kfa);
                ipcp = kipcm_find_ipcp(default_kipcm, data->id);
                if (!user_ipcp || !ipcp) {
                        LOG_ERR("Could not find required ipcps");
                        du_destroy(du);
                        return -1;
                }

                flow = rkzalloc(sizeof(*flow), GFP_ATOMIC);
                if (!flow) {
                        LOG_ERR("Could not allocate flow");
                        du_destroy(du);
                        return -1;
                }

                flow->port_id_state = PORT_STATE_PENDING;
                flow->port_id       = kfa_port_id_reserve(data->kfa, data->id);
                flow->user_ipcp     = user_ipcp;
                flow->sock          = sock;
                flow->fspec_id      = 0;

                sockaddr_copy(&addr, &flow->addr);

                if (!is_port_id_ok(flow->port_id)) {
                        LOG_ERR("Port id is not ok");
                        du_destroy(du);
                        if (flow_destroy(data, flow))
                                LOG_ERR("Problems destroying flow");
                        return -1;
                }

                spin_lock_bh(&data->lock);
                INIT_LIST_HEAD(&flow->list);
                list_add(&flow->list, &data->flows);
                spin_unlock_bh(&data->lock);
                LOG_DBG("Added UDP flow");

                if (!user_ipcp->ops->ipcp_name(user_ipcp->data)) {
                        LOG_DBG("This flow goes for an app");
                        if (kfa_flow_create(data->kfa, flow->port_id, ipcp,
                        		    data->id, NULL, false)) {
                                LOG_ERR("Could not create flow in KFA");
                                du_destroy(du);
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

                        du_destroy(du);
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

                        du_destroy(du);
                        kfa_port_id_release(data->kfa, flow->port_id);
                        unbind_and_destroy_flow(data, flow);
                        return -1;
                }

                /* FIXME: This sets the name to the server? */
                if (sprintf(&api_string[0], "%d", flow->port_id) < 0){
                	kfa_port_id_release(data->kfa, flow->port_id);
                	unbind_and_destroy_flow(data, flow);
                        return -1;
                }

                sname = name_create_ni();
                if (!name_init_from_ni(sname,
                		       "Unknown app",
				       (const string_t*) &api_string[0],
				       "",
				       "")) {
                        name_destroy(sname);
                        kfa_port_id_release(data->kfa, flow->port_id);
                        unbind_and_destroy_flow(data, flow);
                        return -1;
                }

                if (kipcm_flow_arrived(default_kipcm,
                                       data->id,
                                       flow->port_id,
                                       data->dif_name,
				       app->app_name,
                                       sname,
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
                        spin_unlock_bh(&data->lock);

                        LOG_DBG("Port is ALLOCATED, "
                                "queueing frame in user-IPCP");

                        if (!flow->user_ipcp) {
                        	LOG_ERR("Flow is being deallocated, dropping PDU");
                        	du_destroy(du);
                        	return -1;
                        }

                        if (flow->user_ipcp->ops->
                            du_enqueue(flow->user_ipcp->data,
                                        flow->port_id,
                                        du)) {
                                LOG_ERR("Couldn't enqueue SDU to user IPCP");
                                return -1;
                        }
                } else if (flow->port_id_state == PORT_STATE_PENDING) {
                        LOG_DBG("Port is PENDING, "
                                "queueing frame in SDU queue");

                        if (rfifo_push(flow->sdu_queue, du)) {
                                spin_unlock_bh(&data->lock);

                                LOG_ERR("Failed to write %zd bytes"
                                        "into the fifo",
                                        sizeof(struct du *));

                                du_destroy(du);
                                return -1;
                        }

                        spin_unlock_bh(&data->lock);
                } else if (flow->port_id_state == PORT_STATE_NULL) {
                        spin_unlock_bh(&data->lock);

                        LOG_DBG("Port is NULL, "
                                "dropping SDU");

                        du_destroy(du);
                } else {
                        spin_unlock_bh(&data->lock);
                        LOG_ERR("Port state is unhandled");
                }
        }

        return size;
}

static int tcp_recv_new_message(struct ipcp_instance_data * data,
                                struct socket *             sock,
                                struct shim_tcp_udp_flow *  flow)
{
        struct du *    du;
        char            sbuf[2];
        int             size;
        __be16          nlen;

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

	du = du_create_ni(flow->bytes_left);
        if (!du) {
                LOG_ERR("Couldn't create sdu");
                return -1;
        }

        size = recv_msg(sock, NULL, 0, du_buffer(du), flow->bytes_left);
        if (size <= 0) {
                if (size != -EAGAIN) {
                        LOG_ERR("Error during TCP receive (%d)", size);
                        du_destroy(du);
                }

                flow->lbuf = flow->bytes_left;
                flow->du = du;
                return size;
        }

        if (size == flow->bytes_left) {
                flow->bytes_left = 0;
                spin_lock_bh(&data->lock);
                if (flow->port_id_state == PORT_STATE_ALLOCATED) {
                        spin_unlock_bh(&data->lock);

                        if (!flow->user_ipcp) {
                        	LOG_ERR("Flow is being deallocated, dropping SDU");
                        	du_destroy(du);
                        	return -1;
                        }

                        ASSERT(flow->user_ipcp->ops);
                        ASSERT(flow->user_ipcp->ops->sdu_enqueue);

                        if (flow->user_ipcp->ops->
                            du_enqueue(flow->user_ipcp->data,
                                        flow->port_id,
                                        du)) {
                                LOG_ERR("Couldn't enqueue SDU to user IPCP");
                                return -1;
                        }
                } else if (flow->port_id_state == PORT_STATE_PENDING) {
                        LOG_DBG("Port is PENDING, "
                                "queueing frame in SDU queue");

                        if (rfifo_push_ni(flow->sdu_queue, du)) {
                                spin_unlock_bh(&data->lock);

                                LOG_ERR("Failed to write %zd bytes"
                                        "into the fifo",
                                        sizeof(struct sdu *));

                                du_destroy(du);
                                return -1;
                        }

                        spin_unlock_bh(&data->lock);
                } else if (flow->port_id_state == PORT_STATE_NULL) {
                        spin_unlock_bh(&data->lock);
                        du_destroy(du);
                }

                return size;
        } else {
                LOG_DBG("Didn't receive complete message, missing %d bytes", flow->bytes_left);

                flow->lbuf = flow->bytes_left;
                flow->bytes_left = flow->bytes_left - size;
                flow->du = du;

                return -1;
        }
}

static int tcp_recv_partial_message(struct ipcp_instance_data * data,
                                    struct socket *             sock,
                                    struct shim_tcp_udp_flow *  flow)
{
        struct du *    du;
        int             start, size;

        start = flow->lbuf - flow->bytes_left;

        size = recv_msg(sock, NULL, 0, du_buffer(flow->du)+start, flow->bytes_left);
        if (size <= 0) {
                if (size != -EAGAIN)
                        LOG_ERR("Error during TCP receive (%d)", size);
                return size;
        }

        if (size == flow->bytes_left) {
                flow->bytes_left = 0;
		du = flow->du;
		flow->du = NULL;
                spin_lock_bh(&data->lock);
                if (flow->port_id_state == PORT_STATE_ALLOCATED) {
                        spin_unlock_bh(&data->lock);

                        if (!flow->user_ipcp) {
                        	LOG_ERR("Flow is being deallocated, dropping SDU");
                        	du_destroy(du);
                        	return -1;
                        }

                        if (flow->user_ipcp->ops->
                            du_enqueue(flow->user_ipcp->data,
                                        flow->port_id,
                                        du)) {
                                LOG_ERR("Couldn't enqueue SDU to user IPCP");
                                return -1;
                        }
                } else if (flow->port_id_state == PORT_STATE_PENDING) {
                        LOG_DBG("Port is PENDING, "
                                "queueing frame in SDU queue");

                        if (rfifo_push_ni(flow->sdu_queue, du)) {
                                spin_unlock_bh(&data->lock);

                                LOG_ERR("Failed to write %zd bytes"
                                        "into the fifo",
                                        sizeof(struct du *));

                                du_destroy(du);
                                return -1;
                        }

                        spin_unlock_bh(&data->lock);
                } else if (flow->port_id_state == PORT_STATE_NULL) {
                        spin_unlock_bh(&data->lock);
                        du_destroy(du);
                }

                return size;
        } else {
        	flow->bytes_left = flow->bytes_left - size;
                LOG_DBG("Still didn't receive complete message, missing %d bytes",
                	 flow->bytes_left);

                return -1;
        }
}

static int tcp_process_msg(struct ipcp_instance_data * data,
                           struct socket *             sock)
{
        struct shim_tcp_udp_flow * flow;
        struct rcv_data *          recvd;
        int                        size;

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
                spin_lock_bh(&rcv_wq_lock);
                list_for_each_entry(recvd, &rcv_wq_data, list) {
                        if (recvd->sk->sk_socket == flow->sock)
                                recvd->sk = NULL;
                }
                spin_unlock_bh(&rcv_wq_lock);

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

        LOG_DBG("Got message of %d bytes", size);

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
        char	   		   api_string[12];

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

                spin_lock_bh(&data->lock);
                INIT_LIST_HEAD(&flow->list);
                list_add(&flow->list, &data->flows);
                spin_unlock_bh(&data->lock);
                LOG_DBG("TCP flow added");

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
                        if (kfa_flow_create(data->kfa, flow->port_id, ipcp,
                        		    data->id, NULL, false)) {
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

                if (sprintf(&api_string[0], "%d", flow->port_id) < 0){
                	kfa_port_id_release(data->kfa, flow->port_id);
                	unbind_and_destroy_flow(data, flow);
                        return -1;
                }

                sname = name_create_ni();
                if (!name_init_from_ni(sname,
                		       "Unknown app",
				       (const string_t*) &api_string[0],
				       "",
				       "")) {
                        name_destroy(sname);
                        kfa_port_id_release(data->kfa, flow->port_id);
                        tcp_unbind_and_destroy_flow(data, flow);
                        return -1;
                }

                if (kipcm_flow_arrived(default_kipcm,
                                       data->id,
                                       flow->port_id,
                                       data->dif_name,
                                       app->app_name,
                                       sname,
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
        struct ipcp_instance_data *         data;
        struct hostname                     host_name;
        union address                       own;
        struct socket *                     sock;
        int                                 res, len;

        ASSERT(sk);

        sock = sk->sk_socket;
        if (!sock) {
                LOG_ERR("BUG: sk->sk_socket is NULL");
                return -1;
        }

        len = sizeof own;
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,17,0)
        if (kernel_getsockname(sock, &own.sa, &len)) {
#else
	if (kernel_getsockname(sock, &own.sa)) {
#endif
                LOG_ERR("Couldn't retrieve hostname");
                return -1;
        }
        switch (own.family) {
        case AF_INET:
                LOG_DBG("Found sockname (%pI4)", &own.in.sin_addr);
                break;
        case AF_INET6:
                LOG_DBG("Found sockname (%pI6c)", &own.in6.sin6_addr);
                break;
        default:
                unreachable();
        }

        hostname_init(&host_name, &own);
        data = find_instance_by_hostname(&host_name);
        if (!data) {
                LOG_ERR("BUG: failed to map hostname");
                return -1;
        }

        if (sk->sk_socket->type == SOCK_DGRAM) {
                do res = udp_process_msg(data, sock);
                while (res > 0);
                return res;
        } else
                return tcp_process(data, sock);
}

static void tcp_udp_rcv_worker(struct work_struct * work)
{
        struct rcv_data * recvd, * next;

        /* FIXME: more efficient locking and better cleanup */
        spin_lock_bh(&rcv_wq_lock);
        list_for_each_entry_safe(recvd, next, &rcv_wq_data, list) {
                list_del(&recvd->list);
                spin_unlock_bh(&rcv_wq_lock);

                LOG_DBG("Worker on %pK", recvd->sk);

                if (recvd->sk != NULL) {
                        int res = tcp_udp_rcv_process_msg(recvd->sk);
                        if (res <= 0)
                                LOG_DBG("TCP/UDP processing returned %d", res);
                }

                rkfree(recvd);

                spin_lock_bh(&rcv_wq_lock);
        }
        spin_unlock_bh(&rcv_wq_lock);

        LOG_DBG("Worker finished for now");
}

static int tcp_udp_application_register(struct ipcp_instance_data * data,
                                        const struct name *         name,
					const struct name *         daf_name)
{
        struct reg_app_data * app;
        union address         addr;
        unsigned              sa_len;
        struct exp_reg *      exp_reg;
        int                   err;

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
                LOG_ERR("That application is not expected to register"
                        " <APN=%s AEN=%s>",
                        name->process_name, name->entity_name);
                rkfree(app);
                return -1;
        }
        LOG_DBG("Application found in exp_reg");

        app->port = exp_reg->port;

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,2,0)
        err = sock_create_kern(data->host_name.family, SOCK_DGRAM,
			       IPPROTO_UDP, &app->udpsock);
#else
        err = sock_create_kern(&init_net, data->host_name.family, SOCK_DGRAM,
			       IPPROTO_UDP, &app->udpsock);
#endif
        if (err < 0) {
                LOG_ERR("Could not create UDP socket for registration");
                name_destroy(app->app_name);
                rkfree(app);
                return -1;
        }

        sa_len = sockaddr_init(&addr, &data->host_name, app->port);
        err = kernel_bind(app->udpsock, &addr.sa, sa_len);
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

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,2,0)
        err = sock_create_kern(data->host_name.family, SOCK_STREAM,
			       IPPROTO_TCP, &app->tcpsock);
#else
        err = sock_create_kern(&init_net, data->host_name.family, SOCK_STREAM,
			       IPPROTO_TCP, &app->tcpsock);
#endif
        if (err < 0) {
                LOG_ERR("could not create TCP socket for registration");
                sock_release(app->udpsock);
                name_destroy(app->app_name);
                rkfree(app);
                return -1;
        }

        err = kernel_bind(app->tcpsock, &addr.sa, sa_len);
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

static int application_unregister(struct ipcp_instance_data * data,
				  struct reg_app_data * app)
{
	ASSERT(data);
        ASSERT(app);

	lock_sock(app->udpsock->sk);
	write_lock_bh(&app->udpsock->sk->sk_callback_lock);
	app->udpsock->sk->sk_data_ready = app->udpsock->sk->sk_user_data;
	app->udpsock->sk->sk_user_data  = NULL;
	write_unlock_bh(&app->udpsock->sk->sk_callback_lock);
	release_sock(app->udpsock->sk);

	kernel_sock_shutdown(app->udpsock, SHUT_RDWR);
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

static int tcp_udp_application_unregister(struct ipcp_instance_data * data,
                                          const struct name *         name)
{
        struct reg_app_data * app;

        ASSERT(data);
        ASSERT(name);

        LOG_DBG("Handling application un-registration");

        app = find_app_by_name(data, name);
        if (!app) {
                LOG_ERR("Application is not registered, so can't unregister");
                return -1;
        }

        return application_unregister(data, app);
}

#define HASCOUNT_SYNTAX 0	// entries:count:fieldcount:fieldcount:field
#define NOCOUNT_SYNTAX  1	// :entries:field:field:field

static int get_nxt_len(char ** enc,
                       int *   len,
		       int     syntax)
{
        char * tmp;

        ASSERT(enc);
	ASSERT(*enc);

	switch (syntax) {
	    case HASCOUNT_SYNTAX:
		tmp = strsep(enc, ":");
		if (!*enc) {
			LOG_ERR("No separator found!");
			return -1;
		}
		if (kstrtouint(tmp, 10, len)) {
			LOG_ERR("Failed to convert int");
			return -1;
		}
		break;

	    case NOCOUNT_SYNTAX:
	    default:
		if (**enc != ':')
			return (-1);
		(*enc)++;
		// count characters preceding first nul or :
		for (*len = 0; (*enc)[*len] && (*enc)[*len] != ':';)
			(*len)++;
		break;
	}
	//LOG_INFO ("Return *len = %d, string '%s'", *len, *enc);
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
	LOG_INFO ("Config field is '%s'", *dst);

        return 0;
}

static int get_nxt_val(char ** dst,
                       char ** val,
                       int  *  len,
		       int     syntax)
{
        ASSERT(len);
        ASSERT(dst);
        ASSERT(val);

        if (get_nxt_len(val, len, syntax)) {
                LOG_ERR("get_nxt_len failed");
                return -1;
        }

        if (eat_substr(dst, val, len)) {
                LOG_ERR("eat_substr failed");
                return -1;
        }

        return 0;
}

static void clear_directory(struct ipcp_instance_data * data)
{
        struct dir_entry * entry, * next;

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
        clear_directory(data);
        clear_exp_reg(data);
}

static int parse_dir_entry(struct ipcp_instance_data * data, char **blob, int syntax)
{
        int                result = -1;
        unsigned int       len, port_nr;
        char               * pn = 0, * pi = 0, * en = 0, * ei = 0;
        char               * ip = 0, * port = 0;
        struct dir_entry * dir_entry;
        struct dir_entry * entry;
        union address    * addr;

        ASSERT(*blob);

        dir_entry = rkmalloc(sizeof(struct dir_entry), GFP_KERNEL);
        if (!dir_entry)
                return -1;

	/* ap, ae, ip, port */
        if (get_nxt_val(&pn, blob, &len, syntax)
         || get_nxt_val(&en, blob, &len, syntax)
         || get_nxt_val(&ip, blob, &len, syntax)
         || get_nxt_val(&port, blob, &len, syntax)) {
                LOG_ERR("Failed to get next value");
                goto out;
        }

        if (*ip) {
                if (kstrtouint(port, 10, &port_nr)) {
                        LOG_ERR("Failed to convert int");
                        goto out;
                }
                addr = &dir_entry->addr;
                if (in4_pton(ip, -1, (u8*)&addr->in.sin_addr, -1, 0)) {
                        addr->family = AF_INET;
                        addr->in.sin_port = htons(port_nr);
                } else if (in6_pton(ip, -1, (u8*)&addr->in6.sin6_addr, -1, 0)) {
                        addr->family = AF_INET6;
                        addr->in6.sin6_port = htons(port_nr);
                } else {
                        LOG_ERR("Failed to parse ip");
                        goto out;
                }
        } else {
                addr = 0;
        }

        if (!(pi = rkstrdup(""))
         || !(ei = rkstrdup(""))
         || !(dir_entry->app_name = name_create())) {
                LOG_ERR("Failed to get memory");
                goto out;
        }

        name_init_with(dir_entry->app_name, pn, pi, en, ei);

        spin_lock(&data->lock);
        list_for_each_entry(entry, &data->directory, list)
                if (name_cmp(NAME_CMP_APN | NAME_CMP_AEN,
                             entry->app_name, dir_entry->app_name)) {
                        if (addr) {
                                memcpy(&entry->addr, addr, sizeof *addr);
                                LOG_DBG("Updated an existing dir entry");
                                addr = 0;
                        } else {
                                list_del(&entry->list);
                                LOG_DBG("Removed a dir entry");
                        }
                        break;
                }
        if (addr) {
                INIT_LIST_HEAD(&dir_entry->list);
                list_add(&dir_entry->list, &data->directory);
                LOG_DBG("Added a new dir entry");
        } else {
                name_destroy(dir_entry->app_name);
                rkfree(dir_entry);
        }
        spin_unlock(&data->lock);
        result = 0;

out:
        if (port)
               rkfree(port);
        if (ip)
               rkfree(ip);
        if (result) {
                if (pn)
                      rkfree(pn);
                if (pi)
                      rkfree(pi);
                if (en)
                      rkfree(en);
                if (ei)
                      rkfree(ei);
                rkfree(dir_entry);
        }
        return result;
}

static int parse_exp_reg_entry(struct ipcp_instance_data * data, char ** blob, int syntax)
{
        int              result = -1;
        struct exp_reg * exp_reg;
        unsigned int     len;
        char             * pn = 0, * pi = 0, * en = 0, * ei = 0, * port = 0;

        ASSERT(*blob);

        exp_reg = rkmalloc(sizeof(struct exp_reg), GFP_KERNEL);
        if (!exp_reg)
                return -1;

        /* ap, ae, port */
        if (get_nxt_val(&pn, blob, &len, syntax)
         || get_nxt_val(&en, blob, &len, syntax)
         || get_nxt_val(&port, blob, &len, syntax)) {
                LOG_ERR("Failed to get next value");
                goto out;
        }

        if (kstrtouint(port, 10, &exp_reg->port)) {
                LOG_ERR("Failed to convert int");
                goto out;
        }

        if (!(pi = rkstrdup(""))
         || !(ei = rkstrdup(""))
         || !(exp_reg->app_name = name_create())) {
                LOG_ERR("Failed to get memory");
                goto out;
        }

        INIT_LIST_HEAD(&exp_reg->list);
        name_init_with(exp_reg->app_name, pn, pi, en, ei);

        spin_lock(&data->lock);
        list_add(&exp_reg->list, &data->exp_regs);
        spin_unlock(&data->lock);

        LOG_DBG("Added a new exp reg entry");
        result = 0;

out:
        if (port)
               rkfree(port);
        if (result) {
                if (pn)
                      rkfree(pn);
                if (pi)
                      rkfree(pi);
                if (en)
                      rkfree(en);
                if (ei)
                      rkfree(ei);
                rkfree(exp_reg);
        }
        return result;
}

int get_syntax_and_count (char *command, char **copy, char **parse, int *syntax)
{
	int count = 0;
	char *val;

	ASSERT(command);

	val = *copy = rkstrdup(command);
	if (!val) {
		LOG_ERR("Failed to dup value");
		return -1;
	}

	*syntax = HASCOUNT_SYNTAX;	// default -- old syntax
	if (val[0] == ':') {
		int lencount, i;

		*syntax = NOCOUNT_SYNTAX;
		LOG_INFO("New config syntax detected");

		if (get_nxt_len (&val, &lencount, NOCOUNT_SYNTAX)) {
			goto syntax_error;
		}
		// kstrtoint wants null-termination, not length... so brute force.
		for (i=0; i < lencount; i++) {
			if (val[i] < '0' || val[i] > '9')
				goto syntax_error;
			count = (count * 10) + (val[i] - '0');
		}
		val += lencount;
	}
	else if (get_nxt_len(&val, &count, HASCOUNT_SYNTAX)) {
		goto syntax_error;
	}

	*parse = val;
	return (count);

syntax_error:
	LOG_ERR ("Syntax error: Unable to extract syntax and count from '%s'", command);
	rkfree (*copy);
	return (-1);
}

static int parse_assign_conf(struct ipcp_instance_data * data,
                             const struct dif_config *   config)
{
        struct ipcp_config * tmp;

        ASSERT(data);
        ASSERT(config);

        list_for_each_entry(tmp, &(config->ipcp_config_entries), next) {
                const struct ipcp_config_entry * entry = tmp->entry;
                /*
                 *  Want to get an interface name here
                 *  Directory entries and exp registrations
                 */
                if (!strcmp(entry->name, "hostname")) {
                        struct ipcp_instance_data * other;
                        struct hostname host_name;

                        ASSERT(entry->value);

                        if (in4_pton(entry->value, -1,
                                     (u8*)&host_name.in, -1, 0)) {
                                host_name.family = AF_INET;
                                LOG_DBG("Got hostname %pI4", &host_name.in);
                        } else if (in6_pton(entry->value, -1,
                                            (u8*)&host_name.in6, -1, 0)) {
                                host_name.family = AF_INET6;
                                LOG_DBG("Got hostname %pI6c", &host_name.in6);
                        } else {
                                LOG_ERR("Failed to parse hostname");
                                return -1;
                        }
                        other = find_instance_by_hostname(&host_name);
                        if (!other) {
                                data->host_name = host_name;
                        } else if (other != data) {
                                LOG_ERR("Error duplicating hostname,"
                                        " bailing out");
                                return -1;
                        }

                } else if (!strcmp(entry->name, "dirEntry")) {
                        int  count;
                        char * val, * copy;
			int syntax;

			count = get_syntax_and_count (entry->value, &copy, &val, &syntax);
			if (count < 0) {
				return (-1);
			}

                        while (count-- > 0) {
                                if (parse_dir_entry(data, &val, syntax) < 0) {
					rkfree(copy);
                                        return -1;
                                }
                        }

                        rkfree(copy);
                } else if (!strcmp(entry->name, "expReg")) {
                        char * val, * copy;
                        int  count;
			int syntax;

			count = get_syntax_and_count (entry->value, &copy, &val, &syntax);
			if (count < 0) {
				return (-1);
			}

                        while (count-- > 0) {
                                if (parse_exp_reg_entry(data, &val, syntax) < 0) {
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
                		 const struct name * dif_name,
				 const string_t * type,
				 struct dif_config * config)
{
        ASSERT(data);
        ASSERT(dif_information);

        if (data->dif_name) {
                ASSERT(data->dif_name->process_name);

                LOG_ERR("This IPC Process is already assigned to the DIF %s",
                        data->dif_name->process_name);
                return -1;
        }

        data->dif_name = name_dup(dif_name);
        if (!data->dif_name) {
                LOG_ERR("Error duplicating name, bailing out");
                return -1;
        }

        if (parse_assign_conf(data, config)) {
                LOG_ERR("Failed to parse configuration");
                goto err;
        }

        return 0;

err:
        name_destroy(data->dif_name);
        data->dif_name = NULL;
        undo_assignment(data);
        return -1;
}

static int tcp_udp_update_dif_config(struct ipcp_instance_data * data,
                                     const struct dif_config *   new_config)
{
        ASSERT(data);
        ASSERT(new_config);

        /*
         * FIXME: Update configuration here
         *
         * Close all sockets, destroy all flows
         */

        undo_assignment(data);

        if (parse_assign_conf(data, new_config)) {
                LOG_ERR("Failed to update configuration");
                undo_assignment(data);
                /* FIXME: If this fails, DIF is no longer functional */
                return -1;
        }

        return 0;
}

static int tcp_sdu_write(struct shim_tcp_udp_flow * flow,
                         int                        len,
                         char *                     sbuf)
{
        uint16_t length;
        int    size, total;

        ASSERT(flow);
        ASSERT(len);
        ASSERT(sbuf);

        length = htons((short)len);

        total = 0;
        while (total < sizeof(uint16_t)) {
                size = send_msg(flow->sock, NULL, 0, (char*)(&length+total),
                                sizeof(uint16_t) - total);
                if (size < 0) {
                        LOG_ERR("error during sdu write (tcp): %d", size);
                        return -1;
                }
                total += size;
        }
        total = 0;
        while (total < len) {
                size = send_msg(flow->sock, NULL, 0, sbuf+total,
                                len - total);
                if (size < 0) {
                        LOG_ERR("error during sdu write (tcp): %d", size);
                        return -1;
                }
                total += size;
        }

        return 0;
}

static int tcp_udp_du_write(struct ipcp_instance_data * data,
                            port_id_t                   id,
                            struct du *                 du,
                            bool                        blocking)
{
        struct snd_data * snd_data;

        LOG_DBG("Callback on tcp_udp_sdu_write");

        spin_lock_bh(&snd_wq_lock);
        if (snd_wq_size == SEND_WQ_MAX_SIZE) {
        	spin_unlock_bh(&snd_wq_lock);
        	LOG_DBG("Output SDU queue is full, try later");
        	return -EAGAIN;
        }

        snd_data = rkmalloc(sizeof(*snd_data), GFP_ATOMIC);
        if (!snd_data) {
                LOG_ERR("Could not allocate snd_data");
                return -1;
        }

        snd_data->data = data;
        snd_data->id   = id;
        snd_data->du  = du;
        INIT_LIST_HEAD(&snd_data->list);

        list_add_tail(&snd_data->list, &snd_wq_data);
        snd_wq_size++;
        spin_unlock_bh(&snd_wq_lock);

        queue_work(snd_wq, &snd_work);
        return 0;
}

static int __tcp_udp_sdu_write(struct ipcp_instance_data * data,
                               port_id_t                   id,
                               struct du *                 du)
{
        struct shim_tcp_udp_flow * flow;
        int                        size;
	ssize_t                    slen;

        flow = find_flow_by_port(data, id);
        if (!flow) {
                LOG_ERR("Could not find flow with specified port-id");
                du_destroy(du);
                return -1;
        }

        spin_lock_bh(&data->lock);
        if (flow->port_id_state != PORT_STATE_ALLOCATED) {
                du_destroy(du);
                spin_unlock_bh(&data->lock);

                LOG_ERR("Flow is not in the right state to call this");

                return -1;
        }
        spin_unlock_bh(&data->lock);

	slen = du_len(du);
        if (flow->fspec_id == 0) {
                /* We are sending an UDP message */
                size = send_msg(flow->sock, &flow->addr, sizeof(flow->addr),
                                du_buffer(du),
                                slen);
                if (size < 0) {
                        LOG_ERR("Error during SDU write (udp): %d", size);
                        du_destroy(du);
                        return -1;
                } else if (size < slen) {
                        LOG_ERR("Could not completely send SDU");
                        du_destroy(du);
                        return -1;
                }
        } else {
                /* We are sending a TCP message */
                if (tcp_sdu_write(flow, slen,
                                  du_buffer(du))) {
                        LOG_ERR("Could not send SDU on TCP flow");
                        du_destroy(du);
                        return -1;
                }
        }

        LOG_DBG("SDU sent");
        du_destroy(du);

        return 0;
}

static void enable_all_flows(void)
{
	struct ipcp_instance_data * pos, *next;
	struct shim_tcp_udp_flow  * flow, *nflow;

	list_for_each_entry_safe(pos, next, &(tcp_udp_data.instances), list) {
		spin_lock_bh(&pos->lock);
		list_for_each_entry_safe(flow, nflow, &pos->flows, list) {
			if (flow->user_ipcp && flow->user_ipcp->ops)
				flow->user_ipcp->ops->enable_write(flow->user_ipcp->data,
								   flow->port_id);
		}
		spin_unlock_bh(&pos->lock);
	}
}

static void tcp_udp_write_worker(struct work_struct * w)
{
        struct snd_data           * snd_data, * next;

        /* FIXME: more efficient locking and better cleanup */
        spin_lock_bh(&snd_wq_lock);

        list_for_each_entry_safe(snd_data, next, &snd_wq_data, list) {
                list_del(&snd_data->list);
                snd_wq_size --;
                if (snd_wq_size == SEND_WQ_MAX_SIZE - 1) {
                	spin_unlock_bh(&snd_wq_lock);
                	enable_all_flows();
                	spin_lock_bh(&snd_wq_lock);
                }
                spin_unlock_bh(&snd_wq_lock);

                __tcp_udp_sdu_write(snd_data->data,
                                    snd_data->id,
                                    snd_data->du);

                rkfree(snd_data);

                spin_lock_bh(&snd_wq_lock);
        }
        spin_unlock_bh(&snd_wq_lock);

        LOG_DBG("Writer worker finished for now");
}

static const struct name * tcp_udp_ipcp_name(struct ipcp_instance_data * data)
{
        ASSERT(data);
        ASSERT(name_is_ok(data->name));

        return data->name;
}

static const struct name * tcp_udp_dif_name(struct ipcp_instance_data * data)
{
        ASSERT(data);
        ASSERT(name_is_ok(data->dif_name));

        return data->dif_name;
}

static size_t tcp_udp_max_sdu_size(struct ipcp_instance_data * data)
{
        ASSERT(data);

        /* FIXME: return a value that makes more sense */
        return 2000000;
}

ipc_process_id_t tcp_udp_ipcp_id(struct ipcp_instance_data * data)
{
	ASSERT(data);
	return data->id;
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
	.connection_modify 	   = NULL,

        .du_enqueue               = NULL,
        .du_write                 = tcp_udp_du_write,

        .mgmt_du_write            = NULL,
        .mgmt_du_post             = NULL,

        .pff_add                   = NULL,
        .pff_remove                = NULL,
        .pff_dump                  = NULL,
        .pff_flush                 = NULL,
	.pff_modify		   = NULL,

        .query_rib	           = tcp_udp_query_rib,

        .ipcp_name                 = tcp_udp_ipcp_name,
        .dif_name                  = tcp_udp_dif_name,
	.ipcp_id		   = tcp_udp_ipcp_id,

        .set_policy_set_param      = NULL,
        .select_policy_set         = NULL,
        .update_crypto_state	   = NULL,
	.address_change            = NULL,
        .dif_name		   = tcp_udp_dif_name,
	.max_sdu_size		   = tcp_udp_max_sdu_size
};

static int tcp_udp_init(struct ipcp_factory_data * data)
{
        ASSERT(data);

        bzero(&tcp_udp_data, sizeof(tcp_udp_data));
        INIT_LIST_HEAD(&(data->instances));

        INIT_LIST_HEAD(&rcv_wq_data);
        INIT_LIST_HEAD(&snd_wq_data);

        spin_lock_init(&data->lock);

        INIT_WORK(&rcv_work, tcp_udp_rcv_worker);
        INIT_WORK(&snd_work, tcp_udp_write_worker);

        snd_wq_size = 0;

        LOG_INFO("%s initialized", SHIM_NAME);

        return 0;
}

static int tcp_udp_fini(struct ipcp_factory_data * data)
{
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
                                             ipc_process_id_t           id,
					     uint_t			us_nl_port)
{
        struct ipcp_instance * inst;

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

        inst->data->host_name.family = AF_UNSPEC;

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
        struct ipcp_instance_data         * pos, * next;
        struct shim_tcp_udp_flow 	  * flow, * nflow;
        struct reg_app_data	 	  * reg_app, *nreg_app;

        ASSERT(data);
        ASSERT(instance);

        LOG_DBG("Looking for the instance to destroy");

        list_for_each_entry_safe(pos, next, &data->instances, list) {
                if (pos->id == instance->data->id) {
                        LOG_DBG("Instance is %pK", pos);

                        /* Destroy existing flows */
                        list_for_each_entry_safe(flow, nflow,
                        			 &pos->flows, list) {
                        	flow_deallocate(pos, flow);
                        }

                        /* Unregister existing applications */
                        list_for_each_entry_safe(reg_app, nreg_app,
                        			 &pos->reg_apps, list) {
                        	application_unregister(pos, reg_app);
                        }

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

	LOG_INFO("IRATI shim-tcp-udp module removed successfully");
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("RINA Shim IPC Process over TCP/UDP");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini@nextworks.it>");
MODULE_AUTHOR("Sander Vrijders <sander.vrijders@intec.ugent.be>");
MODULE_AUTHOR("Douwe De Bock <douwe.debock@ugent.be>");

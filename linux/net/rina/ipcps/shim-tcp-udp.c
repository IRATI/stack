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

#define PROTO_LEN   32
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

static struct workqueue_struct *rcv_wq;
static struct work_struct rcv_work;
static struct list_head rcv_wq_data;
static DEFINE_SPINLOCK(rcv_wq_lock);


extern struct kipcm * default_kipcm;

struct ipcp_instance_data {
	struct list_head list;
	ipc_process_id_t id;

	/* IPC Process name */
	struct name *name;
	struct name *dif_name;

	struct flow_spec **qos;

	/* Stores the state of flows indexed by port_id */
	spinlock_t flow_lock;
	struct list_head flows;

	spinlock_t app_lock;
	struct list_head apps;

	/* FIXME: Remove it as soon as the kipcm_kfa gets removed */
	struct kfa *kfa;
};

/* FIXME Remove this */
//static struct ipcp_instance_data *inst_data;

/*
   enum port_id_state {
   PORT_STATE_NULL = 1,
   PORT_STATE_PENDING,
   PORT_STATE_ALLOCATED
   };

*/
struct app_data {
	struct list_head list;

	struct socket *lsock;
	struct socket *udpsock;

	struct name *app_name;
	int port;
};

/*
   struct shim_tcp_udp_flow {
   struct list_head list;

   port_id_t port_id;
   enum port_id_state port_id_state;

   struct name *app_name;

   spinlock_t lock;

   int fspec_id;

   struct socket *sock;
   struct sockaddr_in addr;

   struct rfifo *sdu_queue;
   };

*/
struct rcv_data {
	struct list_head list;
	struct sock *sk;
};
/*

   static struct shim_tcp_udp_flow * find_flow(struct ipcp_instance_data * data,
   port_id_t                   id)
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
   find_tcp_flow_by_socket(struct ipcp_instance_data *data,
   const struct socket *sock)
   {
   struct shim_tcp_udp_flow *flow;

   if (!data)
   return NULL;

   list_for_each_entry(flow, &data->flows, list) {
   if (flow->sock == sock) {
   return flow;
   }
   }

   return NULL;
   }

   static int compare_addr(const struct sockaddr_in *f,
   const struct sockaddr_in *s)
   {
   if (f->sin_family == s->sin_family &&
   f->sin_port == s->sin_port &&
   f->sin_addr.s_addr == s->sin_addr.s_addr)
   return 1;
   else
   return 0;
   }

   static struct shim_tcp_udp_flow *
   find_udp_flow(struct ipcp_instance_data *data,
   const struct sockaddr_in *addr,
   const struct socket *sock)
   {
   struct shim_tcp_udp_flow *flow;

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

   static struct app_data * find_app_by_socket(struct ipcp_instance_data *data,
const struct socket *sock)
{
	struct app_data *app;

	if (!data)
		return NULL;

	list_for_each_entry(app, &data->apps, list) {
		if (app->lsock == sock) {
			return app;
		}
	}

	return NULL;
}

	*/
static struct app_data * find_app_by_name(struct ipcp_instance_data *data,
		const struct name *name)
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
/*

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
   if (kfa_flow_deallocate(data->kfa, flow->port_id))
   LOG_ERR("Failed to destroy KFA flow");
   if (flow_destroy(data, flow))
   LOG_ERR("Failed to destroy shim-tcp-udp flow");
   }
   */

static int tcp_udp_flow_allocate_request(struct ipcp_instance_data * data,
		const struct name *        source,
		const struct name *        dest,
		const struct flow_spec *   fspec,
		port_id_t                  id)
{
	LOG_DBG("tcp_udp allocate request");
	ASSERT(data);
	ASSERT(source);
	ASSERT(dest);

	return 0;
}

static int tcp_udp_flow_allocate_response(struct ipcp_instance_data * data,
		port_id_t                  port_id,
		int                        result)
{
	LOG_DBG("tcp_udp allocate response");
	ASSERT(data);
	ASSERT(is_port_id_ok(port_id));

	return 0;
}

static int tcp_udp_flow_deallocate(struct ipcp_instance_data * data,
		port_id_t                  id)
{
	LOG_DBG("tcp_udp deallocate");
	ASSERT(data);

	return 0;
}

/*
   int recv_msg(struct socket *sock, struct sockaddr_in *other,
   unsigned char *buf, int len) 
   {
   return 0;
   }

   int send_msg(struct socket *sock, struct sockaddr_in *other, char *buf, int len) 
   {
   return 0;
   }

*/
static int tcp_udp_rcv_process_msg(struct sock *sk)
{
	return 0;
}

static void tcp_udp_rcv_worker(struct work_struct *work)
{
	struct rcv_data *recvd, *next;
	unsigned long flags;

	spin_lock_irqsave(&rcv_wq_lock, flags);
	list_for_each_entry_safe(recvd, next, &rcv_wq_data, list) {
		spin_unlock_irqrestore(&rcv_wq_lock, flags);

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

static void tcp_udp_rcv(struct sock *sk, int bytes)
{
	struct rcv_data *recvd;

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

	err = sock_create_kern(PF_INET, SOCK_DGRAM, IPPROTO_UDP, &app->udpsock);
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
		LOG_ERR("could not bind socket for registration");
		name_destroy(app->app_name);
		sock_release(app->udpsock);
		rkfree(app);
		return -1;
	}

	write_lock_bh(&app->udpsock->sk->sk_callback_lock);
	app->udpsock->sk->sk_user_data = app->udpsock->sk->sk_data_ready;
	app->udpsock->sk->sk_data_ready = tcp_udp_rcv;
	write_unlock_bh(&app->udpsock->sk->sk_callback_lock);

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

	lock_sock(app->udpsock->sk);
	write_lock_bh(&app->udpsock->sk->sk_callback_lock);
	app->udpsock->sk->sk_data_ready = app->udpsock->sk->sk_user_data;
	app->udpsock->sk->sk_user_data = NULL;
	write_unlock_bh(&app->udpsock->sk->sk_callback_lock);
	release_sock(app->udpsock->sk);

	sock_release(app->udpsock);
	name_destroy(app->app_name);

	spin_lock(&data->app_lock);
	list_del(&app->list);
	spin_unlock(&data->app_lock);

	rkfree(app);

	return 0;
}

static int tcp_udp_assign_to_dif(struct ipcp_instance_data *data,
		const struct dif_info *dif_information)
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

static int tcp_udp_update_dif_config(struct ipcp_instance_data *data,
		const struct dif_config *new_config)
{
	LOG_DBG("tcp_udp update dif config");
	ASSERT(data);
	ASSERT(new_config);

	return 0;
}

static int tcp_udp_sdu_write(struct ipcp_instance_data *data,
		port_id_t id,
		struct sdu *sdu)
{
	LOG_DBG("tcp_udp sdu write");
	ASSERT(data);
	ASSERT(sdu);

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

static struct ipcp_instance_data * find_instance(
		struct ipcp_factory_data *data, ipc_process_id_t id)
{

	struct ipcp_instance_data * pos;

	list_for_each_entry(pos, &(data->instances), list) {
		if (pos->id == id) {
			return pos;
		}
	}

	return NULL;

}

static struct ipcp_instance * tcp_udp_create(struct ipcp_factory_data *data,
		const struct name *name,
		ipc_process_id_t id)
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
	inst->data->qos[0]->max_sdu_size                = 512;
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
	inst->data->qos[1]->max_sdu_size                = 512;
	inst->data->qos[1]->ordered_delivery            = 1;
	inst->data->qos[1]->partial_delivery            = 0;
	inst->data->qos[1]->peak_bandwidth_duration     = 0;
	inst->data->qos[1]->peak_sdu_bandwidth_duration = 0;
	inst->data->qos[1]->undetected_bit_error_rate   = 0;

	/* FIXME: Remove as soon as the kipcm_kfa gets removed*/
	inst->data->kfa = kipcm_kfa(default_kipcm);

	ASSERT(inst->data->kfa);

	LOG_DBG("KFA instance %pK bound to the shim tcp-udp", inst->data->kfa);

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
	.init      = tcp_udp_init,
	.fini      = tcp_udp_fini,
	.create    = tcp_udp_create,
	.destroy   = tcp_udp_destroy,
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

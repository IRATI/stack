/*
 * K-IPCM (Kernel-IPC Manager)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
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

#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/export.h>
#include <linux/kfifo.h>

#define RINA_PREFIX "kipcm"

#include "logs.h"
#include "utils.h"
#include "kipcm.h"
#include "debug.h"
#include "ipcp-factories.h"
#include "ipcp-utils.h"
#include "kipcm-utils.h"
#include "common.h"
#include "du.h"
#include "netlink.h"
#include "netlink-utils.h"

#define DEFAULT_FACTORY "normal-ipc"

struct kipcm {
        spinlock_t              lock;
        struct ipcp_factories * factories;
        struct ipcp_imap *      instances;
        struct ipcp_fmap *      flows;
        struct rina_nl_set *    set;
};

#ifdef RINA_KIPCM_LOCKS_DEBUG

#define KIPCM_LOCK_HEADER(X)   do {                             \
                LOG_DBG("KIPCM instance %pK locking ...", X);   \
        } while (0)
#define KIPCM_LOCK_FOOTER(X)   do {                             \
                LOG_DBG("KIPCM instance %pK locked", X);        \
        } while (0)

#define KIPCM_UNLOCK_HEADER(X) do {                             \
                LOG_DBG("KIPCM instance %pK unlocking ...", X); \
        } while (0)
#define KIPCM_UNLOCK_FOOTER(X) do {                             \
                LOG_DBG("KIPCM instance %pK unlocked", X);      \
        } while (0)

#else

#define KIPCM_LOCK_HEADER(X)   do { } while (0)
#define KIPCM_LOCK_FOOTER(X)   do { } while (0)
#define KIPCM_UNLOCK_HEADER(X) do { } while (0)
#define KIPCM_UNLOCK_FOOTER(X) do { } while (0)

#endif

/* Disable locking only in case of REAL necessity */
#define DISABLE_LOCKING 0

#if DISABLE_LOCKING

#define KIPCM_LOCK_INIT(X) do {                                 \
                LOG_DBG("KIPCM instance locking disabled");     \
        } while(0);
#define KIPCM_LOCK(X)      do { } while (0);
#define KIPCM_UNLOCK(X)    do { } while (0);

#else

#define KIPCM_LOCK_INIT(X) spin_lock_init(&(X -> lock));

#define KIPCM_LOCK(X)   do {                    \
                KIPCM_LOCK_HEADER(X);           \
                spin_lock(&(X -> lock));        \
                KIPCM_LOCK_FOOTER(X);           \
        } while (0)
#define KIPCM_UNLOCK(X) do {                    \
                KIPCM_UNLOCK_HEADER(X);         \
                spin_unlock(&(X -> lock));      \
                KIPCM_UNLOCK_FOOTER(X);         \
        } while (0)

#endif

struct ipcp_flow {
        /* The port-id identifying the flow */
        port_id_t              port_id;

        /*
         * The components of the IPC Process that will handle the
         * write calls to this flow
         */
        struct ipcp_instance * ipc_process;

        /*
         * True if this flow is serving a user-space application, false
         * if it is being used by an RMT
         */
        bool_t                 application_owned;

        /*
         * In case this flow is being used by an RMT, this is a pointer
         * to the RMT instance.
         */
        struct rmt_instance *  rmt_instance;

        //FIXME: Define QUEUE
        //QUEUE(segmentation_queue, pdu *);
        //QUEUE(reassembly_queue,       pdu *);
        //QUEUE(sdu_ready, sdu *);
        struct kfifo           sdu_ready;
};

static int notify_ipcp_allocate_flow_request(void *             data,
					     struct sk_buff *   buff,
					     struct genl_info * info)
{
	struct rnl_ipcm_alloc_flow_req_msg_attrs * msg_attrs;
	struct rnl_msg * 			   msg;
	struct ipcp_instance * 			   ipc_process;
	ipc_process_id_t 			   ipc_id;
	struct rina_msg_hdr *                      hdr;
	struct kipcm * kipcm;
	struct name * source;
	struct name * dest;
	struct name * dif_name;

	if (!data) {
		LOG_ERR("Bogus kipcm instance passed, cannot parse NL msg");
		return -1;
	}

	if (!info) {
		LOG_ERR("Bogus struct genl_info passed, cannot parse NL msg");
		return -1;
	}

	kipcm = (struct kipcm *) data;
	msg_attrs = rkzalloc(sizeof(*msg_attrs), GFP_KERNEL);
	if (!msg_attrs) {
		if (rnl_app_alloc_flow_result_msg(0, 0, -1, info->snd_seq))
			return -1;

		return 0;
	}
	source = name_create();
	if (!source) {
		if (rnl_app_alloc_flow_result_msg(0, 0, -1, info->snd_seq)) {
			rkfree(msg_attrs);
			return -1;
		}

		return 0;
	}
	dest = name_create();
	if (!dest) {
		if (rnl_app_alloc_flow_result_msg(0, 0, -1, info->snd_seq)) {
			rkfree(msg_attrs);
			name_destroy(source);
			return -1;
		}

		return 0;
	}
	dif_name = name_create();
	if (!dif_name) {
		if (rnl_app_alloc_flow_result_msg(0, 0, -1, info->snd_seq)) {
			rkfree(msg_attrs);
			name_destroy(source);
			name_destroy(dest);
			return -1;
		}

		return 0;
	}

	msg = rkzalloc(sizeof(*msg), GFP_KERNEL);
	if (!msg) {
		rkfree(msg_attrs);
		name_destroy(source);
		name_destroy(dest);
		if (rnl_app_alloc_flow_result_msg(0, 0, -1, info->snd_seq))
			return -1;

		return 0;
	}
	hdr = rkzalloc(sizeof(*hdr), GFP_KERNEL);
	if (!hdr) {
		rkfree(msg_attrs);
		name_destroy(source);
		name_destroy(dest);
		rkfree(msg);
		if (rnl_app_alloc_flow_result_msg(0, 0, -1, info->snd_seq))
			return -1;

		return 0;
	}
	msg->attrs = msg_attrs;
	msg->rina_hdr = hdr;

	if (rnl_parse_msg(info, msg)) {
		if (rnl_app_alloc_flow_result_msg(0, 0, -1, info->snd_seq))
			return -1;

		return 0;
	}
	ipc_id = msg->rina_hdr->dst_ipc_id;
	ipc_process = ipcp_imap_find(kipcm->instances, ipc_id);
	if (!ipc_process) {
		LOG_ERR("IPC process %d not found", ipc_id);
		rkfree(hdr);
		rkfree(msg_attrs);
		rkfree(msg);
		if (rnl_app_alloc_flow_result_msg(ipc_id,
                                              msg->rina_hdr->src_ipc_id, -1,
                                              info->snd_seq))
			return -1;

		return 0;
	}
	if (ipc_process->ops->flow_allocate_request(ipc_process->data,
                                                    msg_attrs->source,
                                                    msg_attrs->dest,
                                                    msg_attrs->fspec,
                                                    msg_attrs->id)) {
		LOG_ERR("Failed allocate flow request for port id: %d",
                        msg_attrs->id);
		if (rnl_app_alloc_flow_result_msg(ipc_id,
                                              msg->rina_hdr->src_ipc_id, -1,
                                              info->snd_seq)) {
			rkfree(hdr);
			rkfree(msg_attrs);
			rkfree(msg);
			return -1;
		}

		rkfree(hdr);
		rkfree(msg_attrs);
		rkfree(msg);
		return 0;
	}

	if (rnl_app_alloc_flow_req_arrived_msg(ipc_process->data,
                                               msg_attrs->source,
                                               msg_attrs->dest,
                                               msg_attrs->fspec,
                                               msg_attrs->id,
                                               info->snd_seq)) {
		rkfree(hdr);
		rkfree(msg_attrs);
		rkfree(msg);
		return -1;
	}
	rkfree(hdr);
	rkfree(msg_attrs);
	rkfree(msg);
        
	return 0;
}

static int notify_ipcp_allocate_flow_response(void *             data,
					      struct sk_buff *   buff,
					      struct genl_info * info)
{
	struct kipcm *                         kipcm;
	struct rnl_alloc_flow_resp_msg_attrs * msg_attrs;
	struct rnl_msg * 		       msg;
	struct rina_msg_hdr * 		       hdr;
	struct ipcp_instance * 		       ipc_process;
	ipc_process_id_t 		       ipc_id;
	int 				       retval = 0;
	response_reason_t  		       reason;

	if (!data) {
		LOG_ERR("Bogus kipcm instance passed, cannot parse NL msg");
		return -1;
	}

	kipcm = (struct kipcm *) data;

	if (!info) {
		LOG_ERR("Bogus struct genl_info passed, cannot parse NL msg");
		return -1;
	}
	msg_attrs = rkzalloc(sizeof(*msg_attrs), GFP_KERNEL);
	if (!msg_attrs)
		return -1;
	msg = rkzalloc(sizeof(*msg), GFP_KERNEL);
	if (!msg) {
		rkfree(msg_attrs);
		return -1;
	}
	hdr = rkzalloc(sizeof(*hdr), GFP_KERNEL);
	if (!hdr) {
		rkfree(msg_attrs);
		rkfree(msg);
		return -1;
	}
	msg->attrs = msg_attrs;
	msg->rina_hdr = hdr;

	if (rnl_parse_msg(info, msg)) {
		rkfree(hdr);
		rkfree(msg_attrs);
		rkfree(msg);
		return -1;
	}
	ipc_id = msg->rina_hdr->src_ipc_id;
	ipc_process = ipcp_imap_find(kipcm->instances, ipc_id);
	if (!ipc_process) {
		LOG_ERR("IPC process %d not found", ipc_id);
		rkfree(hdr);
		rkfree(msg_attrs);
		rkfree(msg);
		return -1;
	}
	reason = (response_reason_t) msg_attrs->result;
	if (ipc_process->ops->flow_allocate_response(ipc_process->data,
                                                     msg_attrs->id, &reason)) {
		LOG_ERR("Failed allocate flow request for port id: %d",
                        msg_attrs->id);
		retval = -1;
	}

	rkfree(hdr);
	rkfree(msg_attrs);
	rkfree(msg);

	return retval;
}

static int notify_ipcp_assign_dif_request(void *             data,
				  	  struct sk_buff *   buff,
				  	  struct genl_info * info)
{
	struct kipcm *                                kipcm;
	struct rnl_ipcm_assign_to_dif_req_msg_attrs * attrs;
	struct rnl_msg * 			      msg;
	struct dif_config *			      dif_config;
	struct name * 				      dif_name;
	struct ipcp_instance * 		       	      ipc_process;
	ipc_process_id_t 		      	      ipc_id;

	if (!data) {
		LOG_ERR("Bogus kipcm instance passed, cannot parse NL msg");
		return -1;
	}

	kipcm = (struct kipcm *) data;

	if (!info) {
		LOG_ERR("Bogus struct genl_info passed, cannot parse NL msg");
		return -1;
	}

	attrs = rkzalloc(sizeof(*attrs), GFP_KERNEL);
	if (!attrs) {
		rnl_assign_dif_response(0, -1, info->snd_seq);
		return 0;
	}

	dif_config = rkzalloc(sizeof(struct dif_config), GFP_KERNEL);
	if (!dif_config){
		rkfree(attrs);
		rnl_assign_dif_response(0, -1, info->snd_seq);
		return 0;
	}
	attrs->dif_config = dif_config;

	dif_name = name_create();
	if (!dif_name){
		rkfree(dif_config);
		rkfree(attrs);
		rnl_assign_dif_response(0, -1, info->snd_seq);
		return 0;
	}
	dif_config->dif_name = dif_name;

	msg = rkzalloc(sizeof(*msg), GFP_KERNEL);
	if (!msg) {
		//name_destroy(dif_name);
		rkfree(dif_config);
		rkfree(attrs);
		rnl_assign_dif_response(0, -1, info->snd_seq);
		return 0;
	}
	msg->attrs = attrs;

	if (rnl_parse_msg(info, msg)) {
		//name_destroy(dif_name);
		rkfree(dif_config);
		rkfree(attrs);
		rkfree(msg);
		rnl_assign_dif_response(0, -1, info->snd_seq);
		return 0;
	}
	ipc_id = msg->rina_hdr->dst_ipc_id;

	ipc_process = ipcp_imap_find(kipcm->instances, ipc_id);
	if (!ipc_process) {
		LOG_ERR("IPC process %d not found", ipc_id);
		//name_destroy(dif_name);
		rkfree(dif_config);
		rkfree(attrs);
		rkfree(msg);
		rnl_assign_dif_response(0, -1, info->snd_seq);
		return 0;
	}
	LOG_DBG("Found IPC Process with id %d", ipc_id);

	if (ipc_process->ops->assign_to_dif(ipc_process->data,
			attrs->dif_config->dif_name)) {
		char * tmp = name_tostring(attrs->dif_config->dif_name);
		LOG_ERR("Failed assign to dif %s for IPC process: %d",
                        tmp, ipc_id);
		rkfree(tmp);
		//name_destroy(dif_name);
		rkfree(dif_config);
		rkfree(attrs);
		rkfree(msg);
		rnl_assign_dif_response(0, -1, info->snd_seq);
		return 0;
	}
	LOG_DBG("Assign to DIF operation successful, freeing memory");

	LOG_DBG("Calling assign to DIF response");
	if (rnl_assign_dif_response(ipc_id, 0, info->snd_seq)){
		name_destroy(dif_name);
		rkfree(dif_config);
		rkfree(attrs);
		rkfree(msg);
		return -1;
	}

	//name_destroy(dif_name);
	rkfree(dif_config);
	rkfree(attrs);
	rkfree(msg);
	LOG_DBG("Finishing successfully");
	return 0;
}

static int notify_ipcp_register_app_request(void *             data,
					    struct sk_buff *   buff,
					    struct genl_info * info)
{
	struct kipcm * 				kipcm;
	struct rnl_ipcm_reg_app_req_msg_attrs * attrs;
	struct rnl_msg * 			msg;
	struct name * 				app_name;
	struct name * 				dif_name;
	struct ipcp_instance * 			ipc_process;
	ipc_process_id_t 			ipc_id;

	if (!data) {
		LOG_ERR("Bogus kipcm instance passed, cannot parse NL msg");
		return -1;
	}

	kipcm = (struct kipcm *) data;

	if (!info) {
		LOG_ERR("Bogus struct genl_info passed, cannot parse NL msg");
		return -1;
	}

	attrs = rkzalloc(sizeof(*attrs), GFP_KERNEL);
	if (!attrs) {
		if (rnl_app_register_response_msg(0, 0, -1, info->snd_seq))
			return -1;

		return 0;
	}

	app_name = name_create();
	if (!app_name) {
		rkfree(attrs);
		if (rnl_app_register_response_msg(0, 0, -1, info->snd_seq))
			return -1;

		return 0;
	}
	attrs->app_name= app_name;

	dif_name = name_create();
	if (!dif_name) {
		//name_destroy(app_name);
		rkfree(attrs);
		if (rnl_app_register_response_msg(0, 0, -1, info->snd_seq))
			return -1;

		return 0;
	}
	attrs->dif_name= dif_name;

	msg = rkzalloc(sizeof(*msg), GFP_KERNEL);
	if (!msg) {
		LOG_ERR("Could not allocate space for my_msg struct");
		//name_destroy(app_name);
		//name_destroy(dif_name);
		rkfree(attrs);
		if (rnl_app_register_response_msg(0, 0, -1, info->snd_seq))
			return -1;

		return 0;
	}
	msg->attrs = attrs;
	if (rnl_parse_msg(info, msg)) {
		LOG_ERR("Could not parse message");
		//name_destroy(app_name);
		//name_destroy(dif_name);
		rkfree(attrs);
		rkfree(msg);
		if (rnl_app_register_response_msg(0, 0, -1, info->snd_seq))
			return -1;
		return 0;
	}
	ipc_id = msg->rina_hdr->dst_ipc_id;
	ipc_process = ipcp_imap_find(kipcm->instances, ipc_id);
	if (!ipc_process) {
		LOG_ERR("IPC process %d not found", ipc_id);
		if (rnl_app_register_response_msg(ipc_id,
                                              msg->rina_hdr->src_ipc_id, -1,
                                              info->snd_seq)) {
			name_destroy(app_name);
			name_destroy(dif_name);
			rkfree(attrs);
			rkfree(msg);
			return -1;
		}
		//name_destroy(app_name);
		//name_destroy(dif_name);
		rkfree(attrs);
		rkfree(msg);
		return 0;
	}

	if (ipc_process->ops->application_register(data, attrs->app_name)) {
		if (rnl_app_register_response_msg(ipc_id,
                                              msg->rina_hdr->src_ipc_id, -1,
                                              info->snd_seq)) {
			name_destroy(app_name);
			name_destroy(dif_name);
			rkfree(attrs);
			rkfree(msg);
			return -1;
		}
		//name_destroy(app_name);
		//name_destroy(dif_name);
		rkfree(attrs);
		rkfree(msg);
		return 0;
	}

	if (rnl_app_register_response_msg(ipc_id, msg->rina_hdr->src_ipc_id, 0,
                        info->snd_seq)) {
		name_destroy(app_name);
		name_destroy(dif_name);
		rkfree(attrs);
		rkfree(msg);
		return -1;
	}

	//name_destroy(app_name);
	//name_destroy(dif_name);
	rkfree(attrs);
	rkfree(msg);

	return 0;
}

static int netlink_handlers_unregister(struct rina_nl_set * set)
{
	int retval = 0;

	if (rina_netlink_handler_unregister(set,
				    RINA_C_IPCM_ALLOCATE_FLOW_REQUEST))
		retval = -1;

	if (rina_netlink_handler_unregister(set,
				    RINA_C_IPCM_ALLOCATE_FLOW_RESPONSE))
		retval = -1;

	if (rina_netlink_handler_unregister(set,
				    RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST))
		retval = -1;

	if (rina_netlink_handler_unregister(set,
				    RINA_C_IPCM_REGISTER_APPLICATION_REQUEST))
		retval = -1;

	return retval;
}

static int netlink_handlers_register(struct kipcm * kipcm)
{
	message_handler_cb handler;

	handler = notify_ipcp_assign_dif_request;
	if (rina_netlink_handler_register(kipcm->set,
                                          RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST,
                                          kipcm,
                                          handler))
		return -1;

	handler = notify_ipcp_allocate_flow_request;
	if (rina_netlink_handler_register(kipcm->set,
                                          RINA_C_IPCM_ALLOCATE_FLOW_REQUEST,
                                          kipcm,
                                          handler)) {
		if (rina_netlink_handler_unregister(kipcm->set,
                                                    RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST)) {
			LOG_ERR("Failed handler unregister while bailing out");
			/* FIXME: What else could be done here?" */
		}
		return -1;
	}

	handler = notify_ipcp_allocate_flow_response;
	if (rina_netlink_handler_register(kipcm->set,
                                          RINA_C_IPCM_ALLOCATE_FLOW_RESPONSE,
                                          kipcm,
                                          handler)) {
		if (rina_netlink_handler_unregister(kipcm->set,
                                                    RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST)) {
			LOG_ERR("Failed handler unregister while bailing out");
			/* FIXME: What else could be done here?" */
		}
		if (rina_netlink_handler_unregister(kipcm->set,
                                                    RINA_C_IPCM_ALLOCATE_FLOW_REQUEST)) {
			LOG_ERR("Failed handler unregister while bailing out");
			/* FIXME: What else could be done here?" */
		}
		return -1;
	}
	handler = notify_ipcp_register_app_request;
	if (rina_netlink_handler_register(kipcm->set,
                                          RINA_C_IPCM_REGISTER_APPLICATION_REQUEST,
                                          kipcm,
                                          handler)) {
		if (rina_netlink_handler_unregister(kipcm->set,
                                                    RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST)) {
			LOG_ERR("Failed handler unregister while bailing out");
			/* FIXME: What else could be done here?" */
		}
		if (rina_netlink_handler_unregister(kipcm->set,
                                                    RINA_C_IPCM_ALLOCATE_FLOW_REQUEST)) {
			LOG_ERR("Failed handler unregister while bailing out");
			/* FIXME: What else could be done here?" */
		}
		if (rina_netlink_handler_unregister(kipcm->set,
                                                    RINA_C_IPCM_ALLOCATE_FLOW_RESPONSE)) {
			LOG_ERR("Failed handler unregister while bailing out");
			/* FIXME: What else could be done here?" */
		}
		return -1;
	}


	return 0;
}

struct kipcm * kipcm_init(struct kobject * parent, struct rina_nl_set * set)
{
        struct kipcm * tmp;

        LOG_DBG("Initializing");

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->factories = ipcpf_init(parent);
        if (!tmp->factories) {
                rkfree(tmp);
                return NULL;
        }

        tmp->instances = ipcp_imap_create();
        if (!tmp->instances) {
                if (ipcpf_fini(tmp->factories)) {
                        /* FIXME: What could we do here ? */
                }
                rkfree(tmp);
                return NULL;
        }

        tmp->flows = ipcp_fmap_create();
        if (!tmp->flows) {
                if (ipcp_imap_destroy(tmp->instances)) {
                        /* FIXME: What could we do here ? */
                }
                if (ipcpf_fini(tmp->factories)) {
                        /* FIXME: What could we do here ? */
                }
                rkfree(tmp);
                return NULL;
        }

        LOG_DBG("Registering set");
        if (rina_netlink_set_register(set)) {
        	if (ipcp_imap_destroy(tmp->instances)) {
			/* FIXME: What could we do here ? */
		}
		if (ipcp_fmap_destroy(tmp->flows)) {
			/* FIXME: What could we do here ? */
		}
		if (ipcpf_fini(tmp->factories)) {
			/* FIXME: What could we do here ? */
		}
		rkfree(tmp);
		return NULL;
	}
        tmp->set = set;
        LOG_DBG("Registering handlers");
        if (netlink_handlers_register(tmp)) {
        	if (ipcp_imap_destroy(tmp->instances)) {
			/* FIXME: What could we do here ? */
		}
        	if (ipcp_fmap_destroy(tmp->flows)) {
			/* FIXME: What could we do here ? */
		}
		if (ipcpf_fini(tmp->factories)) {
			/* FIXME: What could we do here ? */
		}
		rkfree(tmp);
		return NULL;
        }

        KIPCM_LOCK_INIT(tmp);

        LOG_DBG("Initialized successfully");

        return tmp;
}

int kipcm_fini(struct kipcm * kipcm)
{
        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return -1;
        }

        LOG_DBG("Finalizing");

        KIPCM_LOCK(kipcm);

        /* FIXME: Destroy all the flows */
        ASSERT(ipcp_fmap_empty(kipcm->flows));
        if (ipcp_fmap_destroy(kipcm->flows)) {
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        /* FIXME: Destroy all the instances */
        ASSERT(ipcp_imap_empty(kipcm->instances));
        if (ipcp_imap_destroy(kipcm->instances)) {
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        if (ipcpf_fini(kipcm->factories)) {
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        if (netlink_handlers_unregister(kipcm->set)) {
        	KIPCM_UNLOCK(kipcm);
		return -1;
        }

        KIPCM_UNLOCK(kipcm);

        rkfree(kipcm);

        LOG_DBG("Finalized successfully");

        return 0;
}

struct ipcp_factory *
kipcm_ipcp_factory_register(struct kipcm *             kipcm,
                            const  char *              name,
                            struct ipcp_factory_data * data,
                            struct ipcp_factory_ops *  ops)
{
        struct ipcp_factory * retval;

        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return NULL;
        }

        KIPCM_LOCK(kipcm);
        retval = ipcpf_register(kipcm->factories, name, data, ops);
        KIPCM_UNLOCK(kipcm);

        return retval;
}
EXPORT_SYMBOL(kipcm_ipcp_factory_register);

int kipcm_ipcp_factory_unregister(struct kipcm *        kipcm,
                                  struct ipcp_factory * factory)
{
        int retval;

        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return -1;
        }

        /* FIXME:
         *
         *   We have to do the body of kipcm_ipcp_destroy() on all the
         *   instances remaining (and not explicitly destroyed), previously
         *   created with the factory being unregisterd ...
         *
         *     Francesco
         */
        KIPCM_LOCK(kipcm);
        retval = ipcpf_unregister(kipcm->factories, factory);
        KIPCM_UNLOCK(kipcm);

        return retval;
}
EXPORT_SYMBOL(kipcm_ipcp_factory_unregister);

int kipcm_ipcp_create(struct kipcm *      kipcm,
                      const struct name * ipcp_name,
                      ipc_process_id_t    id,
                      const char *        factory_name)
{
        char *                 name;
        struct ipcp_factory *  factory;
        struct ipcp_instance * instance;

        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return -1;
        }

        if (!factory_name)
                factory_name = DEFAULT_FACTORY;

        name = name_tostring(ipcp_name);
        if (!name)
                return -1;

        ASSERT(ipcp_name);
        ASSERT(factory_name);

        LOG_DBG("Creating IPC process:");
        LOG_DBG("  name:      %s", name);
        LOG_DBG("  id:        %d", id);
        LOG_DBG("  factory:   %s", factory_name);
        rkfree(name);

        KIPCM_LOCK(kipcm);
        if (ipcp_imap_find(kipcm->instances, id)) {
                LOG_ERR("Process id %d already exists", id);
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        factory = ipcpf_find(kipcm->factories, factory_name);
        if (!factory) {
                LOG_ERR("Cannot find factory '%s'", factory_name);
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        instance = factory->ops->create(factory->data, id);
        if (!instance) {
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        /* FIXME: Ugly as hell */
        instance->factory = factory;

        if (ipcp_imap_add(kipcm->instances, id, instance)) {
                factory->ops->destroy(factory->data, instance);
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        KIPCM_UNLOCK(kipcm);

        return 0;
}

int kipcm_ipcp_destroy(struct kipcm *  kipcm,
                       ipc_process_id_t id)
{
        struct ipcp_instance * instance;
        struct ipcp_factory *  factory;

        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return -1;
        }

        KIPCM_LOCK(kipcm);

        instance = ipcp_imap_find(kipcm->instances, id);
        if (!instance) {
                LOG_ERR("IPC process %d instance does not exist", id);
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        factory = instance->factory;
        ASSERT(factory);

        if (ipcp_fmap_remove_all_for_id(kipcm->flows, id)) {
		KIPCM_UNLOCK(kipcm);
		return -1;
	}

        if (factory->ops->destroy(factory->data, instance)) {
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        if (ipcp_imap_remove(kipcm->instances, id)) {
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        KIPCM_UNLOCK(kipcm);
        
        return 0;
}

int kipcm_ipcp_configure(struct kipcm *             kipcm,
                         ipc_process_id_t           id,
                         const struct ipcp_config * configuration)
{
        struct ipcp_instance * instance_old;
        struct ipcp_factory *  factory;
        struct ipcp_instance * instance_new;

        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return -1;
        }

        KIPCM_LOCK(kipcm);

        instance_old = ipcp_imap_find(kipcm->instances, id);
        if (instance_old == NULL) {
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        factory = instance_old->factory;
        ASSERT(factory);

        instance_new = factory->ops->configure(factory->data,
                                               instance_old,
                                               configuration);
        if (!instance_new) {
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        if (instance_new != instance_old)
                if (ipcp_imap_update(kipcm->instances, id, instance_new)) {
                        KIPCM_UNLOCK(kipcm);
                        return -1;
                }

        KIPCM_UNLOCK(kipcm);

        return 0;
}

int kipcm_flow_add(struct kipcm *   kipcm,
                   ipc_process_id_t ipc_id,
                   port_id_t        port_id)
{
        struct ipcp_flow * flow;

        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return -1;
        }

        KIPCM_LOCK(kipcm);

        if (ipcp_fmap_find(kipcm->flows, port_id)) {
                LOG_ERR("Flow on port-id %d already exists", port_id);
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        flow = rkzalloc(sizeof(*flow), GFP_KERNEL);
        if (!flow) {
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        flow->port_id     = port_id;
        flow->ipc_process = ipcp_imap_find(kipcm->instances, ipc_id);
        if (!flow->ipc_process) {
                LOG_ERR("Couldn't find the ipc process %d", ipc_id);
                rkfree(flow);
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        /*
         * FIXME: We are allowing applications, this must be changed once
         *        the RMT is implemented.
         */
        flow->application_owned = 1;
        flow->rmt_instance      = NULL;
        if (kfifo_alloc(&flow->sdu_ready, PAGE_SIZE, GFP_KERNEL)) {
                LOG_ERR("Couldn't create the sdu-ready queue for "
                        "flow on port-id %d", port_id);
                rkfree(flow);
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        if (ipcp_fmap_add(kipcm->flows, port_id, flow, ipc_id)) {
                kfifo_free(&flow->sdu_ready);
                rkfree(flow);
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        KIPCM_UNLOCK(kipcm);

        return 0;
}
EXPORT_SYMBOL(kipcm_flow_add);

int kipcm_flow_remove(struct kipcm * kipcm,
                      port_id_t      port_id)
{
        struct ipcp_flow * flow;

        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return -1;
        }

        KIPCM_LOCK(kipcm);

        flow = ipcp_fmap_find(kipcm->flows, port_id);
        if (!flow) {
                LOG_ERR("Couldn't retrieve the flow for port-id %d", port_id);
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        kfifo_free(&flow->sdu_ready);
        rkfree(flow);

        if (ipcp_fmap_remove(kipcm->flows, port_id)) {
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        KIPCM_UNLOCK(kipcm);

        return 0;
}
EXPORT_SYMBOL(kipcm_flow_remove);

int kipcm_sdu_write(struct kipcm * kipcm,
                    port_id_t      port_id,
                    struct sdu *   sdu)
{
        struct ipcp_flow *     flow;
        struct ipcp_instance * instance;

        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return -1;
        }

        if (!sdu || !sdu_is_ok(sdu)) {
                LOG_ERR("Bogus SDU received, bailing out");
                return -1;
        }

        LOG_DBG("SDU received (size %zd)", sdu->buffer->size);

        KIPCM_LOCK(kipcm);

        flow = ipcp_fmap_find(kipcm->flows, port_id);
        if (!flow) {
                LOG_ERR("There is no flow bound to port-id %d", port_id);
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        instance = flow->ipc_process;
        ASSERT(instance);
        if (instance->ops->sdu_write(instance->data, port_id, sdu)) {
                LOG_ERR("Couldn't write SDU on port-id %d", port_id);
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        KIPCM_UNLOCK(kipcm);

        /* The SDU is ours */
        return 0;
}

int kipcm_sdu_read(struct kipcm * kipcm,
                   port_id_t      port_id,
                   struct sdu **  sdu)
{
        struct ipcp_flow * flow;
        size_t             size;
        char *             data;

        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, bailing out");
                return -1;
        }
        if (!sdu) {
                LOG_ERR("Bogus parameters passed, bailing out");
                return -1;
        }

        KIPCM_LOCK(kipcm);

        flow = ipcp_fmap_find(kipcm->flows, port_id);
        if (!flow) {
                LOG_ERR("There is no flow bound to port-id %d", port_id);
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        if (kfifo_out(&flow->sdu_ready, &size, sizeof(size_t)) <
            sizeof(size_t)) {
                LOG_ERR("There is not enough data in port-id %d fifo", port_id);
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        /* FIXME: Is it possible to have 0 bytes sdus ??? */
        if (size == 0) {
                LOG_ERR("Zero-size SDU detected");
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        data = rkzalloc(size, GFP_KERNEL);
        if (!data) {
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        if (kfifo_out(&flow->sdu_ready, data, size) != size) {
                LOG_ERR("Could not get %zd bytes from fifo", size);
                rkfree(data);
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        *sdu = sdu_create_from(data, size);
        if (!*sdu) {
                rkfree(data);
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        KIPCM_UNLOCK(kipcm);

        /* The SDU is theirs now */
        return 0;
}

int kipcm_sdu_post(struct kipcm * kipcm,
                   port_id_t      port_id,
                   struct sdu *   sdu)
{
        struct ipcp_flow * flow;
        unsigned int       avail;

        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, cannot post SDU");
                return -1;
        }
        if (!sdu || !sdu_is_ok(sdu)) {
                LOG_ERR("Bogus parameters passed, bailing out");
                return -1;
        }

        KIPCM_LOCK(kipcm);

        flow = ipcp_fmap_find(kipcm->flows, port_id);
        if (!flow) {
                LOG_ERR("There is no flow bound to port-id %d", port_id);
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        avail = kfifo_avail(&flow->sdu_ready);
        if (avail < (sdu->buffer->size + sizeof(size_t))) {
                LOG_ERR("There is no space in the port-id %d fifo", port_id);
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        if (kfifo_in(&flow->sdu_ready,
                     &sdu->buffer->size,
                     sizeof(size_t)) != sizeof(size_t)) {
                LOG_ERR("Could not write %zd bytes from port-id %d fifo",
                        sizeof(size_t), port_id);
                KIPCM_UNLOCK(kipcm);
                return -1;
        }
        if (kfifo_in(&flow->sdu_ready,
                     sdu->buffer->data,
                     sdu->buffer->size) != sdu->buffer->size) {
                LOG_ERR("Could not write %zd bytes from port-id %d fifo",
                        sdu->buffer->size, port_id);
                KIPCM_UNLOCK(kipcm);
                return -1;
        }

        KIPCM_UNLOCK(kipcm);

        /* The SDU is ours now */
        return 0;
}
EXPORT_SYMBOL(kipcm_sdu_post);

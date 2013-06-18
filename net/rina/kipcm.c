/*
 * K-IPCM (Kernel-IPC Manager)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
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

#define RINA_PREFIX "kipcm"

#include <linux/linkage.h>
#include <linux/list.h>
#include <linux/kfifo.h>

#include "logs.h"
#include "utils.h"
#include "kipcm.h"

struct id_to_ipcp_t {
        ipc_process_id_t       id; /* key */
        struct ipc_process_t * ipcprocess; /* Value*/
        struct list_head       list;
};

struct port_id_to_flow_t {
        port_id_t             port_id; /* key */
        const struct flow_t * flow;    /* value */
        struct list_head      list;
};

struct kipc_t {
	/*
         * Maintained and used by the K-IPC Manager to return the proper flow
	 * instance that contains the modules that provide the Data Transfer
	 * Service in each kind of IPC Process.
         */

	//FIXME Define HASH_TABLE
	//HASH_TABLE(port_id_to_flow, port_id_t, struct flow_t *);
        struct list_head * port_id_to_flow;
	
	/*
         * A table with all the instances of IPC Processes, indexed by
	 * process_id.
         */
	//FIXME Define HASH_TABLE
	//HASH_TABLE(id_to_ipcp, ipc_process_id_t, struct ipc_process_t *);
	struct list_head * id_to_ipcp;
};

void * kipcm_init()
{
#if 0
	struct kipc_t * kipcm;
	LIST_HEAD(id_to_ipcp);
	LIST_HEAD(port_id_to_flow);

	LOG_FBEGN;

        if (!shim_init()) {
                LOG_FEXIT;
                return NULL;
        }

        kipcm = kmalloc(sizeof(*kipcm), GFP_KERNEL);
        if (!kipcm) {
                LOG_CRIT("Cannot allocate %d bytes of memory", sizeof(*kipcm));

                LOG_FEXIT;
                return NULL;
        }
        kipcm->id_to_ipcp      = &id_to_ipcp;
        kipcm->port_id_to_flow = &port_id_to_flow;

        LOG_FEXIT;

        return kipcm;
#else
        return NULL;
#endif
}

/*
 * FIXME: This should be kipcm_exit(void * opaque) I guess
 * I haven't yet put it in this form to not generate problems with rina.c
 */
void kipcm_exit()
{
#if 0
        LOG_FBEGN;

        /*
         * Add code here, depending on the operations performed
         * into kipcm_init
         */

        kfree(opaque);
        opaque = 0; /* Useless */

        shim_exit();

        LOG_FEXIT;
#endif
}

int kipcm_shim_register(struct shim_t * shim)
{
        LOG_FBEGN;

        ASSERT(shim);
        LOG_DBG("Registering shim %pK", shim);

        LOG_FEXIT;

        return 1;
}

int kipcm_shim_unregister(struct shim_t * shim)
{
        LOG_FBEGN;

        ASSERT(shim);
        LOG_DBG("Unregistering shim %pK", shim);

        LOG_FEXIT;

        return 1;
}

#if 0
static int is_instance_ok(const struct shim_instance_t * inst)
{
        LDBG("Checking shim instance %pK consistence", inst);

        if (inst                          &&
            inst->flow_allocate_request   &&
            inst->flow_allocate_response  &&
            inst->flow_deallocate         &&

            inst->application_register    &&
            inst->application_unregister  &&

            inst->sdu_read                &&
            inst->sdu_write) {
                LOG_DBG("Shim instance %pK is consistent", inst)
                        return 1;
        }

        LOG_ERR("Shim instance %pK is inconsistent", inst);
        return 0;
}
#endif

int kipcm_flow_add(void *                opaque,
                   port_id_t             id,
                   const struct flow_t * flow)
{
#if 0
        struct port_id_to_flow_t * port_flow;

        LOG_FBEGN;

        port_flow = kmalloc(sizeof(*port_flow), GFP_KERNEL);
        if (!port_flow) {
                LOG_ERR("Cannot allocate %d bytes of memory",
                        sizeof(*port_flow));
                LOG_FEXIT;
                return -1;
        }

        port_flow->port_id = port_id;

        port_flow->flow    = flow;
        INIT_LIST_HEAD(&port_flow->list);
        list_add(&port_flow->list, ((struct kipc_t *) opaque)->port_id_to_flow);

        LOG_FEXIT;
#endif
        return 0;
}
#if 0
const static struct flow_t *
retrieve_flow_by_port_id(void * opaque, port_id_t port_id)
{
#if 0
        struct port_id_to_flow_t * cur;

        list_for_each_entry(cur,
                            ((struct kipc_t *) opaque)->port_id_to_flow, list) {
                if (cur->port_id == port_id)
                        return cur->flow;
        }
#endif
        return NULL;
}

static struct port_id_to_flow_t *
retrieve_port_flow_node(void * opaque, port_id_t port_id)
{
#if 0
	struct port_id_to_flow_t * cur;

	list_for_each_entry(cur,
                            ((struct kipc_t *) opaque)->port_id_to_flow, list) {
                if (cur->port_id == port_id)
                        return cur;
        }
#endif
        return NULL;
}
#endif

int kipcm_flow_remove(void *    opaque,
                      port_id_t id)
{
#if 0
        struct port_id_to_flow_t * port_flow;

	LOG_FBEGN;

        port_flow = retrieve_port_flow_node(opaque, port_id);
        if (!port_flow)
        	return -1;

        list_del(&port_flow->list);

        LOG_FEXIT;
#endif
        return 0;
}

int kipcm_sdu_post(void *               opaque,
		   port_id_t            port_id,
		   const struct sdu_t * sdu)
{
#if 0
        const struct flow_t * flow;
        unsigned int          avail;

        LOG_FBEGN;

        flow = retrieve_flow_by_port_id(opaque, port_id);
        if (flow == NULL) {
                LOG_ERR("There is no flow bound to port-id %d", port_id);

                LOG_FEXIT;
                return -1;
        }

        /* FIXME : Change these stacked ifs with a proper switch */
        if (flow->application_owned) {

                avail = kfifo_avail(flow->sdu_ready);
                if (avail < (sdu->buffer->size + sizeof(size_t))) {
                        LOG_ERR("There is no space in the queue "
                                "for port_id %d",
                                port_id);
                        
                        LOG_FEXIT;
                        return -1;
                }

                /* FIXME: Miquel, check these return values. Francesco */
                if (kfifo_in(flow->sdu_ready,
                             &sdu->buffer->size,
                             sizeof(size_t)) != sizeof(size_t)) {
                        LOG_ERR("Could not push %d bytes into the fifo",
                                sizeof(size_t));
                        LOG_FEXIT;
                        return -1;
                }
                if (kfifo_in(flow->sdu_ready,
                             sdu->buffer->data,
                             sdu->buffer->size) != sdu->buffer->size) {
                        LOG_ERR("Could not push %d bytes into the fifo",
                                sdu->buffer->size);
                        LOG_FEXIT;
                        return -1;
                }
        } else {
                /* FIXME : RMT stuff */
        }

        LOG_FEXIT;
#endif
        return 0;
}

int kipcm_sdu_read(void *         opaque,
		   port_id_t      port_id,
		   bool_t         block,
		   struct sdu_t * sdu)
{
#if 0
        const struct flow_t * flow;
        size_t                size;
        char *                data;

        LOG_FBEGN;

        ASSERT(sdu);
        ASSERT(sdu->buffer);

        flow = retrieve_flow_by_port_id(opaque, port_id);
        if (flow == NULL) {
                LOG_ERR("There is no flow bound to port-id %d", port_id);

                LOG_FEXIT;
                return -1;
        }

        if (kfifo_out(flow->sdu_ready, &size, sizeof(size_t)) <
            sizeof(size_t)) {
                LOG_DBG("There is no data for port-id %d", port_id);

                LOG_FEXIT;
                return -1;
        }

        data = kmalloc(size, GFP_KERNEL);
        if (!data) {
                LOG_ERR("Cannot allocate %d bytes of kernel memory", size);

                LOG_FEXIT;
                return -1;
        }

        if (kfifo_out(flow->sdu_ready, data, size) != size) {
                LOG_ERR("Could not push %d bytes into a fifo", size);

                LOG_FEXIT;
                return -1;
        }

        sdu->buffer->data = data;
        sdu->buffer->size = size;

        LOG_FEXIT;
#endif
        return 0;
}

int  kipcm_sdu_write(void *               opaque,
		     port_id_t            port_id,
		     const struct sdu_t * sdu)
{
#if 0
        const struct flow_t * flow;
        int                   retval;

        LOG_FBEGN;

        flow = retrieve_flow_by_port_id(opaque, port_id);
        if (flow == NULL) {
                LOG_ERR("There is no flow bound to port-id %d", port_id);

                LOG_FEXIT;
                return -1;
        }

        retval = -1;
        switch (flow->ipc_process->type) {
        case DIF_TYPE_SHIM_ETH:
                retval = shim_eth_write_sdu(port_id, sdu);
                break;
        case DIF_TYPE_NORMAL :
                break;
        case DIF_TYPE_SHIM_IP :
                break;
        default :
                BUG();
        }

        if (retval) {
                LOG_ERR("Error writing SDU to the shim");
        }

        LOG_FEXIT;

        return retval;
#else
        return 0;
#endif
}
#if 0
static struct ipc_process_t *
find_ipc_process_by_id(void *           opaque,
		       ipc_process_id_t id)
{
#if 0
        struct id_to_ipcp_t * cur;

        LOG_FBEGN;

        list_for_each_entry(cur, kipcm->id_to_ipcp, list) {
                if (cur->id == id) {
                        LOG_FEXIT;
                        return cur->ipcprocess;
                }
        }

        LOG_FEXIT;
#endif
        return NULL;
}

static struct ipc_process_shim_ethernet_t *
create_shim(ipc_process_id_t ipcp_id)
{
#if 0
        struct ipc_process_shim_ethernet_t * ipcp_shim_eth;

        LOG_FBEGN;

        ipcp_shim_eth = kmalloc(sizeof(*ipcp_shim_eth), GFP_KERNEL);
        if (!ipcp_shim_eth) {
                LOG_ERR("Cannot allocate %d bytes of memory",
                        sizeof(*ipcp_shim_eth));

                LOG_FEXIT;
                return NULL;
        }

        ipcp_shim_eth->ipcp_id = ipcp_id;

        LOG_FEXIT;

        return ipcp_shim_eth;
#else
        return NULL;
#endif
}

static int add_id_to_ipcp_node(void *                 opaque,
			       ipc_process_id_t       id,
                               struct ipc_process_t * ipc_process)
{
#if 0
        struct id_to_ipcp_t *aux_id_to_ipcp;

        LOG_FBEGN;

        aux_id_to_ipcp = kmalloc(sizeof(*aux_id_to_ipcp), GFP_KERNEL);
        if (!aux_id_to_ipcp) {
                LOG_ERR("Cannot allocate %d bytes of memory",
                        sizeof(*aux_id_to_ipcp));
                LOG_FEXIT;
                return -1;
        }

        aux_id_to_ipcp->id         = id;
        aux_id_to_ipcp->ipcprocess = ipc_process;
        INIT_LIST_HEAD(&aux_id_to_ipcp->list);
        list_add(&aux_id_to_ipcp->list, ((struct kipc_t *) opaque)->id_to_ipcp);

        LOG_FEXIT;
#endif
        return 0;
}
#endif
int kipcm_ipc_process_create(void *                opaque,
			     const struct name_t * name,
			     ipc_process_id_t      ipcp_id,
			     dif_type_t            type)
{
#if 0
        struct ipc_process_t *ipc_process;

        LOG_FBEGN;

        switch (type) {
        case DIF_TYPE_SHIM_ETH :
                if (shim_eth_ipc_create(name, ipcp_id)) {
                        LOG_FEXIT;

                        return -1;
                }
                ipc_process = kmalloc(sizeof(*ipc_process), GFP_KERNEL);
                if (!ipc_process) {
                        LOG_ERR("Cannot allocate %d bytes of memory",
                                sizeof(*ipc_process));
                        LOG_FEXIT;
                        return -1;
                }

                ipc_process->type = type;
                ipc_process->data.shim_eth_ipcp = create_shim(ipcp_id);

                add_id_to_ipcp_node(opaque, ipcp_id, ipc_process);

                break;
        case DIF_TYPE_NORMAL:
                break;
        case DIF_TYPE_SHIM_IP:
                break;
        default:
                BUG();
        }
#endif

        LOG_FEXIT;

        return 0;
}

int  kipcm_ipc_process_configure(void *                            opaque,
				 ipc_process_id_t                  ipcp_id,
                                 const struct ipc_process_conf_t *
                                 configuration)
{
#if 0
        struct ipc_process_t *                          ipc_process;
        const struct ipc_process_shim_ethernet_conf_t * conf;

        LOG_FBEGN;

        ipc_process = find_ipc_process_by_id(opaque, ipcp_id);
        if (ipc_process == NULL) {
                LOG_FEXIT;
                return -1;
        }
        switch (ipc_process->type) {
        case DIF_TYPE_SHIM_ETH:
                conf = configuration->ipc_process_conf.shim_eth_ipcp_conf;
#if 0
                if (shim_eth_ipc_configure(ipcp_id, conf)) {
                        LOG_ERR("Failed configuring the SHIM IPC Process");
                        LOG_FEXIT;

                        return -1;
                }
#endif
                break;
        case DIF_TYPE_NORMAL:
                break;
        case DIF_TYPE_SHIM_IP:
                break;
        default:
                BUG();
        }

        LOG_FEXIT;
#endif
        return 0;
}
#if 0
static struct id_to_ipcp_t *
find_id_to_ipcp_by_id(void * opaque, ipc_process_id_t id)
{
#if 0
        struct id_to_ipcp_t * cur;

        LOG_FBEGN;

        list_for_each_entry(cur, ((struct kipc_t *) opaque)->id_to_ipcp, list) {
                if (cur->id == id) {
                        LOG_FEXIT;
                        return cur;
                }
        }

        LOG_FEXIT;
#endif
        return NULL;
}
#endif
int  kipcm_ipc_process_destroy(void * opaque, ipc_process_id_t ipcp_id)
{
#if 0
        struct id_to_ipcp_t * id_ipcp;

        LOG_FBEGN; 

        id_ipcp = find_id_to_ipcp_by_id(opaque, ipcp_id);
        if (!id_ipcp)
                return -1;

        list_del(&id_ipcp->list);

        switch (id_ipcp->ipcprocess->type) {
        case DIF_TYPE_SHIM_ETH :
                shim_eth_destroy(ipcp_id);
                break;
        case DIF_TYPE_NORMAL:
                break;
        case DIF_TYPE_SHIM_IP:
                break;
        default :
                BUG();
        }

        kfree(id_ipcp);

        LOG_FEXIT;
#endif

        return 0;
}

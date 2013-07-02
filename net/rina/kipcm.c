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

#include <linux/slab.h>
#include <linux/kobject.h>
#include <linux/export.h>

#define RINA_PREFIX "kipcm"

#include "logs.h"
#include "utils.h"
#include "shim.h"
#include "kipcm.h"

#define to_shim(O) container_of(O, struct shim, kobj)

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

static struct list_head * initialize_list()
{
	struct list_head * head;

	head = kzalloc(sizeof(*head), GFP_KERNEL);
	if (!head) {
		return NULL;
	}
	head->next = head;
	head->prev = head;

	return head;
}

static int add_id_to_ipcp_node(struct kipcm *         kipcm,
                               ipc_process_id_t       id,
                               struct ipc_process_t * ipc_process)
{
        struct id_to_ipcp_t *id_to_ipcp;

        LOG_FBEGN;

        id_to_ipcp = kzalloc(sizeof(*id_to_ipcp), GFP_KERNEL);
        if (!id_to_ipcp) {
                LOG_ERR("Cannot allocate %zu bytes of memory",
                        sizeof(*id_to_ipcp));
                LOG_FEXIT;
                return -1;
        }

        id_to_ipcp->id         = id;
        id_to_ipcp->ipcprocess = ipc_process;
        INIT_LIST_HEAD(&id_to_ipcp->list);
        list_add(&id_to_ipcp->list, kipcm->id_to_ipcp);

        LOG_FEXIT;

        return 0;
}

static struct ipc_process_t *
find_ipc_process_by_id(struct kipcm *   kipcm,
                       ipc_process_id_t id)
{
        struct id_to_ipcp_t * cur;

        LOG_FBEGN;

        list_for_each_entry(cur, kipcm->id_to_ipcp, list) {
                if (cur->id == id) {
                        LOG_FEXIT;
                        return cur->ipcprocess;
                }
        }

        LOG_FEXIT;

        return NULL;
}

struct kipcm * kipcm_init(struct kobject * parent)
{
        struct kipcm * tmp;

        LOG_FBEGN;

        LOG_DBG("Initializing");

        tmp = kzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp) {
                LOG_ERR("Cannot allocate %zu bytes of memory", sizeof(*tmp));

                LOG_FEXIT;
                return NULL;
        }

        tmp->shims = shims_init(parent);
        if (!tmp->shims) {
                kfree(tmp);

                LOG_FEXIT;
                return NULL;
        }

        tmp->id_to_ipcp = initialize_list();
        if (!tmp->id_to_ipcp) {
		kfree(tmp);

		LOG_FEXIT;
		return NULL;
	}

        tmp->port_id_to_flow = initialize_list();
	if (!tmp->port_id_to_flow) {
		kfree(tmp);

		LOG_FEXIT;
		return NULL;
	}

        LOG_DBG("Initialized successfully");

        LOG_FEXIT;

        return tmp;
}

int kipcm_fini(struct kipcm * kipcm)
{
        LOG_FBEGN;

        LOG_DBG("Finalizing");

        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance, cannot finalize");

                LOG_FEXIT;
                return -1;
        }

        if (shims_fini(kipcm->shims)) {
                LOG_FEXIT;
                return -1;
        } 

        kfree(kipcm->id_to_ipcp);
        kfree(kipcm->port_id_to_flow);
        kfree(kipcm);

        LOG_DBG("Finalized successfully");

        LOG_FEXIT;

        return 0;
}

struct shim * kipcm_shim_register(struct kipcm *    kipcm,
                                  const char *      name,
                                  void *            data,
                                  struct shim_ops * ops)
{ return shim_register(kipcm->shims, name, data, ops); }
EXPORT_SYMBOL(kipcm_shim_register);

int kipcm_shim_unregister(struct kipcm * kipcm,
                          struct shim *  shim)
{ return shim_unregister(kipcm->shims, shim); }
EXPORT_SYMBOL(kipcm_shim_unregister);

int kipcm_ipc_create(struct kipcm *        kipcm,
                     const struct name_t * name,
                     ipc_process_id_t      id,
                     dif_type_t            type)
{
	struct kobject *       k;
	struct shim *          shim;
	struct ipc_process_t * ipc_process;

	switch (type) {
	case DIF_TYPE_SHIM :
		k = kset_find_obj(kipcm->shims->set, "shim-dummy");
		shim = to_shim(k);
		ipc_process = kzalloc(sizeof(*ipc_process), GFP_KERNEL);
                if (!ipc_process) {
                        LOG_ERR("Cannot allocate %zu bytes of memory",
                                sizeof(*ipc_process));
                        LOG_FEXIT;
                        return -1;
                }

		ipc_process->data.shim_instance =
				shim->ops->create(shim->data, id);
		if(!ipc_process->data.shim_instance) {
			kfree(ipc_process);
			return -1;
		}

		if (!add_id_to_ipcp_node(kipcm, id, ipc_process)) {
			kfree(ipc_process);
			return -1;
		}

		break;
	case DIF_TYPE_NORMAL :
		break;
	default :
		BUG();
	}
	return 0;
}

int kipcm_ipc_configure(struct kipcm *                  kipcm,
                        ipc_process_id_t                id,
                        const struct ipc_process_conf * cfg)
{
	struct ipc_process_t * ipc_process;
	struct kobject *       k;
	struct shim *          shim;

	LOG_FBEGN;

	ipc_process = find_ipc_process_by_id(kipcm, id);
	if (ipc_process == NULL) {
		LOG_FEXIT;
		return -1;
	}
	switch (ipc_process->type) {
	case DIF_TYPE_SHIM :
		k = kset_find_obj(kipcm->shims->set, "shim-dummy");
		shim = to_shim(k);
		ipc_process->data.shim_instance = shim->ops->configure(
				kipcm, ipc_process->data.shim_instance, cfg);
		break;
	case DIF_TYPE_NORMAL :
		break;
	default :
		BUG();
	}

	return 0;
}

int kipcm_ipc_destroy(struct kipcm *   kipcm,
                      ipc_process_id_t id)
{ return -1; }

int kipcm_flow_add(struct kipcm *      kipcm,
                   port_id_t           id,
                   const struct flow * flow)
{ return -1; }

int kipcm_flow_remove(struct kipcm * kipcm,
                      port_id_t      id)
{ return -1; }

int kipcm_sdu_write(struct kipcm *       kipcm,
                    port_id_t            id,
                    const struct sdu_t * sdu)
{ return -1; }

int kipcm_sdu_read(struct kipcm * kipcm,
                   port_id_t      id,
                   struct sdu_t * sdu)
{ return -1; }

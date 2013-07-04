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

#include <linux/kobject.h>
#include <linux/export.h>

#define RINA_PREFIX "kipcm"

#include "logs.h"
#include "utils.h"
#include "shim.h"
#include "kipcm.h"

struct kipcm * kipcm_init(struct kobject * parent)
{
        struct kipcm * tmp;

        LOG_DBG("Initializing");

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->shims = shims_init(parent);
        if (!tmp->shims) {
                rkfree(tmp);
                return NULL;
        }

        LOG_DBG("Initialized successfully");

        return tmp;
}

int kipcm_fini(struct kipcm * kipcm)
{
        LOG_DBG("Finalizing");

        if (!kipcm) {
                LOG_ERR("Bogus kipcm instance passed, cannot finalize");
                return -1;
        }

        if (shims_fini(kipcm->shims))
                return -1;

        rkfree(kipcm);

        LOG_DBG("Finalized successfully");

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
{ return -1; }

int kipcm_ipc_configure(struct kipcm *                  kipcm,
                        ipc_process_id_t                id,
                        const struct ipc_process_conf * cfg)
{ return -1; }

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

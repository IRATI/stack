/*
 *  Shim IPC Process
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
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

#ifndef RINA_SHIM_H
#define RINA_SHIM_H

#include <linux/kobject.h>

#include "common.h"

/* FIXME: This configuration part has to be rearranged */

enum shim_config_type {
        SHIM_CONFIG_UINT   = 1,
        SHIM_CONFIG_STRING,
};

struct shim_config_value {
        enum shim_config_type type;
        void *                data;
};

struct shim_config_entry {
        char *                     name;
        struct shim_config_value * value;
};

struct shim_config { 
	struct list_head           list;
	struct shim_config_entry * entry;
};

struct shim_instance_ops {
	int  (* flow_allocate_request)(void *                     data,
                                       const struct name_t *      source,
                                       const struct name_t *      dest,
                                       const struct flow_spec_t * flow_spec,
                                       port_id_t *                id);
	int  (* flow_allocate_response)(void *              data,
                                        port_id_t           id,
                                        response_reason_t * response);
	int  (* flow_deallocate)(void *    data,
                                 port_id_t id);

	int  (* application_register)(void *                data,
                                      const struct name_t * name);
	int  (* application_unregister)(void *                data,
                                        const struct name_t * name);

	int  (* sdu_write)(void *               data,
                           port_id_t            id,
                           const struct sdu_t * sdu);

        /* FIXME: sdu_read will be removed */
        int  (* sdu_read)(void *         data,
                          port_id_t      id,
                          struct sdu_t * sdu);
};

struct shim_instance {
        struct kobject             kobj;

        void *                     data;
        struct shim_instance_ops * ops;
};

struct shim_ops {
        int                    (* init)(void * data);
        int                    (* fini)(void * opaque);

	struct shim_instance * (* create)(void *           data,
                                          ipc_process_id_t id);
        
	struct shim_instance * (* configure)(void *                     data,
                                             struct shim_instance *     inst,
                                             const struct shim_config * cfg);

	int                    (* destroy)(void *                 data,
                                           struct shim_instance * inst);
};

struct shim {
        struct kobject          kobj;
        void *                  data;
        const struct shim_ops * ops; 
};

struct shims {
        struct kset * set;
};

/* Called by the kipcm, might disappear */
struct shims * shims_init(struct kobject * parent);
int            shims_fini(struct shims * shims);

/* Called (once) by each shim module upon loading/unloading */
struct shim *  shim_register(struct shims *          parent,
                             const char *            name,
                             void *                  data,
                             const struct shim_ops * ops);
int            shim_unregister(struct shims * parent,
                               struct shim *  shim);

#endif

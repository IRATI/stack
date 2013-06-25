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

#include "common.h"

enum shim_config_type_t {
        SHIM_CONFIG_UINT   = 1,
        SHIM_CONFIG_STRING,
};

struct shim_config_value_t {
        enum shim_config_type_t type;
        void *                  data;
};

struct shim_config_entry_t {
        char *                       name;
        struct shim_config_value_t * value;
};

struct shim_conf_t { 
	struct list_head             list;
	struct shim_config_entry_t * entry;
};

struct shim_instance_t {
        void * opaque;

        /* Function pointers exported by the shim module, per shim instance */
	int  (* flow_allocate_request)(void *                     opaque,
                                       const struct name_t *      source,
                                       const struct name_t *      dest,
                                       const struct flow_spec_t * flow_spec,
                                       port_id_t *                id);
	int  (* flow_allocate_response)(void *              opaque,
                                        port_id_t           id,
                                        response_reason_t * response);
	int  (* flow_deallocate)(void *    opaque,
                                 port_id_t id);

	int  (* application_register)(void *                opaque,
                                      const struct name_t * name);
	int  (* application_unregister)(void *                opaque,
                                        const struct name_t * name);

	int  (* sdu_write)(void *               opaque,
                           port_id_t            id,
                           const struct sdu_t * sdu);
        int  (* sdu_read)(void *         opaque,
                          port_id_t      id,
                          struct sdu_t * sdu);
};

struct shim_t {
	void * opaque;
        /*
         * The unique label this shim will be identified with, within the
         * system. The label will be exposed to the user.
         *
         * It must not be NULL!
         */
        char * label;

	struct shim_instance_t * (* create)
                (void *           opaque,
		 ipc_process_id_t id);
        
	struct shim_instance_t * (* configure)
                (void *                     opaque,
		 struct shim_instance_t *   instance,
                 const struct shim_conf_t * configuration);

	int                      (* destroy)
	        (void *                  opaque,
		struct shim_instance_t * inst);
};

/* Called by the kipcm, might disappear */
int  shim_init(void);
void shim_exit(void);

/* Called (once) by each shim module upon loading/unloading */
int  shim_register(struct shim_t * shim);
int  shim_unregister(struct shim_t * shim);

#endif

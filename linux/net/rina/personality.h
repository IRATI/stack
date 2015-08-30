/*
 * Personality
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
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

#ifndef RINA_PERSONALITY_H
#define RINA_PERSONALITY_H

#include <linux/kobject.h>

#include "common.h"
#include "ipcp-instances.h"
#include "du.h"

/* Pre-declared, the personality should define it properly */
struct personality_data;

typedef unsigned int personality_id;

struct personality_ops {
        int (* init)(struct kobject *          parent,
                     personality_id            id,
                     struct personality_data * data);
        int (* fini)(struct personality_data * data);

        /*
         * The following function pointers are hooked into the syscalls
         */

        int (* ipc_create)(struct personality_data * data,
                           const struct name *       name,
                           ipc_process_id_t          id,
                           const char *              type);
        int (* ipc_destroy)(struct personality_data * data,
                            ipc_process_id_t          id);

        int (* flow_create)(struct personality_data * data,
			    ipc_process_id_t          pid,
			    struct name *             name);
	int (* flow_opts_set)(struct personality_data *data,
			      port_id_t                port_id,
			      flow_opts_t              flow_opts);
	int (* flow_opts)(struct personality_data *data,
			      port_id_t                port_id);

        int (* flow_destroy)(struct personality_data * data,
			     ipc_process_id_t          ipc_id,
			     port_id_t                 pid);

        /* Takes the ownership of the sdu */
        int (* sdu_write)(struct personality_data * data,
                          port_id_t                 id,
                          struct sdu *              sdu);

        /* Passes the ownership of the sdu */
        int (* sdu_read)(struct personality_data *  data,
                         port_id_t                  id,
                         struct sdu **              sdu);

        /* Takes the ownership of the sdu with port-id */
        int (* mgmt_sdu_write)(struct personality_data * data,
                               ipc_process_id_t          id,
                               struct sdu_wpi *          sdu_wpi);

        /* Passes the ownership of the sdu with port-id */
        int (* mgmt_sdu_read)(struct personality_data * data,
                              ipc_process_id_t          id,
                              struct sdu_wpi **         sdu_wpi);
};

struct personality {
        struct kobject            kobj;
        personality_id            id;
        struct personality_data * data;
        struct personality_ops *  ops;
};

/* FIXME: To be removed ASAP */
extern struct personality * default_personality;

int                  rina_personality_init(struct kobject * parent);
void                 rina_personality_exit(void);

struct personality * rina_personality_register(const char *              name,
                                               struct personality_data * data,
                                               struct personality_ops *  ops);

int                  rina_personality_unregister(struct personality * pers);

#endif

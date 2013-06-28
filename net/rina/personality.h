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

/* FIXME: This include should be removed from here */
#include "kipcm.h"

struct personality_ops {
        int (* init)(struct kobject * parent,
                     void *           data);
        int (* fini)(void * data);

        /* Functions exported to the personality user */
        int (* ipc_create)(void *                    data,
                           const struct name_t *     name,
                           ipc_process_id_t          id,
                           dif_type_t                type);
        int (* ipc_configure)(void *                            data,
                              ipc_process_id_t                  id,
                              const struct ipc_process_conf_t * configuration);
        int (* ipc_destroy)(void *           data,
                            ipc_process_id_t id);
        
        int (* connection_create)(void *                      data,
                                  const struct connection_t * connection);
        int (* connection_destroy)(void *   data,
                                   cep_id_t id);
        int (* connection_update)(void *   data,
                                  cep_id_t id_from,
                                  cep_id_t id_to);

        int (* sdu_write)(void *               data,
                          port_id_t            id,
                          const struct sdu_t * sdu);
        int (* sdu_read)(void *         data,
                         port_id_t      id,
                         struct sdu_t * sdu);
};

struct personality {
        struct kobject           kobj;
        void *                   data;
        struct personality_ops * ops;
};

int  rina_personality_init(struct kobject * parent);
void rina_personality_exit(void);

struct personality * rina_personality_register(const char *             name,
                                               void *                   data,
                                               struct personality_ops * ops);

int rina_personality_unregister(struct personality * pers);

#endif

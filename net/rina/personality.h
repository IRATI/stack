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

#include "common.h"

/* FIXME: These include should disappear from here */
#include "kipcm.h"

struct personality_t {
        /* Might be removed if deemed unnecessary */
        void * data;

        int  (* init)(void * opaque);
        void (* fini)(void * opaque);

        /* Functions exported to the personality user */
        int (* ipc_create)(void *                opaque,
                           const struct name_t * name,
                           ipc_process_id_t      id,
                           dif_type_t            type);
        int (* ipc_configure)(void *                            opaque,
                              ipc_process_id_t                  id,
                              const struct ipc_process_conf_t * configuration);
        int (* ipc_destroy)(void *           opaque,
                            ipc_process_id_t ipcp_id);
        
        int (* connection_create)(void *                      opaque,
                                  const struct connection_t * connection);
        int (* connection_destroy)(void *   opaque,
                                   cep_id_t cep_id);
        int (* connection_update)(void *   opaque,
                                  cep_id_t from_id,
                                  cep_id_t to_id);

        int (* sdu_write)(void *               opaque,
                          port_id_t            id,
                          const struct sdu_t * sdu);
        int (* sdu_read)(void *         opaque,
                         port_id_t      id,
                         struct sdu_t * sdu);
};

/* FIXME: To be removed ASAP */
extern struct personality_t * personality;

int  rina_personality_init(void);
void rina_personality_exit(void);

int  rina_personality_register(struct personality_t * pers);
int  rina_personality_unregister(struct personality_t * pers);

#endif

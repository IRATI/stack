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
        /* Might be removed when deemed unnecessary */
        int  (* init)(void);
        void (* exit)(void);

        void * data;

        /* Functions exported to the personality user */
        /* FIXME: should be int, not long */
        long (* ipc_create)(const struct name_t * name,
                            ipc_process_id_t      id,
                            dif_type_t            type);
        long (* ipc_configure)(ipc_process_id_t                  id,
                               const struct ipc_process_conf_t * configuration);
        long (* ipc_destroy)(ipc_process_id_t ipcp_id);
        
        long (* sdu_read)(port_id_t      id,
                          struct sdu_t * sdu);
        long (* sdu_write)(port_id_t            id,
                           const struct sdu_t * sdu);
        long (* connection_create)(const struct connection_t * connection);
        long (* connection_destroy)(cep_id_t cep_id);
        long (* connection_update)(cep_id_t from_id,
                                   cep_id_t to_id);
};

/* FIXME: To be removed ASAP */
extern const struct personality_t * personality;

int  rina_personality_init(void);
void rina_personality_exit(void);

int  rina_personality_register(const struct personality_t * pers);
int  rina_personality_unregister(const struct personality_t * pers);

#endif

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

struct personality_data;

struct personality_t {
        /*
         * The unique label this personality will be identified with, within
         * the system. The label will be exposed to the user.
         *
         * It must not be NULL!
         */
        char *                    label;

        /* Might be removed if deemed unnecessary */
        struct personality_data * data;

        int  (* init)(struct personality_data * data);
        void (* fini)(struct personality_data * data);

        /* Functions exported to the personality user */
        int (* ipc_create)(struct personality_data * data,
                           const struct name_t *     name,
                           ipc_process_id_t          id,
                           dif_type_t                type);
        int (* ipc_configure)(struct personality_data *         data,
                              ipc_process_id_t                  id,
                              const struct ipc_process_conf_t * configuration);
        int (* ipc_destroy)(struct personality_data * data,
                            ipc_process_id_t          id);
        
        int (* connection_create)(struct personality_data *   data,
                                  const struct connection_t * connection);
        int (* connection_destroy)(struct personality_data * data,
                                   cep_id_t                  id);
        int (* connection_update)(struct personality_data * data,
                                  cep_id_t                  id_from,
                                  cep_id_t                  id_to);

        int (* sdu_write)(struct personality_data * data,
                          port_id_t                 id,
                          const struct sdu_t *      sdu);
        int (* sdu_read)(struct personality_data * data,
                         port_id_t                 id,
                         struct sdu_t *            sdu);
};

int  rina_personality_init(void);
void rina_personality_exit(void);

int  rina_personality_register(struct personality_t * pers);
int  rina_personality_unregister(struct personality_t * pers);

#endif

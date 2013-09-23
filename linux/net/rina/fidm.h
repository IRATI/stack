/*
 * FIDM (Flows-IDs Manager)
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

#ifndef RINA_FIDM_H
#define RINA_FIDM_H

typedef int16_t flow_id_t;

int           is_flow_id_ok(flow_id_t id);

struct fidm;

struct fidm * fidm_create(void);
int           fidm_destroy(struct fidm * instance);

flow_id_t     fidm_allocate(struct fidm * instance);
int           fidm_release(struct fidm * instance,
                           flow_id_t     id);

#endif

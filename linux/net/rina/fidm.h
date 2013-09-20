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

/* FIXME: Move to _create and _destroy */
int        fidm_init(void);
int        fidm_fini(void);

/* ALWAYS use this function to check if the id looks good */
static inline int is_flow_id_ok(flow_id_t id)
{ return id >= 0 ? 1 : 0; }

flow_id_t  fidm_allocate(void);
int        fidm_release(flow_id_t id);

#endif

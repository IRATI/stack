/*
 * Default policy set for delimiting, partially implements the general
 * delimiting module spec
 *
 *    Eduard Grasa <eduard.grasa@i2cat.net>
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

#ifndef DELIMITING_PS_DEFAULT_H
#define DELIMITING_PS_DEFAULT_H

#include "delim-ps.h"
#include "delim.h"

struct ps_base * delim_ps_default_create(struct rina_component *component);
void delim_ps_default_destroy(struct ps_base *bps);
int default_delim_fragment(struct delim_ps * ps, struct du * du,
			   struct du_list * du_list);
int default_delim_num_udfs(struct delim_ps * ps, struct du * du);
int default_delim_process_udf(struct delim_ps * ps, struct du * du,
			      struct du_list * du_list);

#endif /* DELIMITING_PS_DEFAULT_H */

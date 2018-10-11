/*
 * Delimiting module
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

#ifndef RINA_DELIMITING_H
#define RINA_DELIMITING_H

#include "common.h"
#include "du.h"
#include "ps-factory.h"
#include "rds/robjects.h"

struct delim {
	/* The delimiting module instance */
	struct rina_component base;

	/* The parent EFCP object instance */
	struct efcp * efcp;

	/* The maximum fragment size for the DIF */
	uint32_t max_fragment_size;

	/* Lists to facilitate interacting with the policy */
	struct du_list * tx_dus;
	struct du_list * rx_dus;

	struct robject robj;
};

struct delim * delim_create(struct efcp * efcp, struct robject * parent);
int delim_destroy(struct delim * delim);
struct delim * delim_from_component(struct rina_component *component);
int delim_select_policy_set(struct delim * delim,
                            const string_t *  path,
                            const string_t *  name);
int delim_set_policy_set_param(struct delim * delim,
			       const char * path,
			       const char * name,
			       const char * value);
int delim_ps_publish(struct ps_factory * factory);
int delim_ps_unpublish(const char * name);

#endif

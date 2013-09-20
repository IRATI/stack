/*
 * Flow Manager
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

#include <linux/module.h>

#include "flow-manager.h"

int fmgr_init(void)
{ return 0; }
EXPORT_SYMBOL(fmgr_init);

int fmgr_fini(void)
{ return 0; }
EXPORT_SYMBOL(fmgr_fini);

flow_id_t fmgr_allocate(void)
{ return -1; }
EXPORT_SYMBOL(fmgr_allocate);

int fmgr_release(flow_id_t id)
{ return -1; }
EXPORT_SYMBOL(fmgr_release);

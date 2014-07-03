/*
 * Support for multiple VMPI providers
 *
 *    Vincenzo Maffione <v.maffione@nextworks.it>
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

#ifndef __VMPI_PROVIDER_H__
#define __VMPI_PROVIDER_H__

#include "vmpi-ops.h"


#define VMPI_PROVIDER_HOST       0U
#define VMPI_PROVIDER_GUEST      1U
#define VMPI_PROVIDER_AUTO       2U


int vmpi_provider_find_instance(unsigned int provider, int id,
                                struct vmpi_ops *ops);

int vmpi_provider_register(unsigned int provider, unsigned int id,
                           const struct vmpi_ops *ops);

int vmpi_provider_unregister(unsigned int provider, unsigned int id);

#endif  /* __VMPI_PROVIDER_H__ */

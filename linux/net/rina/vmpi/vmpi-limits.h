/*
 * VMPI common definitions and limits
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

#include <asm/page.h>

#ifndef __VMPI_LIMITS_H__
#define __VMPI_LIMITS_H__

#define VMPI_MAX_CHANNELS_DEFAULT 64
#define VMPI_BUF_SIZE   PAGE_SIZE

#if (PAGE_SIZE > 4096)
#warning "Page size is greater than 4096: this situation has never been tested"
#endif

#endif   /* __VMPI_LIMITS_H__ */

/*
 * RINA Memory
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

#ifndef RINA_RMEM_H
#define RINA_RMEM_H

#include <linux/slab.h>

void * rkmalloc(size_t size, gfp_t flags);
void * rkzalloc(size_t size, gfp_t flags);
void   rkfree(void * ptr);
void   rms_dump(void);

#include <linux/string.h>

#define bzero(DEST, LEN) do { (void) memset(DEST, 0, LEN); } while (0)

#endif

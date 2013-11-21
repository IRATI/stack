/*
 * RINA Bitmaps
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

#ifndef RINA_RBMP_H
#define RINA_RBMP_H

struct rbmp;

struct rbmp * rbmp_create(size_t bits, ssize_t offset);
struct rbmp * rbmp_create_ni(size_t bits, ssize_t offset);
int           rbmp_destroy(struct rbmp * b);

ssize_t       rbmp_allocate(struct rbmp * instance);
int           rbmp_release(struct rbmp * instance,
                           ssize_t       id);
bool          rbmp_is_id_ok(struct rbmp * b, ssize_t id);

#endif

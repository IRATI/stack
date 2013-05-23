/*
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

#ifndef RINA_IPCM_H
#define RINA_IPCM_H

int  ipcm_init(void);
void ipcm_exit(void);

/*
* Introduced by Miquel 5.22.2013
*/

int kipc_add_entry(port_id_t port_id, const flow_t *flow);
int kipc_remove_entry(port_id_t port_id);
int kipc_post_sdu(port_id_t port_id, const sdu_t *sdu);

#endif

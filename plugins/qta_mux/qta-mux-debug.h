/*
 * QTA-Mux policy set
 * Debug operations for RMT PS
 *
 *    Leonardo Bergesio <leonardo.bergesio@i2cat.net>
 *    Eduard Grasa	<eduard.grasa@i2cat.net>
 *
 * This program is free software; you can casistribute it and/or modify
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

#include "common.h"

#define UQUEUE_DEBUG_SIZE 100000
#define QTA_MUX_DEBUG 0

struct urgency_queue_debug_info {
	s64	 	 q_log[UQUEUE_DEBUG_SIZE][2]; /*stores length & time*/
	uint_t		 q_index;
	uint_t           urgency_level;
	port_id_t        port;
	struct list_head list;
};

int    qta_mux_debug_proc_init(void);
void   qta_mux_debug_proc_exit(void);
struct urgency_queue_debug_info * uqueue_debug_info_create(port_id_t port,
							   uint_t urgency_lev);
void udebug_info_destroy(struct urgency_queue_debug_info * info);

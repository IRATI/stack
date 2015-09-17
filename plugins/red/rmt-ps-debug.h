/*
 * TCP-like RED Congestion avoidance policy set
 * Debug operations for RMT PS
 *
 *    Leonardo Bergesio <leonardo.bergesio@i2cat.net>
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
#include <net/red.h>

#define RMT_DEBUG 0
#define RMT_DEBUG_SIZE 300000

struct red_rmt_debug {
	unsigned int	 q_log[RMT_DEBUG_SIZE][2];
	unsigned int     q_index;
	port_id_t        port;
	struct red_stats stats;
	struct list_head list;
};

int    red_rmt_debug_proc_init(void);
void   red_rmt_debug_proc_exit(void);
struct red_rmt_debug * red_rmt_debug_create(port_id_t port);

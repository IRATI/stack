/*
 * TCP-like RED Congestion avoidance policy set
 * Debug operations for DTCP PS
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

#define DTCP_DEBUG 0
#define DTCP_DEBUG_SIZE 300000

struct red_dtcp_debug {
	unsigned long    	ws_log[DTCP_DEBUG_SIZE];
	unsigned int     	ws_index;
	struct list_head 	list;
};

int    red_dtcp_debug_proc_init(void);
void   red_dtcp_debug_proc_exit(void);
struct red_dtcp_debug * red_dtcp_debug_create(void);

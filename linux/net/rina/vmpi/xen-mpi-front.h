/*
 * A guest-side vmpi-impl implementation for Xen
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

#ifndef __XEN_PUBLIC_IO_MPIIF_H__
#define __XEN_PUBLIC_IO_MPIIF_H__

#include <xen/interface/io/ring.h>
#include <xen/interface/grant_table.h>

/*
 * Notifications after enqueuing any type of message should be conditional on
 * the appropriate req_event or rsp_event field in the shared ring.
 * If the client sends notification for rx requests then it should specify
 * feature 'feature-rx-notify' via xenbus. Otherwise the backend will assume
 * that it cannot safely queue packets (as it may not be kicked to send them).
 */

 /*
 * "feature-split-event-channels" is introduced to separate guest TX
 * and RX notificaion. Backend either doesn't support this feature or
 * advertise it via xenstore as 0 (disabled) or 1 (enabled).
 *
 * To make use of this feature, frontend should allocate two event
 * channels for TX and RX, advertise them to backend as
 * "event-channel-tx" and "event-channel-rx" respectively. If frontend
 * doesn't want to use this feature, it just writes "event-channel"
 * node as before.
 */

#define XEN_NETIF_MAX_TX_SIZE 0xFFFF
struct xen_mpi_tx_request {
    grant_ref_t gref;      /* Reference to buffer page */
    uint16_t offset;       /* Offset within buffer page */
    uint16_t flags;        /* XEN_NETTXF_* */
    uint16_t id;           /* Echoed in response message. */
    uint16_t size;         /* Packet size in bytes.       */
};

struct xen_mpi_tx_response {
	uint16_t id;
	int16_t  status;       /* XEN_NETIF_RSP_* */
};

struct xen_mpi_rx_request {
	uint16_t    id;        /* Echoed in response message.        */
	grant_ref_t gref;      /* Reference to incoming granted frame */
        uint16_t    offset;
        uint16_t    len;
};

struct xen_mpi_rx_response {
    uint16_t id;
    uint16_t flags;        /* XEN_NETRXF_* */
    int16_t  status;       /* -ve: BLKIF_RSP_* ; +ve: Rx'ed pkt size. */
};

/*
 * Generate mpi ring structures and types.
 */

DEFINE_RING_TYPES(xen_mpi_tx,
		  struct xen_mpi_tx_request,
		  struct xen_mpi_tx_response);
DEFINE_RING_TYPES(xen_mpi_rx,
		  struct xen_mpi_rx_request,
		  struct xen_mpi_rx_response);

#define XEN_NETIF_RSP_DROPPED	-2
#define XEN_NETIF_RSP_ERROR	-1
#define XEN_NETIF_RSP_OKAY	 0

#endif

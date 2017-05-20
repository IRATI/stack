/*
 * An hypervisor-side vmpi-impl implementation for Xen
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

#ifndef __XEN_MPIBACK__COMMON_H__
#define __XEN_MPIBACK__COMMON_H__

#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/io.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/wait.h>
#include <linux/sched.h>

#include "xen-mpi-ioring.h"  //#include <xen/interface/io/mpi.h>
#include <xen/interface/grant_table.h>
#include <xen/grant_table.h>
#include <xen/xenbus.h>

#include "vmpi-structs.h"
#include "vmpi-host-impl.h"
#include "vmpi.h"


typedef unsigned int pending_ring_idx_t;
#define INVALID_PENDING_RING_IDX (~0U)

/* For the head field in pending_tx_info: it is used to indicate
 * whether this tx info is the head of one or more coalesced requests.
 *
 * When head != INVALID_PENDING_RING_IDX, it means the start of a new
 * tx requests queue and the end of previous queue.
 *
 * An example sequence of head fields (I = INVALID_PENDING_RING_IDX):
 *
 * ...|0 I I I|5 I|9 I I I|...
 * -->|<-INUSE----------------
 *
 * After consuming the first slot(s) we have:
 *
 * ...|V V V V|5 I|9 I I I|...
 * -----FREE->|<-INUSE--------
 *
 * where V stands for "valid pending ring index". Any number other
 * than INVALID_PENDING_RING_IDX is OK. These entries are considered
 * free and can contain any number other than
 * INVALID_PENDING_RING_IDX. In practice we use 0.
 *
 * The in use non-INVALID_PENDING_RING_IDX (say 0, 5 and 9 in the
 * above example) number is the index into pending_tx_info and
 * mmap_pages arrays.
 */
struct pending_tx_info {
	struct xen_mpi_tx_request req; /* coalesced tx request */
	pending_ring_idx_t head; /* head != INVALID_PENDING_RING_IDX
				  * if it is head of one or more tx
				  * reqs
				  */
};

#define XEN_MPI_TX_RING_SIZE __CONST_RING_SIZE(xen_mpi_tx, PAGE_SIZE)
#define XEN_MPI_RX_RING_SIZE __CONST_RING_SIZE(xen_mpi_rx, PAGE_SIZE)

struct xenmpi_rx_meta {
	int id;
	int size;
};

/* Discriminate from any valid pending_idx value. */
#define INVALID_PENDING_IDX 0xFFFF

#define MAX_BUFFER_OFFSET PAGE_SIZE

struct vmpi_impl_info {
	/* Unique identifier for this interface. */
	domid_t          domid;

	/* Use NAPI for guest TX */
        struct work_struct tx_worker;

	/* When feature-split-event-channels = 0, tx_irq = rx_irq. */
	unsigned int tx_irq;
	/* Only used when feature-split-event-channels = 1 */
	char tx_irq_name[IFNAMSIZ+4]; /* DEVNAME-tx */
	struct xen_mpi_tx_back_ring tx;
        struct vmpi_queue tx_queue;
	struct page *mmap_pages[XEN_MPI_TX_RING_SIZE];
	pending_ring_idx_t pending_prod;
	pending_ring_idx_t pending_cons;
	u16 pending_ring[XEN_MPI_TX_RING_SIZE];
	struct pending_tx_info pending_tx_info[XEN_MPI_TX_RING_SIZE];

	struct gnttab_copy tx_copy_ops[XEN_MPI_TX_RING_SIZE];

	/* Use kthread for guest RX */
	struct task_struct *task;
	wait_queue_head_t wq;
	/* When feature-split-event-channels = 0, tx_irq = rx_irq. */
	unsigned int rx_irq;
	/* Only used when feature-split-event-channels = 1 */
	char rx_irq_name[IFNAMSIZ+4]; /* DEVNAME-rx */
	struct xen_mpi_rx_back_ring rx;
	RING_IDX rx_last_buf_slots;

	/* This array is allocated seperately because it was large
           in netback. That's not true anymore, so it could be
           allocated here. */
	struct gnttab_copy *rx_copy_ops;

	/* We create one rx_meta structure per ring request we consume, so
	 * the maximum number is the same as the ring size.
	 */
	struct xenmpi_rx_meta rx_meta[XEN_MPI_RX_RING_SIZE];
        unsigned int rx_pending_prod;
        unsigned int rx_pending_cons;

	/* Transmit shaping: allow 'credit_bytes' every 'credit_usec'. */
	unsigned long   credit_bytes;
	unsigned long   credit_usec;
	unsigned long   remaining_credit;
	struct timer_list credit_timeout;
	u64 credit_window_start;

	/* Miscellaneous private stuff. */
        struct device *parent;
        struct vmpi_info *mpi;
        struct vmpi_ring *write;
        struct vmpi_queue *read;
        struct vmpi_stats *stats;
        vmpi_read_cb_t read_cb;
        void *read_cb_data;
};

static inline struct xenbus_device *xenmpi_to_xenbus_device(struct vmpi_impl_info *vif)
{
	return to_xenbus_device(vif->parent);
}

struct vmpi_impl_info *xenmpi_alloc(struct device *parent,
			    domid_t domid);

int xenmpi_connect(struct vmpi_impl_info *vif, unsigned long tx_ring_ref,
		   unsigned long rx_ring_ref, unsigned int tx_evtchn,
		   unsigned int rx_evtchn);
void xenmpi_disconnect(struct vmpi_impl_info *vif);
void xenmpi_free(struct vmpi_impl_info *vif);

int xenmpi_xenbus_init(void);
void xenmpi_xenbus_fini(void);

int xenmpi_must_stop_queue(struct vmpi_impl_info *vif);

/* (Un)Map communication rings. */
void xenmpi_unmap_frontend_rings(struct vmpi_impl_info *vif);
int xenmpi_map_frontend_rings(struct vmpi_impl_info *vif,
			      grant_ref_t tx_ring_ref,
			      grant_ref_t rx_ring_ref);

/* Check for SKBs from frontend and schedule backend processing */
void xenmpi_check_rx_xenmpi(struct vmpi_impl_info *vif);

/* Prevent the device from generating any further traffic. */
void xenmpi_carrier_off(struct vmpi_impl_info *vif);

int xenmpi_tx_action(struct vmpi_impl_info *vif, int budget);

int xenmpi_kthread(void *data);
void xenmpi_kick_thread(struct vmpi_impl_info *vif);

/* Determine whether the needed number of slots (req) are available,
 * and set req_event if not.
 */
bool xenmpi_rx_ring_slots_available(struct vmpi_impl_info *vif, int needed);

void xenmpi_stop_queue(struct vmpi_impl_info *vif);

extern bool separate_tx_rx_irq;

#endif /* __XEN_MPIBACK__COMMON_H__ */

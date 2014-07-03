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

#include "xen-mpi-back-common.h"

#include <linux/kthread.h>
#include <linux/ethtool.h>
#include <linux/rtnetlink.h>
#include <linux/if_vlan.h>
#include <linux/vmalloc.h>

#include <xen/events.h>
#include <asm/xen/hypercall.h>


static irqreturn_t xenmpi_tx_interrupt(int irq, void *dev_id)
{
	struct vmpi_impl_info *vif = dev_id;

        IFV(printk("%s\n", __func__));
	if (RING_HAS_UNCONSUMED_REQUESTS(&vif->tx))
		schedule_work(&vif->tx_worker);

	return IRQ_HANDLED;
}

void xenmpi_tx_worker_function(struct work_struct *work)
{
	struct vmpi_impl_info *vif = container_of(work, struct vmpi_impl_info, tx_worker);
        int budget = 64;
	int work_done;

        IFV(printk("%s\n", __func__));

	work_done = xenmpi_tx_action(vif, budget);

        if (work_done >= budget) {
                schedule_work(&vif->tx_worker);
        } else {
		int more_to_do = 0;
		unsigned long flags;

		/* It is necessary to disable IRQ before calling
		 * RING_HAS_UNCONSUMED_REQUESTS. Otherwise we might
		 * lose event from the frontend.
		 *
		 * Consider:
		 *   RING_HAS_UNCONSUMED_REQUESTS
		 *   <frontend generates event to trigger napi_schedule>
		 *   __napi_complete
		 *
		 * This handler is still in scheduled state so the
		 * event has no effect at all. After __napi_complete
		 * this handler is descheduled and cannot get
		 * scheduled again. We lose event in this case and the ring
		 * will be completely stalled.
		 */

		local_irq_save(flags);

		RING_FINAL_CHECK_FOR_REQUESTS(&vif->tx, more_to_do);
		if (more_to_do)
		        schedule_work(&vif->tx_worker);

		local_irq_restore(flags);
        }
}

static irqreturn_t xenmpi_rx_interrupt(int irq, void *dev_id)
{
	struct vmpi_impl_info *vif = dev_id;

        IFV(printk("%s\n", __func__));
	xenmpi_kick_thread(vif);

	return IRQ_HANDLED;
}

static irqreturn_t xenmpi_interrupt(int irq, void *dev_id)
{
	xenmpi_tx_interrupt(irq, dev_id);
	xenmpi_rx_interrupt(irq, dev_id);

	return IRQ_HANDLED;
}

struct vmpi_info *vmpi_info_from_vmpi_impl_info(struct vmpi_impl_info *vif)
{
        return vif->mpi;
}

int vmpi_impl_register_read_callback(vmpi_impl_info_t *vif, vmpi_read_cb_t cb,
                                     void* opaque)
{
        vif->read_cb = cb;
        vif->read_cb_data = opaque;

        return 0;
}

int vmpi_impl_txkick(struct vmpi_impl_info *vif)
{
	int min_slots_needed = 1;

	/* Drop the packet if vif is not ready */
	if (vif->task == NULL)
		goto drop;

        IFV(printk("%s\n", __func__));

	/* If a buffer can't possibly fit in the remaining slots
	 * then turn off the queue to give the ring a chance to
	 * drain.
	 */
	if (!xenmpi_rx_ring_slots_available(vif, min_slots_needed))
		xenmpi_stop_queue(vif);

	xenmpi_kick_thread(vif);

	return 0;

 drop:
	return -1;
}

static void xenmpi_up(struct vmpi_impl_info *vif)
{
	enable_irq(vif->tx_irq);
	if (vif->tx_irq != vif->rx_irq)
		enable_irq(vif->rx_irq);
	xenmpi_check_rx_xenmpi(vif);
}

static void xenmpi_down(struct vmpi_impl_info *vif)
{
	disable_irq(vif->tx_irq);
	if (vif->tx_irq != vif->rx_irq)
		disable_irq(vif->rx_irq);
	del_timer_sync(&vif->credit_timeout);
}

struct vmpi_impl_info *xenmpi_alloc(struct device *parent, domid_t domid)
{
	struct vmpi_impl_info *vif;
	char name[IFNAMSIZ] = {};
	int i;
        int r = -ENOMEM;

	snprintf(name, IFNAMSIZ - 1, "mpi%u", domid);
	vif = kmalloc(sizeof(*vif), GFP_KERNEL);
	if (vif == NULL) {
		pr_warn("Could not allocate mpi for %s\n", name);
		return ERR_PTR(-ENOMEM);
	}
        memset(vif, 0, sizeof(*vif));

        vif->parent = parent;

	vif->rx_copy_ops = vmalloc(sizeof(struct gnttab_copy) *
				     XEN_MPI_RX_RING_SIZE);
	if (vif->rx_copy_ops == NULL) {
		pr_warn("Could not allocate grant copy space for %s\n", name);
                goto grant_copy;
	}

	vif->domid  = domid;

	vif->credit_bytes = vif->remaining_credit = ~0UL;
	vif->credit_usec  = 0UL;
	init_timer(&vif->credit_timeout);
	vif->credit_window_start = get_jiffies_64();

        vmpi_queue_init(&vif->tx_queue, 0, VMPI_BUF_SIZE);

	vif->pending_cons = 0;
	vif->pending_prod = XEN_MPI_TX_RING_SIZE;
	for (i = 0; i < XEN_MPI_TX_RING_SIZE; i++)
		vif->pending_ring[i] = i;
	for (i = 0; i < XEN_MPI_TX_RING_SIZE; i++)
		vif->mmap_pages[i] = NULL;

        INIT_WORK(&vif->tx_worker, xenmpi_tx_worker_function);

        vif->mpi = vmpi_init((void*)vif, &r, false);
        if (vif->mpi == NULL) {
                goto vmpi_ini;
        }
        vif->write = vmpi_get_write_ring(vif->mpi);
        vif->read = vmpi_get_read_queue(vif->mpi);
        vif->read_cb = NULL;
        vif->stats = vmpi_get_stats(vif->mpi);

	__module_get(THIS_MODULE);

        printk("%s: allocated %s\n", __func__, name);

	return vif;

vmpi_ini:
        vmpi_queue_fini(&vif->tx_queue);
        vfree(vif->rx_copy_ops);
grant_copy:
        kfree(vif);

        return ERR_PTR(r);
}

int xenmpi_connect(struct vmpi_impl_info *vif, unsigned long tx_ring_ref,
		   unsigned long rx_ring_ref, unsigned int tx_evtchn,
		   unsigned int rx_evtchn)
{
	struct task_struct *task;
	int err = -ENOMEM;

	BUG_ON(vif->tx_irq);
	BUG_ON(vif->task);

	err = xenmpi_map_frontend_rings(vif, tx_ring_ref, rx_ring_ref);
	if (err < 0)
		goto err;

	init_waitqueue_head(&vif->wq);

	if (tx_evtchn == rx_evtchn) {
		/* feature-split-event-channels == 0 */
		err = bind_interdomain_evtchn_to_irqhandler(
			vif->domid, tx_evtchn, xenmpi_interrupt, 0,
			"xen-mpi", vif);
		if (err < 0)
			goto err_unmap;
		vif->tx_irq = vif->rx_irq = err;
		disable_irq(vif->tx_irq);
	} else {
		/* feature-split-event-channels == 1 */
		snprintf(vif->tx_irq_name, sizeof(vif->tx_irq_name),
			 "%s-tx", "xen-mpi");
		err = bind_interdomain_evtchn_to_irqhandler(
			vif->domid, tx_evtchn, xenmpi_tx_interrupt, 0,
			vif->tx_irq_name, vif);
		if (err < 0)
			goto err_unmap;
		vif->tx_irq = err;
		disable_irq(vif->tx_irq);

		snprintf(vif->rx_irq_name, sizeof(vif->rx_irq_name),
			 "%s-rx", "xen-mpi");
		err = bind_interdomain_evtchn_to_irqhandler(
			vif->domid, rx_evtchn, xenmpi_rx_interrupt, 0,
			vif->rx_irq_name, vif);
		if (err < 0)
			goto err_tx_unbind;
		vif->rx_irq = err;
		disable_irq(vif->rx_irq);
	}

	task = kthread_create(xenmpi_kthread,
			      (void *)vif, "%s", "xen-mpi");
	if (IS_ERR(task)) {
		pr_warn("Could not allocate kthread for %s\n", "xen-mpi");
		err = PTR_ERR(task);
		goto err_rx_unbind;
	}

	vif->task = task;

	xenmpi_up(vif);

	wake_up_process(vif->task);

        printk("%s: completed\n", __func__);

	return 0;

err_rx_unbind:
	unbind_from_irqhandler(vif->rx_irq, vif);
	vif->rx_irq = 0;
err_tx_unbind:
	unbind_from_irqhandler(vif->tx_irq, vif);
	vif->tx_irq = 0;
err_unmap:
	xenmpi_unmap_frontend_rings(vif);
err:
	module_put(THIS_MODULE);
	return err;
}

void xenmpi_carrier_off(struct vmpi_impl_info *vif)
{
	xenmpi_down(vif);
}

void xenmpi_disconnect(struct vmpi_impl_info *vif)
{
	xenmpi_carrier_off(vif);

	if (vif->task) {
		kthread_stop(vif->task);
		vif->task = NULL;
	}

	if (vif->tx_irq) {
		if (vif->tx_irq == vif->rx_irq)
			unbind_from_irqhandler(vif->tx_irq, vif);
		else {
			unbind_from_irqhandler(vif->tx_irq, vif);
			unbind_from_irqhandler(vif->rx_irq, vif);
		}
		vif->tx_irq = 0;
	}

	xenmpi_unmap_frontend_rings(vif);

        printk("%s: completed\n", __func__);
}

void xenmpi_free(struct vmpi_impl_info *vif)
{
        vmpi_fini(vif->mpi, false);
        vmpi_queue_fini(&vif->tx_queue);

        cancel_work_sync(&vif->tx_worker);

	vfree(vif->rx_copy_ops);
        kfree(vif);

	module_put(THIS_MODULE);

        printk("%s: freed\n", __func__);
}

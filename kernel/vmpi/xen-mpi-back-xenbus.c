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

struct backend_info {
	struct xenbus_device *dev;
	struct vmpi_impl_info *vif;

	/* This is the state that will be reflected in xenstore when any
	 * active hotplug script completes.
	 */
	enum xenbus_state state;

	enum xenbus_state frontend_state;
};

static int connect_rings(struct backend_info *);
static void connect(struct backend_info *);
static void backend_create_xenmpi(struct backend_info *be);
static void set_backend_state(struct backend_info *be,
			      enum xenbus_state state);

static int mpiback_remove(struct xenbus_device *dev)
{
	struct backend_info *be = dev_get_drvdata(&dev->dev);

	set_backend_state(be, XenbusStateClosed);

	if (be->vif) {
		kobject_uevent(&dev->dev.kobj, KOBJ_OFFLINE);
		xenbus_rm(XBT_NIL, dev->nodename, "hotplug-status");
		xenmpi_free(be->vif);
		be->vif = NULL;
	}
	kfree(be);
	dev_set_drvdata(&dev->dev, NULL);

        printk("%s: completed\n", __func__);

	return 0;
}


/**
 * Entry point to this code when a new device is created.  Allocate the basic
 * structures and switch to InitWait.
 */
static int mpiback_probe(struct xenbus_device *dev,
			 const struct xenbus_device_id *id)
{
	const char *message;
	struct xenbus_transaction xbt;
	int err;
	int sg;
	struct backend_info *be = kzalloc(sizeof(struct backend_info),
					  GFP_KERNEL);
	if (!be) {
		xenbus_dev_fatal(dev, -ENOMEM,
				 "allocating backend structure");
		return -ENOMEM;
	}

	be->dev = dev;
	dev_set_drvdata(&dev->dev, be);

	sg = 1;

	do {
		err = xenbus_transaction_start(&xbt);
		if (err) {
			xenbus_dev_fatal(dev, err, "starting transaction");
			goto fail;
		}

		/* We support rx-copy path. */
		err = xenbus_printf(xbt, dev->nodename,
				    "feature-rx-copy", "%d", 1);
		if (err) {
			message = "writing feature-rx-copy";
			goto abort_transaction;
		}

		err = xenbus_transaction_end(xbt, 0);
	} while (err == -EAGAIN);

	if (err) {
		xenbus_dev_fatal(dev, err, "completing transaction");
		goto fail;
	}

	/*
	 * Split event channels support, this is optional so it is not
	 * put inside the above loop.
	 */
	err = xenbus_printf(XBT_NIL, dev->nodename,
			    "feature-split-event-channels",
			    "%u", separate_tx_rx_irq);
	if (err)
		pr_debug("Error writing feature-split-event-channels\n");

	err = xenbus_switch_state(dev, XenbusStateInitWait);
	if (err)
		goto fail;

	be->state = XenbusStateInitWait;

	/* This kicks hotplug scripts, so do it immediately. */
	backend_create_xenmpi(be);

        printk("%s: completed\n", __func__);

	return 0;

abort_transaction:
	xenbus_transaction_end(xbt, 1);
	xenbus_dev_fatal(dev, err, "%s", message);
fail:
	pr_debug("failed\n");
	mpiback_remove(dev);
	return err;
}

static void backend_create_xenmpi(struct backend_info *be)
{
	int err;
	struct xenbus_device *dev = be->dev;

	if (be->vif != NULL)
		return;

	be->vif = xenmpi_alloc(&dev->dev, dev->otherend_id);
	if (IS_ERR(be->vif)) {
		err = PTR_ERR(be->vif);
		be->vif = NULL;
		xenbus_dev_fatal(dev, err, "creating interface");
		return;
	}
}

static void backend_disconnect(struct backend_info *be)
{
	if (be->vif)
		xenmpi_disconnect(be->vif);
}

static void backend_connect(struct backend_info *be)
{
	if (be->vif)
		connect(be);
}

static inline void backend_switch_state(struct backend_info *be,
					enum xenbus_state state)
{
	struct xenbus_device *dev = be->dev;

	pr_debug("%s -> %s\n", dev->nodename, xenbus_strstate(state));
	be->state = state;

	/* If we are waiting for a hotplug script then defer the
	 * actual xenbus state change.
	 */
	if (1)
		xenbus_switch_state(dev, state);
}

/* Handle backend state transitions:
 *
 * The backend state starts in InitWait and the following transitions are
 * allowed.
 *
 * InitWait -> Connected
 *
 *    ^    \         |
 *    |     \        |
 *    |      \       |
 *    |       \      |
 *    |        \     |
 *    |         \    |
 *    |          V   V
 *
 *  Closed  <-> Closing
 *
 * The state argument specifies the eventual state of the backend and the
 * function transitions to that state via the shortest path.
 */
static void set_backend_state(struct backend_info *be,
			      enum xenbus_state state)
{
	while (be->state != state) {
		switch (be->state) {
		case XenbusStateClosed:
			switch (state) {
			case XenbusStateInitWait:
			case XenbusStateConnected:
				pr_info("%s: prepare for reconnect\n",
					be->dev->nodename);
				backend_switch_state(be, XenbusStateInitWait);
				break;
			case XenbusStateClosing:
				backend_switch_state(be, XenbusStateClosing);
				break;
			default:
				BUG();
			}
			break;
		case XenbusStateInitWait:
			switch (state) {
			case XenbusStateConnected:
				backend_connect(be);
				backend_switch_state(be, XenbusStateConnected);
				break;
			case XenbusStateClosing:
			case XenbusStateClosed:
				backend_switch_state(be, XenbusStateClosing);
				break;
			default:
				BUG();
			}
			break;
		case XenbusStateConnected:
			switch (state) {
			case XenbusStateInitWait:
			case XenbusStateClosing:
			case XenbusStateClosed:
				backend_disconnect(be);
				backend_switch_state(be, XenbusStateClosing);
				break;
			default:
				BUG();
			}
			break;
		case XenbusStateClosing:
			switch (state) {
			case XenbusStateInitWait:
			case XenbusStateConnected:
			case XenbusStateClosed:
				backend_switch_state(be, XenbusStateClosed);
				break;
			default:
				BUG();
			}
			break;
		default:
			BUG();
		}
	}

        printk("%s: backend --> %s\n", __func__, xenbus_strstate(state));
}

/**
 * Callback received when the frontend's state changes.
 */
static void frontend_changed(struct xenbus_device *dev,
			     enum xenbus_state frontend_state)
{
	struct backend_info *be = dev_get_drvdata(&dev->dev);

	pr_debug("%s -> %s\n", dev->otherend, xenbus_strstate(frontend_state));

	be->frontend_state = frontend_state;

	switch (frontend_state) {
	case XenbusStateInitialising:
		set_backend_state(be, XenbusStateInitWait);
		break;

	case XenbusStateInitialised:
		break;

	case XenbusStateConnected:
		set_backend_state(be, XenbusStateConnected);
		break;

	case XenbusStateClosing:
		set_backend_state(be, XenbusStateClosing);
		break;

	case XenbusStateClosed:
		set_backend_state(be, XenbusStateClosed);
		if (xenbus_dev_is_online(dev))
			break;
		/* fall through if not online */
	case XenbusStateUnknown:
		set_backend_state(be, XenbusStateClosed);
		device_unregister(&dev->dev);
		break;

	default:
		xenbus_dev_fatal(dev, -EINVAL, "saw state %d at frontend",
				 frontend_state);
		break;
	}
}


static void xen_net_read_rate(struct xenbus_device *dev,
			      unsigned long *bytes, unsigned long *usec)
{
	char *s, *e;
	unsigned long b, u;
	char *ratestr;

	/* Default to unlimited bandwidth. */
	*bytes = ~0UL;
	*usec = 0;

	ratestr = xenbus_read(XBT_NIL, dev->nodename, "rate", NULL);
	if (IS_ERR(ratestr))
		return;

	s = ratestr;
	b = simple_strtoul(s, &e, 10);
	if ((s == e) || (*e != ','))
		goto fail;

	s = e + 1;
	u = simple_strtoul(s, &e, 10);
	if ((s == e) || (*e != '\0'))
		goto fail;

	*bytes = b;
	*usec = u;

	kfree(ratestr);
	return;

 fail:
	pr_warn("Failed to parse network rate limit. Traffic unlimited.\n");
	kfree(ratestr);
}

static void connect(struct backend_info *be)
{
	int err;
	struct xenbus_device *dev = be->dev;

	err = connect_rings(be);
	if (err)
		return;

	xen_net_read_rate(dev, &be->vif->credit_bytes,
			  &be->vif->credit_usec);
	be->vif->remaining_credit = be->vif->credit_bytes;
}


static int connect_rings(struct backend_info *be)
{
	struct vmpi_impl_info *vif = be->vif;
	struct xenbus_device *dev = be->dev;
	unsigned long tx_ring_ref, rx_ring_ref;
	unsigned int tx_evtchn, rx_evtchn, rx_copy;
	int err;

	err = xenbus_gather(XBT_NIL, dev->otherend,
			    "tx-ring-ref", "%lu", &tx_ring_ref,
			    "rx-ring-ref", "%lu", &rx_ring_ref, NULL);
	if (err) {
		xenbus_dev_fatal(dev, err,
				 "reading %s/ring-ref",
				 dev->otherend);
		return err;
	}

	/* Try split event channels first, then single event channel. */
	err = xenbus_gather(XBT_NIL, dev->otherend,
			    "event-channel-tx", "%u", &tx_evtchn,
			    "event-channel-rx", "%u", &rx_evtchn, NULL);
	if (err < 0) {
		err = xenbus_scanf(XBT_NIL, dev->otherend,
				   "event-channel", "%u", &tx_evtchn);
		if (err < 0) {
			xenbus_dev_fatal(dev, err,
					 "reading %s/event-channel(-tx/rx)",
					 dev->otherend);
			return err;
		}
		rx_evtchn = tx_evtchn;
	}

	err = xenbus_scanf(XBT_NIL, dev->otherend, "request-rx-copy", "%u",
			   &rx_copy);
	if (err == -ENOENT) {
		err = 0;
		rx_copy = 0;
	}
	if (err < 0) {
		xenbus_dev_fatal(dev, err, "reading %s/request-rx-copy",
				 dev->otherend);
		return err;
	}
	if (!rx_copy)
		return -EOPNOTSUPP;

	/* Map the shared frame, irq etc. */
	err = xenmpi_connect(vif, tx_ring_ref, rx_ring_ref,
			     tx_evtchn, rx_evtchn);
	if (err) {
		xenbus_dev_fatal(dev, err,
				 "mapping shared-frames %lu/%lu port tx %u rx %u",
				 tx_ring_ref, rx_ring_ref,
				 tx_evtchn, rx_evtchn);
		return err;
	}
	return 0;
}


/* ** Driver Registration ** */


static const struct xenbus_device_id mpiback_ids[] = {
	{ "mpi" },
	{ "" }
};


static struct xenbus_driver mpiback_driver = {
        .ids = mpiback_ids,
	.probe = mpiback_probe,
	.remove = mpiback_remove,
	.otherend_changed = frontend_changed,
};

int xenmpi_xenbus_init(void)
{
	return xenbus_register_backend(&mpiback_driver);
}

void xenmpi_xenbus_fini(void)
{
	return xenbus_unregister_driver(&mpiback_driver);
}

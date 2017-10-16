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

#include <linux/atomic.h>
#include <linux/list.h>
#include <linux/export.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/random.h>
#include <linux/spinlock.h>

#define CDRR_NAME	"cdrr"
#define RINA_PREFIX 	"dtcp-ps-cdrr"

#include "logs.h"
#include "connection.h"
#include "dtp.h"
#include "dtp-utils.h"
#include "dtp.h"
#include "rds/rmem.h"
#include "dtcp-ps.h"
#include "dtcp-conf-utils.h"
#include "du.h"

/* This identifies the link maximum capacity at 10msec, 1Gb/s
 */
static unsigned int cdrr_link_rate = 95000;

/* This identifies profile of congestion reaction to use,
 * 0 = harsh, 1 = soft, congestion based.
 */
static unsigned int cdrr_mode = 0;

/* Master kobject for CDRR. */
static struct kobject cdrr_kobj;

/* Rate info. */
struct cdrr_rate_info {
	/* Rate kernel object. */
	struct kobject robj;
	/* The DTCP bound to the rate. */
	struct dtcp * dtcp;

	/* 0 is direct mgb swap, 1 is "congestion load" based. */
	int mode;

	/* Frame of time, in msec, after that the credit will be renewed. */
	unsigned int time_frame;
	/* Minimum granted bandwidth. */
	unsigned int mgb;
	/* Reset rate for this instance. */
	unsigned int reset_rate;

	/* When we check the last time? */
	struct timespec last;

	/* The volume of congested bytes/pdu received. */
	unsigned int congested;
	/* The volume of congested bytes/pdu received. */
	unsigned int total;
	/* Volume of the feedback bytes/pdu intentionally sent.*/
	unsigned int feedback;
};

/*
 * Kobject defs (taken from linux/samples/kobject)
 */

/* Kernel object for the policy. */
struct kobject cdrr_obj;

/* Attribute used in CDRR policy. */
struct cdrr_attribute {
	struct attribute attr;
	ssize_t (* show)(
		struct kobject * kobj,
		struct attribute * attr,
		char * buf);
	ssize_t (* store)(
		struct kobject * foo,
		struct attribute * attr,
		const char * buf,
		size_t count);
};

/* Show procedure! */
static ssize_t cdrr_attr_show(
	struct kobject * kobj,
	struct attribute * attr,
	char * buf) {

	struct cdrr_rate_info * ri = 0;

	/*
	 * CDRR global attribute.
	 */

	if(strcmp(attr->name, "link_limit") == 0) {
		return snprintf(
			buf, PAGE_SIZE, "%u\n", cdrr_link_rate);
	}

	if(strcmp(attr->name, "mode") == 0) {
		return snprintf(
			buf, PAGE_SIZE, "%u\n", cdrr_mode);
	}

	/*
	 * Per-instance attribute.
	 */

	if(strcmp(attr->name, "mgb") == 0) {
		ri = container_of(kobj, struct cdrr_rate_info, robj);
		return snprintf(buf, PAGE_SIZE, "%u\n", ri->mgb);
	}

	if(strcmp(attr->name, "time_frame") == 0) {
		ri = container_of(kobj, struct cdrr_rate_info, robj);
		return snprintf(buf, PAGE_SIZE, "%u\n", ri->time_frame);
	}

	if(strcmp(attr->name, "reset_rate") == 0) {
		ri = container_of(kobj, struct cdrr_rate_info, robj);
		return snprintf(buf, PAGE_SIZE, "%u\n", ri->reset_rate);
	}

	if(strcmp(attr->name, "total") == 0) {
		ri = container_of(kobj, struct cdrr_rate_info, robj);
		return snprintf(buf, PAGE_SIZE, "%u\n", ri->total);
	}

	if(strcmp(attr->name, "feedback") == 0) {
		ri = container_of(kobj, struct cdrr_rate_info, robj);
		return snprintf(buf, PAGE_SIZE, "%u\n", ri->feedback);
	}

	if(strcmp(attr->name, "congested") == 0) {
		ri = container_of(kobj, struct cdrr_rate_info, robj);
		return snprintf(buf, PAGE_SIZE, "%u\n", ri->congested);
	}

	if(strcmp(attr->name, "current_rate") == 0) {
		ri = container_of(kobj, struct cdrr_rate_info, robj);
		return snprintf(
			buf, PAGE_SIZE, "%u\n", ri->dtcp->sv->sndr_rate);
	}

	return 0;
}

/* Store procedure! */
static ssize_t cdrr_attr_store(
	struct kobject * kobj,
	struct attribute * attr,
	const char * buf,
	size_t count) {

	int op = 0;
	int l = 0;

	struct cdrr_rate_info * ri = 0;

	if(strcmp(attr->name, "reset_rate") == 0) {
		ri = container_of(kobj, struct cdrr_rate_info, robj);
		op = kstrtoint(buf, 10, &l);

		if(op < 0) {
			LOG_ERR("Failed to set the reset rate.");
			return op;
		}

		/* Single flow rate is now changed! */
		ri->reset_rate = l;
	}

	/* The reaction mode of the policy. */
	if(strcmp(attr->name, "mode") == 0) {
		op = kstrtoint(buf, 10, &l);

		if(op < 0) {
			LOG_ERR("Failed to set the policy reaction mode.");
			return op;
		}

		cdrr_mode = l;

		LOG_INFO("Cdrr mode switched to %d", cdrr_mode);
	}

	if(strcmp(attr->name, "link_limit") == 0) {
		op = kstrtoint(buf, 10, &l);

		if(op < 0) {
			LOG_ERR("Failed to set the link limit rate.");
			return op;
		}

		/* Link rate is now changed! */
		cdrr_link_rate = l;

		LOG_INFO("Cdrr link limit set to %d", cdrr_link_rate);
	}

	return count;
}

/* Sysfs operations. */
static const struct sysfs_ops cdrr_sysfs_ops = {
	.show  = cdrr_attr_show,
	.store = cdrr_attr_store,
};

/*
 * Attributes visible in CDRR sysfs directory.
 */

static struct cdrr_attribute cdrr_ll_attr =
	__ATTR(link_limit, 0664, cdrr_attr_show, cdrr_attr_store);

static struct cdrr_attribute cdrr_m_attr =
	__ATTR(mode, 0664, cdrr_attr_show, cdrr_attr_store);

static struct attribute * cdrr_attrs[] = {
	&cdrr_ll_attr.attr,
	&cdrr_m_attr.attr,
	NULL,
};

/*
 * Attributes visible per DTCP instance.
 */

static struct cdrr_attribute cdrr_tf_attr =
	__ATTR(time_frame, 0444, cdrr_attr_show, cdrr_attr_store);

static struct cdrr_attribute cdrr_rr_attr =
	__ATTR(reset_rate, 0664, cdrr_attr_show, cdrr_attr_store);

static struct cdrr_attribute cdrr_mgb_attr =
	__ATTR(mgb, 0444, cdrr_attr_show, cdrr_attr_store);

static struct cdrr_attribute cdrr_tb_attr =
	__ATTR(total, 0444, cdrr_attr_show, cdrr_attr_store);

static struct cdrr_attribute cdrr_cb_attr =
	__ATTR(congested, 0444, cdrr_attr_show, cdrr_attr_store);

static struct cdrr_attribute cdrr_fb_attr =
	__ATTR(feedback, 0444, cdrr_attr_show, cdrr_attr_store);

static struct cdrr_attribute cdrr_cc_attr =
	__ATTR(current_rate, 0444, cdrr_attr_show, cdrr_attr_store);

static struct attribute * cdrr_dtcp_attrs[] = {
	&cdrr_tf_attr.attr,
	&cdrr_rr_attr.attr,
	&cdrr_mgb_attr.attr,
	&cdrr_tb_attr.attr,
	&cdrr_cb_attr.attr,
	&cdrr_fb_attr.attr,
	&cdrr_cc_attr.attr,
	NULL,
};

/* Operations associated with the release of the kobj. */
static void cdrr_release(struct kobject *kobj) {
	/* Nothing... */
}

/* Master CDRR ktype, different for the type of shown attributes. */
static struct kobj_type cdrr_ktype = {
	.sysfs_ops = &cdrr_sysfs_ops,
	.release = cdrr_release,
	.default_attrs = cdrr_attrs,
};

/* DTCP CDRR ktype, different for the type of shown attributes. */
static struct kobj_type cdrr_dtcp_ktype = {
	.sysfs_ops = &cdrr_sysfs_ops,
	.release = cdrr_release,
	.default_attrs = cdrr_dtcp_attrs,
};

/*
 * Utilities:
 */

static inline unsigned long cdrr_to_ms(struct timespec * t) {
	return (t->tv_sec * 1000) + (t->tv_nsec / 1000000);
}

static int cdrr_send_control(struct dtcp * dtcp) {
	struct du * du = pdu_ctrl_generate(dtcp, PDU_TYPE_FC);

	if (!du) {
		return -1;
	}

	if (dtcp_pdu_send(dtcp, du)) {
	       return -1;
	}

	return 0;
}

/*
 * Callbacks:
 */

static int cdrr_rate_reduction(struct dtcp_ps * ps, const struct pci * pci) {
	struct cdrr_rate_info * ri = (struct cdrr_rate_info *)ps->priv;
	struct dtcp * dtcp = ri->dtcp;
	/* struct timespec ts = {0}; */

	/* Counts the PDUs. */
	ssize_t size = 1;

	int c = 0;

#if 0
	int cong_step = 0;
	int cong_class = 0;
	int rate_step =  0;
	int rate_add = 0;
#endif

	/* Act only where you receive data. */
	if(!(pci_type(pci) & PDU_TYPE_DT)) {
		return 0;
	}

	c = pci_flags_get(pci) & PDU_FLAGS_EXPLICIT_CONGESTION;

	/* Mode = 0
	 * Adapt as fast as you can to the registered rates.
	 */
	if(ri->mode == 0) {
		if(c) {
			/* MGB not set yet? */
			spin_lock_bh(&dtcp->parent->sv_lock);
			if(dtcp->sv->sndr_rate != ri->mgb) {
				dtcp->sv->sndr_rate = ri->mgb;
				spin_unlock_bh(&dtcp->parent->sv_lock);
				cdrr_send_control(dtcp);
				ri->feedback++;
			} else {
				spin_unlock_bh(&dtcp->parent->sv_lock);
			}
		} else {
			/* Reset rate not set yet? */
			spin_lock_bh(&dtcp->parent->sv_lock);
			if(dtcp->sv->sndr_rate != ri->reset_rate) {
				dtcp->sv->sndr_rate = ri->reset_rate;
				spin_unlock_bh(&dtcp->parent->sv_lock);
				cdrr_send_control(dtcp);
				ri->feedback++;
			}else {
				spin_unlock_bh(&dtcp->parent->sv_lock);
			}
		}
	}

	/* getnstimeofday(&ts); */

	/* Congestion detected!
	 * Just count those bytes as the congested received one.
	 */
	if(c) {
		ri->congested += size;
	}

	ri->total += size;

#if 0
	/* Mode = 1
	 * Rate set between link max rate and MGB.
	 */
	if (ri->mode == 1) {
		cong_step = ri->total / 10;

		if(cong_step <= 0) {
			cong_class = 0;
		} else {
			cong_class = ri->congested / cong_step;
		}

		rate_step =  (ri->reset_rate - ri->mgb) / 10;
		rate_add = rate_step * (10 - cong_class);

		/* Is the current rate in the same class of the estimated one?
		 */
		if(dtcp_sndr_rate(ri->dtcp) != ri->mgb + rate_add) {
			dtcp_sndr_rate_set(dtcp, ri->mgb + rate_add);

			/* Try to send a FC PDU in order to react to the
			 * congestion.
			 */
			cdrr_send_control(dtcp);
			ri->feedback++;
		}
	}
#endif

	return 0;
}

static int cdrr_param(
	struct ps_base * bps, const char * name, const char * value) {

	struct dtcp_ps * ps = container_of(bps, struct dtcp_ps, base);
	struct cdrr_rate_info * ri = ps->priv;

	int ret = 0;
	int ival = 0;

	if (strcmp(name, "mgb") == 0) {
		ret = kstrtoint(value, 10, &ival);
		if (!ret) {
			ri->mgb = ival;
		}
	}

	if (strcmp(name, "reset_rate") == 0) {
		ret = kstrtoint(value, 10, &ival);
		if (!ret) {
			ri->reset_rate = ival;
		}
	}

	return 0;
}

/*
 * Creation/destruction of the policy.
 */

static struct ps_base * cdrr_create(struct rina_component * component) {
        struct dtcp * dtcp = dtcp_from_component(component);
        struct dtcp_ps * ps = rkzalloc(sizeof(*ps), GFP_KERNEL);
        struct cdrr_rate_info * ri = 0;

        if (!ps) {
                return NULL;
        }

        ps->base.set_policy_set_param   = cdrr_param;
        ps->dm                          = dtcp;

        ps->flow_init                   = NULL;
        ps->lost_control_pdu            = NULL;
        ps->rtt_estimator               = NULL;
        ps->retransmission_timer_expiry = NULL;
        ps->received_retransmission     = NULL;
        ps->sender_ack                  = NULL;
        ps->sending_ack                 = NULL;
        ps->receiving_ack_list          = NULL;
        ps->initial_rate                = NULL;
        ps->receiving_flow_control      = NULL;
        ps->update_credit               = NULL;
#ifdef CONFIG_RINA_DTCP_RCVR_ACK
        ps->rcvr_ack                    = NULL,
#endif
#ifdef CONFIG_RINA_DTCP_RCVR_ACK_ATIMER
        ps->rcvr_ack                    = NULL,
#endif
        ps->rcvr_flow_control           = NULL;
        ps->rate_reduction              = cdrr_rate_reduction;
        ps->rcvr_control_ack            = NULL;
        ps->no_rate_slow_down           = NULL;
        ps->no_override_default_peak    = NULL;

        ri = rkzalloc(sizeof(struct cdrr_rate_info), GFP_KERNEL);

        if(!ri) {
        	LOG_ERR("No more memory.");
        	kfree(ps);

        	return 0;
        }

        memset(ri, 0, sizeof(struct cdrr_rate_info));

        ps->priv = ri;

        ri->mode 	= cdrr_mode;
        ri->reset_rate 	= cdrr_link_rate;
        ri->dtcp 	= dtcp;
        ri->mgb  	= dtcp_sending_rate(dtcp->cfg);
        ri->time_frame	= dtcp_time_period(dtcp->cfg);

        getnstimeofday(&ri->last);

        if(kobject_init_and_add(
		&ri->robj, &cdrr_dtcp_ktype, &cdrr_kobj, "%pK", dtcp)) {

        	LOG_ERR("Failed to create sysfs for dtcp 0x%pK", dtcp);

        	rkfree(ri);
        	rkfree(ps);
        	return NULL;
        }

        LOG_INFO("CDRRI > New DTCP, "
		"dtcp: %pK, "
		"mgb: %u, "
		"time frame: %u msec, "
		"reset rate: %u, ",
		ri->dtcp,
		ri->mgb,
		ri->time_frame,
		ri->reset_rate);

        return &ps->base;
}

static void cdrr_destroy(struct ps_base * bps) {
        struct dtcp_ps *ps = container_of(bps, struct dtcp_ps, base);
        struct cdrr_rate_info * ri = (struct cdrr_rate_info *)ps->priv;

        if(ri) {
		kobject_put(&ri->robj);

		rkfree(ri);
	}

        if (bps) {
                rkfree(ps);
        }

	return;
}

struct ps_factory dtcp_factory = {
        .owner	 = THIS_MODULE,
        .create  = cdrr_create,
        .destroy = cdrr_destroy,
};

/*
 * Module initialization/release.
 */

static int __init mod_init(void) {
	int ret = 0;

        strcpy(dtcp_factory.name, CDRR_NAME);

        LOG_INFO("CDRR policy set loaded successfully");

        ret = dtcp_ps_publish(&dtcp_factory);
        if (ret) {
        	LOG_INFO("Failed to publish CDRR policy set factory");
                return -1;
        }

        if(kobject_init_and_add(&cdrr_kobj, &cdrr_ktype, 0, "cdrr")) {
        	LOG_ERR("Failed to create cdrr sysfs main entry.");
        }

        return 0;
}

static void __exit mod_exit(void) {
        int ret;

        kobject_put(&cdrr_kobj);

        ret = dtcp_ps_unpublish(CDRR_NAME);

        LOG_INFO("CDRR policy set successfully removed");

        if (ret) {
        	LOG_INFO("Failed to unpublish CDRR policy set factory");
                return;
        }
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("Congestion Detection Rate Reduction policies");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kewin Rausch <kewin.rausch@create-net.org>");
MODULE_VERSION("1");

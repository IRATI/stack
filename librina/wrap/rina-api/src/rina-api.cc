/*
 *  POSIX-like RINA API, wrapper for IRATI librina
 *
 *    Vincenzo Maffione   <v.maffione@nextworks.it>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <iostream>
#include <string>
#include <cassert>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <librina/librina.h>
#include <rina/api.h>
#include "ctrl.h"
#include "irati/kucommon.h"
#include "irati/serdes-utils.h"
#include "irati/kernel-msg.h"

using namespace rina;
using namespace std;

extern "C"
{

#define APIDBG
#undef APIDBG

int rina_open(void)
{
        return irati_open_ctrl_port(0);
}

#define RINA_REG_EVENT_ID   0x7a6b /* casual value, used just for assert() */

static void irati_msg_fill_common(struct irati_msg_base * msg, int wfd)
{
	if (!msg)
		return;

	msg->dest_ipcp_id = 0;
	msg->src_ipcp_id = 0;
	msg->dest_port = IPCM_CTRLDEV_PORT;
	msg->src_port = get_app_ctrl_port_from_cfd(wfd);
}

static int
irati_register_req_fill(struct irati_msg_app_reg_app *req, const char *dif_name,
		        const char *appl_name, int wfd, int fd)
{
	struct name * dn, * appn, *dan;

	if (!req)
		return -1;

	dn = rina_name_create();
	if (!dn)
		return -1;

	if (dif_name && rina_name_from_string(dif_name, dn)) {
		rina_name_free(dn);
		return -1;
	}

	appn = rina_name_create();
	if (!appn) {
		rina_name_free(dn);
		return -1;
	}

	if (appl_name && rina_name_from_string(appl_name, appn)) {
		rina_name_free(dn);
		rina_name_free(appn);
		return -1;
	}

	dan = rina_name_create();
	if (!dan) {
		rina_name_free(dn);
		rina_name_free(appn);
		return -1;
	}

	memset(req, 0, sizeof(*req));
	irati_msg_fill_common(IRATI_MB(req), wfd);
	req->msg_type = RINA_C_APP_REGISTER_APPLICATION_REQUEST;
	req->event_id = RINA_REG_EVENT_ID;
	req->fa_ctrl_port = get_app_ctrl_port_from_cfd(fd);
	req->dif_name = dn;
	req->app_name = appn;
	req->daf_name = dan;
	req->pid = getpid();
	if (dif_name)
		req->reg_type = rina::APPLICATION_REGISTRATION_SINGLE_DIF;
	else
		req->reg_type = rina::APPLICATION_REGISTRATION_ANY_DIF;

	return 0;
}

static int
irati_unregister_req_fill(struct irati_msg_app_reg_app_resp *req, const char *dif_name,
		          const char *appl_name, int wfd)
{
	struct name * dn, * appn;

	if (!req)
		return -1;

	dn = rina_name_create();
	if (!dn)
		return -1;

	if (dif_name && rina_name_from_string(dif_name, dn)) {
		rina_name_free(dn);
		return -1;
	}

	appn = rina_name_create();
	if (!appn) {
		rina_name_free(dn);
		return -1;
	}

	if (appl_name && rina_name_from_string(appl_name, appn)) {
		rina_name_free(dn);
		rina_name_free(appn);
		return -1;
	}

	memset(req, 0, sizeof(*req));
	irati_msg_fill_common(IRATI_MB(req), wfd);
	req->msg_type = RINA_C_APP_UNREGISTER_APPLICATION_REQUEST;
	req->event_id = RINA_REG_EVENT_ID;
	req->dif_name = dn;
	req->app_name = appn;

	return 0;
}

int rina_register_wait(int fd, int wfd)
{
	struct irati_msg_app_reg_app_resp *resp;
	int8_t response;
	int ret = -1;

	resp = (struct irati_msg_app_reg_app_resp *) irati_read_next_msg(wfd);
	if (!resp) {
		goto out;
	}

	assert(resp->msg_type == RINA_C_APP_REGISTER_APPLICATION_RESPONSE ||
			resp->msg_type == RINA_C_APP_UNREGISTER_APPLICATION_RESPONSE);
	assert(resp->event_id == RINA_REG_EVENT_ID);
	response = resp->result;
	irati_ctrl_msg_free(IRATI_MB(resp));

	if (response) {
		errno = EBUSY;
		goto out;
	} else {
		ret = 0;
	}

out:
	close (wfd);

	return ret;
}

static int
rina_register_common(int fd, const char *dif_name, const char *local_appl,
                     int flags, bool reg)
{
	void * req = 0;
	int ret = 0;
	int wfd;

	if (flags & ~(RINA_F_NOWAIT)) {
		errno = EINVAL;
		return -1;
	}

	/* Open a dedicated file descriptor to perform the operation and wait
	 * for the response. */
	wfd = rina_open();
	if (wfd < 0) {
		return wfd;
	}

	if (reg) {
		req = new irati_msg_app_reg_app();
		ret = irati_register_req_fill((struct irati_msg_app_reg_app*) req,
				              dif_name, local_appl, wfd, fd);
	} else {
		req = new irati_msg_app_reg_app_resp();
		ret = irati_unregister_req_fill((struct irati_msg_app_reg_app_resp*) req,
			              	        dif_name, local_appl, wfd);
	}

	if (ret) {
		errno = ENOMEM;
		irati_ctrl_msg_free(IRATI_MB(req));
		goto out;
	}

	/* Issue the request ad wait for the response. */
	ret = irati_write_msg(wfd, IRATI_MB(req));
	irati_ctrl_msg_free(IRATI_MB(req));
	if (ret < 0) {
		goto out;
	}

	if (flags & RINA_F_NOWAIT) {
		return wfd; /* Return the file descriptor to wait on. */
	}

	/* Wait for the operation to complete right now. */
	return rina_register_wait(fd, wfd);
out:
	close_port(wfd);

	return ret;
}

int
rina_register(int fd, const char *dif_name, const char *local_appl, int flags)
{
	return rina_register_common(fd, dif_name, local_appl, flags, true);
}

int
rina_unregister(int fd, const char *dif_name, const char *local_appl, int flags)
{
	return rina_register_common(fd, dif_name, local_appl, flags, false);
}

#define RINA_FA_EVENT_ID    0x6271 /* casual value, used just for assert() */

static int
irati_fa_req_fill(struct irati_kmsg_ipcm_allocate_flow *req, const char *dif_name,
		  const char *local_appl, const char *remote_appl,
		  const struct rina_flow_spec *flowspec, int wfd)
{
	struct name * dn, * ln, * rn;

	if (!req)
		return -1;

	dn = rina_name_create();
	if (!dn)
		return -1;

	if (dif_name && rina_name_from_string(dif_name, dn)) {
		rina_name_free(dn);
		return -1;
	}

	ln = rina_name_create();
	if (!ln) {
		rina_name_free(dn);
		return -1;
	}

	if (local_appl && rina_name_from_string(local_appl, ln)) {
		rina_name_free(dn);
		rina_name_free(ln);
		return -1;
	}

	rn = rina_name_create();
	if (!rn) {
		rina_name_free(dn);
		rina_name_free(ln);
		return -1;
	}

	if (remote_appl && rina_name_from_string(remote_appl, rn)) {
		rina_name_free(dn);
		rina_name_free(ln);
		rina_name_free(rn);
		return -1;
	}

	memset(req, 0, sizeof(*req));
	irati_msg_fill_common(IRATI_MB(req), wfd);
	req->msg_type = RINA_C_IPCM_ALLOCATE_FLOW_REQUEST;
	req->event_id = RINA_FA_EVENT_ID;
	req->dif_name = dn;
	req->local = ln;
	req->remote = rn;
	req->fspec = new flow_spec();
	if (flowspec) {
		req->fspec->average_bandwidth = flowspec->avg_bandwidth;
		req->fspec->average_sdu_bandwidth = 0;
		req->fspec->delay = flowspec->max_delay;
		req->fspec->jitter = flowspec->max_jitter;
		req->fspec->loss = flowspec->max_loss;
		req->fspec->max_allowable_gap = flowspec->max_sdu_gap;
		req->fspec->undetected_bit_error_rate = flowspec->max_loss;
		req->fspec->ordered_delivery = flowspec->in_order_delivery;
		req->fspec->msg_boundaries = flowspec->msg_boundaries;
	} else {
		req->fspec->average_bandwidth = 0;
		req->fspec->average_sdu_bandwidth = 0;
		req->fspec->delay = 0;
		req->fspec->jitter = 0;
		req->fspec->loss = 10000;
		req->fspec->max_allowable_gap = 10;
		req->fspec->ordered_delivery = false;
		req->fspec->undetected_bit_error_rate = 0;
		req->fspec->partial_delivery = true;
		req->fspec->msg_boundaries = false;
	}

	req->pid = getpid();

	return 0;
}

int rina_flow_alloc_wait(int wfd)
{
	struct irati_msg_app_alloc_flow_result *resp;
	int ret = -1;

	resp = (struct irati_msg_app_alloc_flow_result *) irati_read_next_msg(wfd);
	if (!resp && errno == EAGAIN) {
		/* Nothing to read, propagate the error without closing wfd,
		 * because the caller will call us again. */
		return ret;
	}
	if (!resp) {
		goto out;
	}

	assert(resp->msg_type == RINA_C_APP_ALLOCATE_FLOW_REQUEST_RESULT);
	assert(resp->event_id == RINA_FA_EVENT_ID);

	if (resp->port_id < 0) {
		errno = EPERM;
	} else {
		ret = irati_open_io_port(resp->port_id);
	}

	irati_ctrl_msg_free(IRATI_MB(resp));
out:
	close(wfd);

	return ret;
}

int
rina_flow_alloc(const char *dif_name, const char *local_appl,
		const char *remote_appl, const struct rina_flow_spec *flowspec,
		unsigned int flags)
{
	struct irati_kmsg_ipcm_allocate_flow * req;
	int wfd, ret;

	if (flowspec && flowspec->version != RINA_FLOW_SPEC_VERSION) {
		errno = EINVAL;
		return -1;
	}

	if (flags & ~(RINA_F_NOWAIT)) {
		errno = EINVAL;
		return -1;
	}

	wfd = rina_open();
	if (wfd < 0) {
		return wfd;
	}

	req = new irati_kmsg_ipcm_allocate_flow();
	ret = irati_fa_req_fill(req, dif_name, local_appl,
			        remote_appl, flowspec, wfd);
	if (ret) {
		errno = ENOMEM;
		return -1;
	}

	ret = irati_write_msg(wfd, IRATI_MB(req));
	irati_ctrl_msg_free(IRATI_MB(req));
	if (ret < 0) {
		close(wfd);
		return ret;
	}

	if (flags & RINA_F_NOWAIT) {
		/* Return the control file descriptor. */
		return wfd;
	}

	/* Return the I/O file descriptor (or an error). */
	return rina_flow_alloc_wait(wfd);
}

/* Split accept lock and pending lists. */
static volatile char sa_lock_var = 0;
static int sa_handle = 0;
static unsigned int sa_pending_len = 0;
struct sa_list {
	struct list_head sa_handles;
};
static struct sa_list sa_pending;
bool sa_init = false;
#define SA_PENDING_MAXLEN   (1 << 11)

struct sa_pending_item {
	int handle;
	struct irati_kmsg_ipcm_allocate_flow *req;
	struct list_head node;
};

static void sa_lock(void)
{
	while (__sync_lock_test_and_set(&sa_lock_var, 1)) {
		/* Memory barrier is implicit into the compiler built-in.
		 * We could also use the newer __atomic_test_and_set() built-in. */
	}
}

static void sa_unlock(void)
{
    /* Stores 0 in the lock variable. We could also use the newer
     * __atomic_clear() built-in. */
    __sync_lock_release(&sa_lock_var);
}

static int
irati_fa_resp_fill(struct irati_msg_app_alloc_flow_response *req,
		   uint32_t event_id, int resp, int fd)
{
	req->msg_type = RINA_C_APP_ALLOCATE_FLOW_RESPONSE;
	irati_msg_fill_common(IRATI_MB(req), fd);
	req->event_id = event_id;
	req->result = resp;
	req->not_source = true;
	req->pid = getpid();

	return 0;
}

int
rina_flow_accept(int fd, char **remote_appl, struct rina_flow_spec *spec,
		 unsigned int flags)
{
	struct irati_kmsg_ipcm_allocate_flow *req = NULL;
	struct sa_pending_item * spi = NULL;
	struct irati_msg_app_alloc_flow_response * resp;
	int ffd = -1;
	int ret;

	if (remote_appl) {
		*remote_appl = NULL;
	}

	if (spec) {
		if (spec->version != RINA_FLOW_SPEC_VERSION) {
			errno = EINVAL;
			return -1;
		}
		memset(spec, 0, sizeof(*spec));
	}

	if (flags & ~(RINA_F_NORESP)) { /* wrong flags */
		errno = EINVAL;
		return -1;
	}

	if (flags & RINA_F_NORESP) {
		if (sa_pending_len >= SA_PENDING_MAXLEN) {
			errno = ENOSPC;
			return -1;
		}

		spi = new sa_pending_item();
		if (!spi) {
			errno = ENOMEM;
			return -1;
		}
		memset(spi, 0, sizeof(*spi));
	}

	req = (struct irati_kmsg_ipcm_allocate_flow *) irati_read_next_msg(fd);
	if (!req) {
		goto out0;
	}

	assert(req->msg_type == RINA_C_APP_ALLOCATE_FLOW_REQUEST_ARRIVED);

	if (remote_appl) {
		* remote_appl = rina_name_to_string(req->remote);
		if (!(*remote_appl)) {
			goto out1;
		}
	}

	if (spec) {
		spec->avg_bandwidth = req->fspec->average_bandwidth;
		spec->in_order_delivery = req->fspec->ordered_delivery;
		spec->max_delay = req->fspec->delay;
		spec->max_jitter = req->fspec->jitter;
		spec->max_loss = req->fspec->loss;
		spec->max_sdu_gap = req->fspec->max_allowable_gap;
	}

	if (flags & RINA_F_NORESP) {
		sa_lock();
		if (!sa_init) {
			INIT_LIST_HEAD(&sa_pending.sa_handles);
			sa_init = true;
		}
		spi->req = req;
		spi->handle = sa_handle ++;
		if (sa_handle < 0) { /* Overflow */
			sa_handle = 0;
		}
		list_add_tail(&spi->node, &sa_pending.sa_handles);
		sa_pending_len ++;
		sa_unlock();

		return spi->handle;
	}

	resp = new irati_msg_app_alloc_flow_response();
	irati_fa_resp_fill((irati_msg_app_alloc_flow_response*) resp,
			   req->event_id, 0, fd);
	ret = irati_write_msg(fd, IRATI_MB(resp));
	if (ret < 0) {
		goto out2;
	}

	ffd = irati_open_io_port(req->port_id);

out2:
	irati_ctrl_msg_free(IRATI_MB(resp));
out1:
	irati_ctrl_msg_free(IRATI_MB(req));
	out0:
	if (spi) {
		free(spi);
	}

	return ffd;
}

int rina_flow_respond(int fd, int handle, int response)
{
	struct sa_pending_item *cur, *spi = NULL;
	struct irati_kmsg_ipcm_allocate_flow *req;
	struct irati_msg_app_alloc_flow_response * resp;
	int ffd = -1;
	int ret;

	sa_lock();
	list_for_each_entry(cur, &sa_pending.sa_handles, node) {
		if (handle == cur->handle) {
			spi = cur;
			list_del(&spi->node);
			sa_pending_len --;
			break;
		}
	}
	sa_unlock();

	if (spi == NULL) {
		errno = EINVAL;
		return -1;
	}

	req = spi->req;
	free(spi);

	resp = new irati_msg_app_alloc_flow_response();
	irati_fa_resp_fill((irati_msg_app_alloc_flow_response*) resp,
			   req->event_id, response, fd);
	ret = irati_write_msg(fd, IRATI_MB(resp));
	if (ret < 0) {
		goto out;
	}

	if (response == 0) {
		/* Positive response, open an I/O device. */
		ffd = irati_open_io_port(req->port_id);
	} else {
		/* Negative response, just return 0. */
		ffd = 0;
	}
out:
	irati_ctrl_msg_free(IRATI_MB(req));
	irati_ctrl_msg_free(IRATI_MB(resp));

	return ffd;
}

void
rina_flow_spec_default(struct rina_flow_spec *spec)
{
        memset(spec, 0, sizeof(*spec));
        spec->version = RINA_FLOW_SPEC_VERSION;
        spec->max_sdu_gap = -1;
        spec->avg_bandwidth = 0;
        spec->max_delay = 0;
        spec->max_jitter = 0;
        spec->max_loss = 10000;
        spec->in_order_delivery = 0;
        spec->msg_boundaries = 1;
}

unsigned int
rina_flow_mss_get(int fd)
{
	struct irati_iodev_ctldata data;

	data.port_id = 0;

	if (ioctl(fd, IRATI_IOCTL_MSS_GET, &data)) {
		data.port_id = 0;
	}

	return data.port_id;
}

}

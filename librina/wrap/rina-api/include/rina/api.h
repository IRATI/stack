/*
 * API for applications
 *
 * Copyright (C) 2015-2016 Nextworks
 * Author: Vincenzo Maffione <v.maffione@gmail.com>
 *
 * This file is part of rlite.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifndef __RINA_API_H__
#define __RINA_API_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*
 * A POSIX-like RINA API for applications.
 *
 * The API functions typically return 0 or a positive value on success.
 * On error, -1 is returned with the errno variable set accordingly to
 * the specific error.
 *
 * Each application name is specified using a C string, where the name’s
 * components (Application Process Name, Application Process Instance,
 * Application Entity Name and Applicatiion Entity Instance) are separated
 * by the | separator (pipe). The separator can be omitted if it is only
 * used to separate empty strings or a non-empty string from an empty
 * string. Valid strings are for instance "aa|bb|cc|dd", "aa|bb||",
 * "aa|bb", "aa".
 */

/*
 * The rina_flow_spec struct specifies the flow QoS parameters asked
 * by an application that issue a flow allocation request.
 */
struct rina_flow_spec {
    uint32_t version; /* version number to allow for extensions */
#define RINA_FLOW_SPEC_VERSION 1
    uint64_t max_sdu_gap;   /* in SDUs */
    uint64_t avg_bandwidth; /* in bits per second */
    uint32_t max_delay;     /* in microseconds */
#define RINA_FLOW_SPEC_LOSS_MAX 10000
    uint16_t max_loss;         /* from 0 (0%) to 10000 (100%) */
    uint32_t max_jitter;       /* in microseconds */
    uint8_t in_order_delivery; /* boolean */
    uint8_t msg_boundaries;    /* boolean */
};

#define RINA_F_NOWAIT (1 << 0)
#define RINA_F_NORESP (1 << 1)

/*
 * Open a file descriptor that can be used to register/unregister names,
 * and to manage incoming flow allocation requests. On success, it
 * returns a file descriptor that can be later provided to rina_register(),
 * rina_unregister(), rina_flow_accept(), and rina_flow_respond().
 * On error -1 is returned.
 * This function is typically used on the "server side" of applications.
 */
int rina_open(void);

/*
 * Register the application name @local_appl to a DIF in the system.
 * After a successful registration, flow allocation requests can be received
 * on @fd. If @dif_name is not NULL, the system may register the
 * application to @dif_name. However, the @dif_name argument is only
 * advisory and the implementation is free to ignore it. If DIF is
 * NULL, the system autonomously decide to which DIF @local_appl will
 * be registered to.
 *
 * If RINA_F_NOWAIT is not specified in @flags, this function will block
 * the caller until the operation is complete, and 0 is returned on success.
 *
 * If RINA_F_NOWAIT is specified in @flags, the function returns a
 * file descriptor (different from @fd) which can be used to wait
 * for the operation to complete (e.g. using POLLIN with poll() or
 * select()). In this case the operation can be completed by a subsequent
 * call to rina_register_wait().
 *
 * On error -1 is returned, with the errno code properly set.
 */
int rina_register(int fd, const char *dif_name, const char *local_appl,
                  int flags);

/*
 * Unregister the application name @local_appl from the DIF where it
 * was registered. The @dif_name argument must match the one passed
 * to rina_register(). After a successful unregistration, flow allocation
 * requests can no longer be received on @fd.
 * The meaning of the RINA_F_NOWAIT flag is the same as in rina_register().
 *
 * Returns 0 on success, -1 on error, with the errno code properly set.
 */
int rina_unregister(int fd, const char *dif_name, const char *local_appl,
                    int flags);

/*
 * Wait for the completion of a (un)registration procedure previosuly initiated
 * with a call to rina_[un]register() on @fd with the RINA_F_NOWAIT flag set.
 * The @wfd file descriptor must match the one returned by rina_[un]register().
 *
 * On success it returns 0, on error -1, with the errno code properly set.
 */
int rina_register_wait(int fd, int wfd);

/*
 * Accept an incoming flow request arrived on @fd. If @flags does not contain
 * RINA_F_NORESP, it also sends a positive response to the requesting
 * application; otherwise, the response (positive or negative) can be sent by
 * a subsequent call to the rina_flow_respond().
 * On success, the char* pointed by remote_appl, if not NULL, is assigned the
 * name of the requesting application. The memory for the requestor name is
 * allocated by the callee and must be freed by the caller. Moreover, if
 * @spec is not NULL, the referenced data structure is filled with the QoS
 * specification specified by the requesting application.
 *
 * If @flags does not contain RINA_F_NORESP, on success this function returns
 * a file descriptor that can be subsequently used with standard I/O system
 * calls (write(), read(), select(), ...) to exchange SDUs on the flow and
 * synchronize.
 *
 * If @flags does contain RINA_F_NORESP, on success a positive number is
 * returned as an handle to be passed to a subsequent call to
 * rina_flow_respond().
 *
 * A call to rina_flow_accept(fd, &x, flags & ~RINA_F_NORESP) is functionally
 * equivalent to:
 *     h = rina_flow_accept(sfd, &x, flags | RINA_F_NORESP);
 *     cfd = rina_flow_respond(sfd, h, 0);
 *
 * On error -1 is returned, with the errno code properly set.
 */
int rina_flow_accept(int fd, char **remote_appl, struct rina_flow_spec *spec,
                     unsigned int flags);

/*
 * Emit a verdict on the flow allocation request identified by @handle,
 * that was previously received on @fd by calling rina_flow_accept() with
 * the RINA_F_NORESP flag set. A zero @response indicates a positive
 * response, which completes the flow allocation procedure. A non-zero
 * @response indicates that the flow allocation request is denied. In both
 * cases @response is sent to the requesting application to inform it
 * about the verdict. When the response is positive, on success this
 * function returns a file descriptor that can be subsequently used with
 * standard I/O system calls (write(), read(), select(), ...) to exchange
 * SDUs on the flow and synchronize. When the response is negative, 0 is
 * returned on success. In any case, -1 is returned on error, with the
 * errno code properly set.
 */
int rina_flow_respond(int fd, int handle, int response);

/*
 * Issue a flow allocation request towards the destination application called
 * @remote_appl, using @local_appl as a source application name. If @flowspec
 * is not NULL, it specifies the QoS parameters to be used for the flow, if the
 * flow allocation request is successful. If @flowspec is NULL, an
 * implementation-specific default QoS will be assumed (which typically
 * corresponds to a best-effort QoS).
 * If @dif_name is not NULL the system may look for @remote_appl in a DIF
 * called @dif_name. However, the @dif_name argument is only advisory and
 * the system is free to ignore it and take an autonomous decision.
 *
 * If @flags specifies RINA_F_NOWAIT, a call to this function does not wait
 * until the completion of the flow allocation procedure; on success, it just
 * returns a "control" file descriptor that can be subsequently fed to
 * rina_flow_alloc_wait() to wait for completion and obtain the flow I/O file
 * descriptor. Moreover, the control file descriptor can be used with poll(),
 * select() and similar.
 *
 * If @ flags does not specify RINA_F_NOWAIT, a call to this function waits
 * until the flow allocation procedure is complete. On success, it returns
 * a file descriptor that can be subsequently used with standard I/O system
 * calls (write(), read(), select(), ...) to exchange SDUs on the flow and
 * synchronize.
 *
 * In any case, -1 is returned on error, with the errno code properly set.
 */
int rina_flow_alloc(const char *dif_name, const char *local_appl,
                    const char *remote_appl,
                    const struct rina_flow_spec *flowspec, unsigned int flags);

/*
 * Wait for the completion of a flow allocation procedure previosuly initiated
 * with a call to rina_flow_alloc() with the RINA_F_NOWAIT flag set. The @wfd
 * file descriptor must match the one returned by rina_flow_alloc().
 *
 * On success, it returns a file descriptor that can be subsequently used with
 * standard I/O system calls (write(), read(), select(), ...) to exchange SDUs
 * on the flow and synchronize.
 *
 * On error -1 is returned, with the errno code properly set.
 */
int rina_flow_alloc_wait(int wfd);

/*
 * Fills in the provided @spec with an unrelable best-effort QoS.
 */
void rina_flow_spec_unreliable(struct rina_flow_spec *spec);

/* Deprecated function. */
#define rina_flow_spec_default rina_flow_spec_unreliable

/*
 * Retrieve the MSS (Maximum SDU Size) that can be written to the flow
 * with a single write. Returns 0 on error with errno set properly.
 */
unsigned int rina_flow_mss_get(int fd);

#ifdef __cplusplus
}
#endif

#endif /* __RINA_API_H__ */

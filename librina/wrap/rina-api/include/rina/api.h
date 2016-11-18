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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __RINA_API_H__
#define __RINA_API_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The rina_flow_spec struct specifies the flow QoS parameters asked
 * by an application that issue a flow allocation request.
 * Some spare space is reserved to allow future ABI-compatible extensions.
 */
struct rina_flow_spec {
    uint64_t max_sdu_gap;       /* in SDUs */
    uint64_t avg_bandwidth;     /* in bits per second */
    uint32_t max_delay;         /* in microseconds */
    uint32_t max_jitter;        /* in microseconds */
    uint8_t in_order_delivery;  /* boolean */

    uint8_t reserved[39];       /* for future use */
};

/*
 * Open a file descriptor that can be used to register/unregister names,
 * and to manage incoming flow allocation requests. On success, it
 * returns a file descriptor that can be later provided to rina_register(),
 * rina_unregister(), rina_flow_accept(), rina_flow_wait() and
 * rina_flow_respond(). On error -1 is returned.
 * This function is typically used on the "server side" of applications.
 */
int rina_open(void);

/*
 * Register the application name @local_appl to a DIF in the system.
 * After a successful invocation, flow allocation requests can be received
 * on @fd. If @dif_name is not NULL, the system may register the
 * application to @dif_name. However, the @dif_name argument is only
 * advisory and the implementation is free to ignore it. If DIF is
 * NULL, the system autonomously decide to which DIF @local_appl will
 * be registered to.
 * Returns 0 on success, -1 on error, with the errno code properly set.
 */
int rina_register(int fd, const char *dif_name, const char *local_appl);

/*
 * Unregister the application name @local_appl from the DIF where it
 * was registered. The @dif_name argument must match the one passed
 * to rina_register(). After a successful invocation, flow allocation
 * requests can no longer be received on @fd.
 * Returns 0 on success, -1 on error, with the errno code properly set.
 */
int rina_unregister(int fd, const char *dif_name, const char *local_appl);

/*
 * Accept a flow request arrived on @fd, sending a positive response to
 * the requesting application. On success, the char* pointed by remote_appl,
 * if not NULL, is assigned the name of the requesting application. The
 * memory for the requestor name is allocated by the calle and must be
 * freed by the caller.
 *
 * A call to rina_flow_accept(fd, &x) is functionally equivalent to:
 *     h = rina_flow_wait(sfd, &x);
 *     cfd = rina_flow_respond(sfd, h, 0);
 *
 * On success, it returns a file descriptor that can be subsequently used
 * with standard I/O system calls (write(), read(), select(), ...) to
 * exchange SDUs on the flow and synchronize. On error -1 is returned,
 * with the errno code properly set.
 *
 */
int rina_flow_accept(int fd, char **remote_appl);

/*
 * Receive a flow allocation request arrived on @fd, without accepting it
 * or issuing a negative verdict. On success, the char* pointed by remote_appl,
 * if not NULL, is assigned the name of the requesting application. The
 * memory for the requestor name is allocated by the calle and must be
 * freed by the caller.
 *
 * On error, -1 is returned. On success, a positive number is returned as
 * an handle to be passed to subsequent call to rina_flow_respond().
 */
int rina_flow_wait(int fd, char **remote_appl);

/*
 * Emit a verdict on the flow allocation request identified by @handle,
 * that was previously received on @fd. A zero @response indicates a positive
 * response, which completes the flow allocation procedure. A non-zero
 * @response indicates that the flow allocation request is denied. In both
 * cases @response is sent to the requesting application to inform it
 * about the verdict. When the response is positive, this function returns
 * a file descriptor that can be subsequently used with standard I/O system
 * calls (write(), read(), select(), ...) to exchange SDUs on the flow and
 * synchronize. When the response is negative, 0 is returned. On error -1
 * is returned, with the errno code properly set.
 */
int rina_flow_respond(int fd, int handle, int response);

/*
 * Allocate a flow towards the destination application called @remote_appl,
 * using @local_appl as a source application name. If @flowspec is not NULL,
 * it specifies the QoS parameters to be used for the flow, if the flow
 * allocation request is successful. If @flowspec is NULL, an
 * implementation-specific default QoS will be assumed (which typically
 * corresponds to a best-effort QoS).
 * If @dif_name is not NULL the system may look for @remote_appl in a DIF
 * called @dif_name. However, the @dif_name argument is only advisory and
 * the system is free to ignore it and take an autonomous decision.
 *
 * On success, it returns a file descriptor that can be subsequently used
 * with standard I/O system calls (write(), read(), select(), ...) to
 * exchange SDUs on the flow and synchronize. On error -1 is returned,
 * with the errno code properly set.
 */
int rina_flow_alloc(const char *dif_name, const char *local_appl,
              const char *remote_appl, const struct rina_flow_spec *flowspec);

/*
 * Fills in the provided @spec with an implementation-specific default QoS,
 * which typically corresponds to a best-effort QoS.
 */
void rina_flow_spec_default(struct rina_flow_spec *spec);

#ifdef __cplusplus
}
#endif

#endif  /* __RINA_API_H__ */

/*
 * Functionalities to interact with the IRATI ctrl-device
 *
 * Copyright (C) 2015-2016 Nextworks
 * Author: Vincenzo Maffione <v.maffione@gmail.com>
 * Author: Eduard Grasa <eduard.grasa@i2cat.net>
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <sys/ioctl.h>

#define RINA_PREFIX "librina.ctrldev"

#include "librina/logs.h"
#include "ctrl.h"

#include "irati/kernel-msg.h"

struct irati_msg_base * irati_read_next_msg(int cfd)
{
	unsigned int max_resp_size = irati_numtables_max_size(
			irati_ker_numtables,
			sizeof(irati_ker_numtables)/sizeof(struct irati_msg_layout));
	struct irati_msg_base *resp;
	char serbuf[8192];
	int ret;

	ret = read(cfd, serbuf, sizeof(serbuf));
	if (ret < 0) {
		LOG_ERR("read(cfd)");
		return NULL;
	}

	/* Here we can malloc the maximum kernel message size. */
	resp = IRATI_MB(malloc(max_resp_size));
	if (!resp) {
		LOG_ERR("Out of memory\n");
		errno = ENOMEM;
		return NULL;
	}

	/* Deserialize the message from serbuf into resp. */
	ret = deserialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
			serbuf, ret, (void *)resp, max_resp_size);
	if (ret) {
		LOG_ERR("Problems during deserialization [%d]\n", ret);
		free(resp);
		errno = ENOMEM;
		return NULL;
	}

	return resp;
}

int irati_write_msg(int cfd, struct irati_msg_base *msg)
{
	char serbuf[8192];
	unsigned int serlen;
	int ret;

	/* Serialize the message. */
	serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX, msg);
	if (serlen > sizeof(serbuf)) {
		LOG_ERR("Serialized message would be too long [%u]\n", serlen);
		errno = EINVAL;
		return -1;
	}
	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, msg);

	ret = write(cfd, serbuf, serlen);
	if (ret < 0) {
		LOG_ERR("write(cfd)");
	} else if (ret != serlen) {
		/* This should never happen if kernel code is correct. */
		LOG_ERR("Error: partial write [%d/%u]\n", ret, serlen);
		ret = -1;
	} else {
		ret = 0;
	}

	return ret;
}

static int open_port_common(bool ipcm)
{
	struct irati_ctrldev_ctldata info;
	int fd;
	int ret;

	fd = open(IRATI_CTRLDEV_NAME, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "open(%s) failed: %s\n", IRATI_CTRLDEV_NAME,
				strerror(errno));
		return -1;
	}

	if (ipcm)
		info.port_id = IRATI_IPCM_PORT;
	else
		info.port_id = fd;

	ret = ioctl(fd, IRATI_CTRL_FLOW_BIND, &info);
	if (ret) {
		fprintf(stderr, "ioctl(%s) failed: %s\n", IRATI_CTRLDEV_NAME,
				strerror(errno));
		return -1;
	}

	return fd;
}

int irati_open_appl_ipcp_port()
{
	return open_port_common(false);
}

int irati_open_ipcm_port()
{
	return open_port_common(true);
}

void irati_ctrl_msg_free(struct irati_msg_base *msg)
{
	irati_msg_free(irati_ker_numtables, RINA_C_MAX, msg);
}

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
#include <errno.h>
#include <sys/ioctl.h>

#define RINA_PREFIX "librina.ctrldev"

#include "librina/logs.h"
#include "librina/concurrency.h"
#include "ctrl.h"

#include "irati/kernel-msg.h"

#define IRATI_MAX_CTRL_MSG_SIZE 1000000

struct irati_msg_base * irati_read_next_msg(int cfd)
{
	struct irati_msg_base *resp;
	char * serbuf;
	uint32_t size;
	int ret;

	ret = read(cfd, &size, 0);
	if (ret <= 0) {
		LOG_ERR("read(cfd) returned %d", ret);
		return NULL;
	}

	serbuf = malloc(size);
	if (!serbuf) {
		LOG_ERR("Cannot allocate memory");
		errno = ENOMEM;
		return NULL;
	}

	ret = read(cfd, serbuf, size);
	if (ret <= 0) {
		LOG_ERR("read(cfd) returned %d", ret);
		free(serbuf);
		return NULL;
	}

	/* Here we can malloc the maximum kernel message size. */
	resp = (struct irati_msg_base *) deserialize_irati_msg(irati_ker_numtables,
							       RINA_C_MAX,
							       serbuf, ret);
	free(serbuf);

	if (!resp) {
		LOG_ERR("Problems during deserialization [%d]\n", ret);
		errno = ENOMEM;
		return NULL;
	}

	return resp;
}

int irati_write_msg(int cfd, struct irati_msg_base *msg)
{
	char * serbuf;
	int ret;
	unsigned int serlen;

	/* Serialize the message. */
	serlen = irati_msg_serlen(irati_ker_numtables, RINA_C_MAX, msg);
	if (serlen > IRATI_MAX_CTRL_MSG_SIZE) {
		LOG_ERR("Serialized message would be too long [%u]\n", serlen);
		errno = EINVAL;
		return -1;
	}

	serbuf = malloc(serlen);
	if (!serbuf) {
		LOG_ERR("Cannot allocate memory");
		errno = ENOMEM;
		return -1;
	}

	serlen = serialize_irati_msg(irati_ker_numtables, RINA_C_MAX,
				     serbuf, msg);

	ret = write(cfd, serbuf, serlen);
	free(serbuf);
	if (ret < 0) {
		LOG_ERR("write(cfd)");
		errno = EFAULT;
	} else if (ret != serlen) {
		/* This should never happen if kernel code is correct. */
		LOG_ERR("Error: partial write [%d/%u]\n", ret, serlen);
		errno = EFAULT;
		ret = -1;
	} else {
		ret = 0;
	}

	return ret;
}

int close_port(int cfd)
{
	return close(cfd);
}

irati_msg_port_t get_app_ctrl_port_from_cfd(int cfd)
{
	return getpid()*1000 + cfd;
}

int irati_open_ctrl_port(irati_msg_port_t port_id)
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

	if (port_id == 0)
		info.port_id = get_app_ctrl_port_from_cfd(fd);
	else
		info.port_id = port_id;

	ret = ioctl(fd, IRATI_CTRL_FLOW_BIND, &info);
	if (ret) {
		fprintf(stderr, "ioctl(%s) failed: %s\n", IRATI_CTRLDEV_NAME,
				strerror(errno));
		return -1;
	}

	return fd;
}

void irati_ctrl_msg_free(struct irati_msg_base *msg)
{
	irati_msg_free(irati_ker_numtables, RINA_C_MAX, msg);
	free(msg);
}

int irati_open_io_port(int port_id)
{
        struct irati_iodev_ctldata iodata;
        int fd;

        if (port_id < 0) {
                /* This happens in case of flow allocation failure. Don't
                 * open the I/O device, just set the file descriptor to an
                 * invalid value. */
                return -1;
        }

        fd = open("/dev/irati", O_RDWR);
        if (fd < 0)
                return fd;

        iodata.port_id = (uint32_t) port_id;
        if (ioctl(fd, IRATI_FLOW_BIND, &iodata)) {
                close(fd);
                return -1;
        }

        return fd;
}

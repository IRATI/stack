/*
 *  VMPI
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
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

#ifndef VMPI_H
#define VMPI_H

typedef uint32_t vq_id_t;
typedef uint32_t vc_id_t;

struct vq_caps;

bool    vq_caps_get(vq_id_t vq, struct vq_caps * caps);

int     vq_write(vq_id_t vq, void * data, size_t length);
void    vq_notify_pending(vq_id_t vq,
                          void  (*cb)(vq_id_t vq));
int     vq_read(vq_id_t vq, void ** data, size_t * length);

/* Generic VQ functionalities */
vq_id_t vq_allocate(void);
void    vq_release(vq_id_t id);
bool    vq_is_ok(vq_id_t vq);

/* Generic VC functionalities */
vc_id_t vc_allocate(vq_id_t vq);
void    vc_release(vq_id_t vq, vc_id_t vc);
bool    vc_is_ok(vq_id_t vq, vc_id_t vc);

struct vc_caps;
bool    vc_caps_get(vq_id_t vq, vc_id_t vc, struct vc_caps * caps);

/* DCC (VQ and VC) related functionalities */
void    dcc_ingress(vq_id_t * vq, vc_id_t * vc);
vq_id_t dcc_egress(vq_id_t * vq, vc_id_t * vc);

/* Message transfer */
int     vc_write(vq_id_t vq, vc_id_t vc, void * data, size_t length);
void    vc_notify_pending(vq_id_t vq,
                          vc_id_t vc,
                          void  (*cb)(vq_id_t vq, vc_id_t vc));
int     vc_read(vq_id_t vq, vc_id_t vc, void ** data, size_t * length);

#endif

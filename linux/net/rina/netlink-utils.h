/*
 * NetLink related utilities
 *
 *    Leonardo Bergesio <leonardo.bergesio@i2cat.net>
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

#ifndef RINA_NETLINK_UTILS_H
#define RINA_NETLINK_UTILS_H

/* These definitions must be moved in the user-space exported headers */

enum app_alloc_flow_req_arrived_attrs {
        AAFRA_ATTR_SOURCE_APP_NAME = 1,
        AAFRA_ATTR_DEST_APP_NAME,
        AAFRA_ATTR_FLOW_SPEC,
        AAFRA_ATTR_PORT_ID,
        AAFRA_ATTR_DIF_NAME,
        __AAFRA_ATTR_MAX,
};

/* FIXME: is it really needed ? */
#define AAFRA_ATTR_MAX (__AAFRA_ATTR_MAX - 1)

enum app_name_info_attrs {
        APNI_ATTR_PROCESS_NAME = 1,
        APNI_ATTR_PROCESS_INSTANCE,
        APNI_ATTR_ENTITY_NAME,
        APNI_ATTR_ENTITY_INSTANCE,
        __APNI_ATTR_MAX,
};

/* FIXME: is it really needed ? */
#define APNI_ATTR_MAX (__APNI_ATTR_MAX - 1)

enum flow_spec_attrs {
        FSPEC_ATTR_AVG_BWITH = 1,
        FSPEC_ATTR_AVG_SDU_BWITH,
        FSPEC_ATTR_DELAY,
        FSPEC_ATTR_JITTER,
        FSPEC_ATTR_MAX_GAP,
        FSPEC_ATTR_MAX_SDU_SIZE,
        FSPEC_ATTR_IN_ORD_DELIVERY,
        FSPEC_ATTR_PART_DELIVERY,
        FSPEC_ATTR_PEAK_BWITH_DURATION,
        FSPEC_ATTR_PEAK_SDU_BWITH_DURATION,
        FSPEC_ATTR_UNDETECTED_BER,
        __FSPEC_ATTR_MAX,
};

/* FIXME: is it really needed ? */
#define FSPEC_ATTR_MAX (__FSPEC_ATTR_MAX - 1)

int rnl_format_app_alloc_flow_req_arrived(struct sk_buff * msg,
                                          struct name      source,
                                          struct name      dest,
                                          struct flow_spec fspec,
                                          port_id_t        id,
                                          struct name      dif_name);

#endif

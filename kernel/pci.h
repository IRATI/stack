/*
 * Protocol Control Information
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
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

#ifndef RINA_PCI_H
#define RINA_PCI_H

#include <linux/types.h>

#include "common.h"
#include "ipcp-instances.h"

typedef uint8_t version_t;

#define PDU_FLAGS_EXPLICIT_CONGESTION 0x01
#define PDU_FLAGS_DATA_RUN            0x80
/* To be truely defined; internal to stack, needs to be discussed */
#define PDU_FLAGS_BAD                 0xFF

typedef uint8_t pdu_flags_t;

/* Data Transfer PDUs */
#define PDU_TYPE_DT            0x80 /* Data Transfer PDU */
/* Control PDUs */
#define PDU_TYPE_CACK          0xC0 /* Control Ack PDU */
/* Ack/Flow Control PDUs */
#define PDU_TYPE_ACK           0xC1 /* ACK only */
#define PDU_TYPE_NACK          0xC2 /* Forced Retransmission PDU (NACK) */
#define PDU_TYPE_FC            0xC4 /* Flow Control only */
#define PDU_TYPE_ACK_AND_FC    0xC5 /* ACK and Flow Control */
#define PDU_TYPE_NACK_AND_FC   0xC6 /* NACK and Flow Control */
/* Selective Ack/Nack PDUs */
#define PDU_TYPE_SACK          0xC9 /* Selective ACK */
#define PDU_TYPE_SNACK         0xCA /* Selective NACK */
#define PDU_TYPE_SACK_AND_FC   0xCD /* Selective ACK and Flow Control */
#define PDU_TYPE_SNACK_AND_FC  0xCE /* Selective NACK and Flow Control */
/* Rendezvous PDU */
#define PDU_TYPE_RENDEZVOUS    0xCF /* Rendezvous */
/* Management PDUs */
#define PDU_TYPE_MGMT          0x40 /* Management */
/* Number of different PDU types */
#define PDU_TYPES              12

typedef uint8_t pdu_type_t;

#define pdu_type_is_ok(X)                                \
	((X == PDU_TYPE_DT)         ? true :             \
	 ((X == PDU_TYPE_CACK)       ? true :            \
	  ((X == PDU_TYPE_SACK)       ? true :           \
	   ((X == PDU_TYPE_NACK)       ? true :          \
	    ((X == PDU_TYPE_FC)         ? true :         \
	     ((X == PDU_TYPE_ACK)        ? true :        \
	      ((X == PDU_TYPE_ACK_AND_FC) ? true :       \
	       ((X == PDU_TYPE_SACK_AND_FC) ? true :     \
		((X == PDU_TYPE_SNACK_AND_FC) ? true :   \
		 ((X == PDU_TYPE_RENDEZVOUS)   ? true :  \
		  ((X == PDU_TYPE_MGMT)         ? true : \
		   false)))))))))))

#define pdu_type_is_control(X)                           \
	((X == PDU_TYPE_CACK)       ? true :             \
	 ((X == PDU_TYPE_SACK)       ? true :            \
	  ((X == PDU_TYPE_NACK)       ? true :           \
	   ((X == PDU_TYPE_FC)         ? true :          \
	    ((X == PDU_TYPE_ACK)        ? true :         \
	     ((X == PDU_TYPE_ACK_AND_FC) ? true :        \
	      ((X == PDU_TYPE_SACK_AND_FC) ? true :      \
	       ((X == PDU_TYPE_RENDEZVOUS)  ? true :     \
	        ((X == PDU_TYPE_SNACK_AND_FC) ? true :   \
		 false)))))))))

struct pci {
	unsigned char *h; /* do not move from 1st position */
	size_t len;
};

ssize_t	* pci_offset_table_create(struct dt_cons *dt_cons);
bool pci_is_ok(const struct pci *pci);
ssize_t	pci_calculate_size(struct efcp_config *cfg,pdu_type_t type);
int pci_cep_source_set(struct pci *pci, cep_id_t src_cep_id);
int pci_cep_destination_set(struct pci *pci, cep_id_t dst_cep_id);
int pci_destination_set(struct pci *pci, address_t dst_address);
int pci_source_set(struct pci *pci, address_t src_address);
int pci_sequence_number_set(struct pci *pci, seq_num_t sequence_number);
int pci_qos_id_set(struct pci *pci, qos_id_t qos_id);
int pci_type_set(struct pci *pci, pdu_type_t	type);
int pci_flags_set(struct pci *pci,pdu_flags_t flags);
int pci_len_set(struct pci *pci, ssize_t len);
int pci_format(struct pci *pci, cep_id_t src_cep_id, cep_id_t dst_cep_id,
	       address_t src_address, address_t dst_address,
	       seq_num_t sequence_number, qos_id_t qos_id,
	       pdu_flags_t flags,
	       ssize_t length, pdu_type_t type);

/* FIXME: remove _get from the API name */
seq_num_t pci_sequence_number_get(const struct pci *pci);
pdu_type_t pci_type(const struct pci *pci);
address_t pci_source(const struct pci *pci);
address_t pci_destination(const struct pci *pci);
cep_id_t pci_cep_source(const struct pci *pci);
cep_id_t pci_cep_destination(const struct pci *pci);
qos_id_t pci_qos_id(const struct pci *pci);
pdu_flags_t pci_flags_get(const struct pci *pci);
ssize_t pci_length(const struct pci *pci);

/* For Control PDUs */
int		pci_control_ack_seq_num_set(struct pci *pci,
						    seq_num_t seq);
int		pci_control_new_rt_wind_edge_set(struct pci *pci,
							 seq_num_t    seq);
int		pci_control_my_rt_wind_edge_set(struct pci *pci,
							seq_num_t    seq);
int		pci_control_my_left_wind_edge_set(struct pci *pci,
							  seq_num_t    seq);
int		pci_control_last_seq_num_rcvd_set(struct pci *pci,
							  seq_num_t    seq);
int		pci_control_new_left_wind_edge_set(struct pci *pci,
							   seq_num_t seq);
seq_num_t	pci_control_ack_seq_num(const struct pci *pci);
seq_num_t	pci_control_new_rt_wind_edge(const struct pci *pci);
seq_num_t	pci_control_new_left_wind_edge(const struct pci *pci);
seq_num_t	pci_control_my_rt_wind_edge(const struct pci *pci);
seq_num_t	pci_control_my_left_wind_edge(const struct pci *pci);
seq_num_t	pci_control_last_seq_num_rcvd(const struct pci *pci);
u_int32_t	pci_control_sndr_rate(const struct pci *pci);
int		pci_control_sndr_rate_set(struct pci *pci,
						  u_int32_t rate);
u_int32_t	pci_control_time_frame(const struct pci *pci);
int		pci_control_time_frame_set(struct pci *pci,
						   u_int32_t frame);
/* XXX:Needed only for process_A_expiration */
int			pci_get(struct pci *pci);
int			pci_release(struct pci *pci); /* This should be called only after process_A_expiration */


#if 0
booli			pci_getset_test(void);
#endif
#endif

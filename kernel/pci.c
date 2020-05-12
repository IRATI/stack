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

#include <linux/export.h>
#include <linux/types.h>
#include <linux/skbuff.h>
#include <linux/version.h>

#define RINA_PREFIX "pci"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "du.h"

#define VERSION_SIZE 1
#define FLAGS_SIZE 1
#define TYPE_SIZE 1

#define VERSION 1

enum pci_field_index {
	PCI_BASE_VERSION = 0,
	PCI_BASE_DST_ADD,
	PCI_BASE_SRC_ADD,
	PCI_BASE_QOS_ID,
	PCI_BASE_DST_CEP,
	PCI_BASE_SRC_CEP,
	PCI_BASE_TYPE,
	PCI_BASE_FLAGS,
	PCI_BASE_LEN,
	/* pci_mgmt & pci_dt */
	PCI_DT_MGMT_SN,
	PCI_DT_MGMT_SIZE,
	/* ctrl pci sn */
	PCI_CTRL_SN,
	/* pci_fc */
	PCI_FC_NEW_RWE,
	PCI_FC_MY_LWE,
	PCI_FC_MY_RWE,
	PCI_FC_SNDR_RATE,
	PCI_FC_TIME_FRAME,
	PCI_FC_SIZE,
	/* pci_cc */
	PCI_CACK_LAST_CSN_RCVD,
	PCI_CACK_NEW_LWE,
	PCI_CACK_NEW_RWE,
	PCI_CACK_MY_LWE,
	PCI_CACK_MY_RWE,
	PCI_CACK_SNDR_RATE,
	PCI_CACK_TIME_FRAME,
	PCI_CACK_SIZE,
	/* pci_ack */
	PCI_ACK_ACKED_SN,
	PCI_ACK_SIZE,
	/* pci_ack_fc */
	PCI_ACK_FC_ACKED_SN,
	PCI_ACK_FC_LAST_CSN_RCVD,
	PCI_ACK_FC_NEW_LWE,
	PCI_ACK_FC_NEW_RWE,
	PCI_ACK_FC_MY_LWE,
	PCI_ACK_FC_MY_RWE,
	PCI_ACK_FC_SNDR_RATE,
	PCI_ACK_FC_TIME_FRAME,
	PCI_ACK_FC_SIZE,
	/* pci_rvous */
	PCI_RVOUS_LAST_CSN_RCVD,
	PCI_RVOUS_NEW_LWE,
	PCI_RVOUS_NEW_RWE,
	PCI_RVOUS_MY_LWE,
	PCI_RVOUS_MY_RWE,
	PCI_RVOUS_SNDR_RATE,
	PCI_RVOUS_TIME_FRAME,
	PCI_RVOUS_SIZE,
	/* number of fields */
	PCI_FIELD_INDEX_MAX,
};

/* struct pci_old {
 *	address_t   destination;
 *	address_t   source;
 *
 *	struct {
 *		qos_id_t qos_id;
 *		cep_id_t source;
 *		cep_id_t destination;
 *	} connection_id;
 *
 *	pdu_type_t  type;
 *	pdu_flags_t flags;
 *	seq_num_t   sequence_number;
 *	size_t      ttl;
 *
 *	struct {
 *		seq_num_t last_ctrl_seq_num_rcvd;
 *		seq_num_t ack_nack_seq_num;
 *		seq_num_t new_rt_wind_edge;
 *		seq_num_t new_lf_wind_edge;
 *		seq_num_t my_lf_wind_edge;
 *		seq_num_t my_rt_wind_edge;
 *		u_int32_t sndr_rate;
 *		u_int32_t time_frame;
 *	} control;
 *};
*/

ssize_t *pci_offset_table_create(struct dt_cons *dt_cons)
{
	ssize_t *pci_offsets;
	ssize_t offset = 0;
	ssize_t base_offset = 0;
	int i;

	pci_offsets = rkzalloc(PCI_FIELD_INDEX_MAX * sizeof(offset),
			       GFP_KERNEL);
	if (!pci_offsets) {
		LOG_ERR("Could not allocate memory for PCI offsets table");
		return NULL;
	}

	for (i = 0; i < PCI_FIELD_INDEX_MAX; i++) {
		pci_offsets[i] = offset;
		switch (i) {
		case PCI_BASE_VERSION:
			offset += VERSION_SIZE;
			break;
		case PCI_BASE_DST_ADD:
		case PCI_BASE_SRC_ADD:
			offset += dt_cons->address_length;
			break;
		case PCI_BASE_QOS_ID:
			offset += dt_cons->qos_id_length;
			break;
		case PCI_BASE_DST_CEP:
		case PCI_BASE_SRC_CEP:
			offset += dt_cons->cep_id_length;
			break;
		case PCI_BASE_TYPE:
			offset += TYPE_SIZE;
			break;
		case PCI_BASE_FLAGS:
			offset += FLAGS_SIZE;
			break;
		case PCI_BASE_LEN:
			offset += dt_cons->length_length;
			base_offset = offset;
			break;
		case PCI_DT_MGMT_SN:
		case PCI_FC_NEW_RWE:
		case PCI_FC_MY_LWE:
		case PCI_FC_MY_RWE:
		case PCI_CACK_NEW_LWE:
		case PCI_CACK_NEW_RWE:
		case PCI_CACK_MY_LWE:
		case PCI_CACK_MY_RWE:
		case PCI_ACK_FC_ACKED_SN:
		case PCI_ACK_ACKED_SN:
		case PCI_ACK_FC_NEW_LWE:
		case PCI_ACK_FC_NEW_RWE:
		case PCI_ACK_FC_MY_LWE:
		case PCI_ACK_FC_MY_RWE:
		case PCI_RVOUS_NEW_LWE:
		case PCI_RVOUS_NEW_RWE:
		case PCI_RVOUS_MY_LWE:
		case PCI_RVOUS_MY_RWE:
			offset += dt_cons->seq_num_length;
			break;
		case PCI_CTRL_SN:
			offset += dt_cons->ctrl_seq_num_length;
			base_offset += dt_cons->ctrl_seq_num_length;
			break;
		case PCI_CACK_LAST_CSN_RCVD:
		case PCI_ACK_FC_LAST_CSN_RCVD:
		case PCI_RVOUS_LAST_CSN_RCVD:
			offset += dt_cons->ctrl_seq_num_length;
			break;
		case PCI_FC_SNDR_RATE:
		case PCI_CACK_SNDR_RATE:
		case PCI_ACK_FC_SNDR_RATE:
		case PCI_RVOUS_SNDR_RATE:
			offset += dt_cons->rate_length;
			break;
		case PCI_FC_TIME_FRAME:
		case PCI_CACK_TIME_FRAME:
		case PCI_ACK_FC_TIME_FRAME:
		case PCI_RVOUS_TIME_FRAME:
			offset += dt_cons->frame_length;
			break;
		case PCI_DT_MGMT_SIZE:
		case PCI_FC_SIZE:
		case PCI_CACK_SIZE:
		case PCI_ACK_SIZE:
		case PCI_RVOUS_SIZE:
		case PCI_ACK_FC_SIZE:
			offset = base_offset;
			break;
		}
	}

	LOG_DBG("PCI Offsets Table Calculated:");
	LOG_DBG("pci_offsets[PCI_BASE_VERSION] = %zu", pci_offsets[PCI_BASE_VERSION]);
	LOG_DBG("pci_offsets[PCI_BASE_DST_ADD] = %zu", pci_offsets[PCI_BASE_DST_ADD]);
	LOG_DBG("pci_offsets[PCI_BASE_SRC_ADD] = %zu", pci_offsets[PCI_BASE_SRC_ADD]);
	LOG_DBG("pci_offsets[PCI_BASE_QOS_ID] = %zu", pci_offsets[PCI_BASE_QOS_ID]);
	LOG_DBG("pci_offsets[PCI_BASE_DST_CEP] = %zu", pci_offsets[PCI_BASE_DST_CEP]);
	LOG_DBG("pci_offsets[PCI_BASE_SRC_CEP] = %zu", pci_offsets[PCI_BASE_SRC_CEP]);
	LOG_DBG("pci_offsets[PCI_BASE_TYPE] = %zu", pci_offsets[PCI_BASE_TYPE]);
	LOG_DBG("pci_offsets[PCI_BASE_FLAGS] = %zu", pci_offsets[PCI_BASE_FLAGS]);
	LOG_DBG("pci_offsets[PCI_BASE_LEN] = %zu", pci_offsets[PCI_BASE_LEN]);
	LOG_DBG("pci_offsets[PCI_DT_MGMT_SN] = %zu", pci_offsets[PCI_DT_MGMT_SN]);
	LOG_DBG("pci_offsets[PCI_DT_MGMT_SIZE] = %zu", pci_offsets[PCI_DT_MGMT_SIZE]);
	LOG_DBG("pci_offsets[PCI_CTRL_SN] = %zu", pci_offsets[PCI_CTRL_SN]);
	LOG_DBG("pci_offsets[PCI_FC_NEW_RWE] = %zu", pci_offsets[PCI_FC_NEW_RWE]);
	LOG_DBG("pci_offsets[PCI_FC_MY_LWE] = %zu", pci_offsets[PCI_FC_MY_LWE]);
	LOG_DBG("pci_offsets[PCI_FC_MY_RWE] = %zu", pci_offsets[PCI_FC_MY_RWE]);
	LOG_DBG("pci_offsets[PCI_FC_SNDR_RATE] = %zu", pci_offsets[PCI_FC_SNDR_RATE]);
	LOG_DBG("pci_offsets[PCI_FC_TIME_FRAME] = %zu", pci_offsets[PCI_FC_TIME_FRAME]);
	LOG_DBG("pci_offsets[PCI_FC_SIZE] = %zu", pci_offsets[PCI_FC_SIZE]);
	LOG_DBG("pci_offsets[PCI_CACK_LAST_CSN_RCVD] = %zu", pci_offsets[PCI_CACK_LAST_CSN_RCVD]);
	LOG_DBG("pci_offsets[PCI_CACK_NEW_LWE] = %zu", pci_offsets[PCI_CACK_NEW_LWE]);
	LOG_DBG("pci_offsets[PCI_CACK_NEW_RWE] = %zu", pci_offsets[PCI_CACK_NEW_RWE]);
	LOG_DBG("pci_offsets[PCI_CACK_MY_LWE] = %zu", pci_offsets[PCI_CACK_MY_LWE]);
	LOG_DBG("pci_offsets[PCI_CACK_MY_RWE] = %zu", pci_offsets[PCI_CACK_MY_RWE]);
	LOG_DBG("pci_offsets[PCI_CACK_SNDR_RATE] = %zu", pci_offsets[PCI_CACK_SNDR_RATE]);
	LOG_DBG("pci_offsets[PCI_CACK_TIME_FRAME] = %zu", pci_offsets[PCI_CACK_TIME_FRAME]);
	LOG_DBG("pci_offsets[PCI_CACK_SIZE] = %zu", pci_offsets[PCI_CACK_SIZE]);
	LOG_DBG("pci_offsets[PCI_ACK_ACKED_SN] = %zu", pci_offsets[PCI_ACK_ACKED_SN]);
	LOG_DBG("pci_offsets[PCI_ACK_SIZE] = %zu", pci_offsets[PCI_ACK_SIZE]);
	LOG_DBG("pci_offsets[PCI_ACK_FC_ACKED_SN] = %zu", pci_offsets[PCI_ACK_FC_ACKED_SN]);
	LOG_DBG("pci_offsets[PCI_ACK_FC_LAST_CSN_RCVD] = %zu", pci_offsets[PCI_ACK_FC_LAST_CSN_RCVD]);
	LOG_DBG("pci_offsets[PCI_ACK_FC_NEW_LWE] = %zu", pci_offsets[PCI_ACK_FC_NEW_LWE]);
	LOG_DBG("pci_offsets[PCI_ACK_FC_NEW_RWE] = %zu", pci_offsets[PCI_ACK_FC_NEW_RWE]);
	LOG_DBG("pci_offsets[PCI_ACK_FC_MY_LWE] = %zu", pci_offsets[PCI_ACK_FC_MY_LWE]);
	LOG_DBG("pci_offsets[PCI_ACK_FC_MY_RWE] = %zu", pci_offsets[PCI_ACK_FC_MY_RWE]);
	LOG_DBG("pci_offsets[PCI_ACK_FC_SNDR_RATE] = %zu", pci_offsets[PCI_ACK_FC_SNDR_RATE]);
	LOG_DBG("pci_offsets[PCI_ACK_FC_TIME_FRAME] = %zu", pci_offsets[PCI_ACK_FC_TIME_FRAME]);
	LOG_DBG("pci_offsets[PCI_RVOUS_SIZE] = %zu", pci_offsets[PCI_RVOUS_SIZE]);
	LOG_DBG("pci_offsets[PCI_ACK_FC_SIZE] = %zu",pci_offsets[PCI_ACK_FC_SIZE]);
	return pci_offsets;
}

bool pci_is_ok(const struct pci *pci)
{
	if (pci && pci->h && pci->len > 0 && pdu_type_is_ok(pci_type(pci)))
		return true;
	return false;
}
EXPORT_SYMBOL(pci_is_ok);

static struct efcp_config *__pci_efcp_config_get(const struct pci *pci)
{
	struct du *pdu;

	pdu = container_of(pci, struct du, pci);
	return pdu->cfg;
}

#define PCI_GETTER(pci, pci_index, dt_cons_field, type)			\
	{struct efcp_config *cfg;					\
	cfg = __pci_efcp_config_get(pci);				\
	switch (cfg->dt_cons->dt_cons_field) {				\
	case (1):							\
		return (type) *((__u8*)					\
		(pci->h + cfg->pci_offset_table[pci_index]));		\
	case (2):							\
		return (type) *((__u16*)				\
		(pci->h + cfg->pci_offset_table[pci_index]));		\
	case (4):							\
		return (type) *((__u32*)				\
		(pci->h + cfg->pci_offset_table[pci_index]));		\
	}								\
	return -1;}							\

#define PCI_GETTER_NO_DTC(pci, pci_index, size, type)			\
	{struct efcp_config *cfg;					\
	cfg = __pci_efcp_config_get(pci);				\
	switch (size) {							\
	case (1):							\
		return (type) *((__u8*)					\
		(pci->h + cfg->pci_offset_table[pci_index]));		\
	case (2):							\
		return (type) *((__u16*)				\
		(pci->h + cfg->pci_offset_table[pci_index]));		\
	case (4):							\
		return (type) *((__u32*)				\
		(pci->h + cfg->pci_offset_table[pci_index]));		\
	}								\
	return -1;}							\

#define PCI_SETTER(pci, pci_index, dt_cons_field, val)			\
	{struct efcp_config *cfg;					\
	cfg = __pci_efcp_config_get(pci);				\
	switch (cfg->dt_cons->dt_cons_field) {				\
	case (1):							\
		*((__u8 *)						\
		(pci->h + cfg->pci_offset_table[pci_index])) = val;\
		break;							\
	case (2):							\
		*((__u16 *)						\
		(pci->h + cfg->pci_offset_table[pci_index])) = val;	\
		break;							\
	case (4):							\
		*((__u32 *)						\
		(pci->h + cfg->pci_offset_table[pci_index])) = val;	\
		break;							\
	}								\
	return 0;}

#define PCI_SETTER_NO_DTC(pci, pci_index, size, val)			\
	{struct efcp_config *cfg;					\
	cfg = __pci_efcp_config_get(pci);				\
	switch (size) {							\
	case (1):							\
		*((__u8 *)						\
		(pci->h + cfg->pci_offset_table[pci_index])) = val;	\
		break;							\
	case (2):							\
		*((__u16 *)						\
		(pci->h + cfg->pci_offset_table[pci_index])) = val;	\
		break;							\
	case (4):							\
		*((__u32 *)						\
		(pci->h + cfg->pci_offset_table[pci_index])) = val;	\
		break;							\
	}								\
	return 0;}

/* Base getters */
cep_id_t pci_version(const struct pci *pci)
{ PCI_GETTER_NO_DTC(pci, PCI_BASE_VERSION, VERSION_SIZE, version_t); }
EXPORT_SYMBOL(pci_version);

cep_id_t pci_cep_source(const struct pci *pci)
{ PCI_GETTER(pci, PCI_BASE_SRC_CEP, cep_id_length, cep_id_t); }
EXPORT_SYMBOL(pci_cep_source);

cep_id_t pci_cep_destination(const struct pci *pci)
{ PCI_GETTER(pci, PCI_BASE_DST_CEP, cep_id_length, cep_id_t); }
EXPORT_SYMBOL(pci_cep_destination);

address_t pci_destination(const struct pci *pci)
{ PCI_GETTER(pci, PCI_BASE_DST_ADD, address_length, address_t); }
EXPORT_SYMBOL(pci_destination);

address_t pci_source(const struct pci *pci)
{ PCI_GETTER(pci, PCI_BASE_SRC_ADD, address_length, address_t); }
EXPORT_SYMBOL(pci_source);

qos_id_t pci_qos_id(const struct pci *pci)
{ PCI_GETTER(pci, PCI_BASE_QOS_ID, qos_id_length, qos_id_t); }
EXPORT_SYMBOL(pci_qos_id);

pdu_type_t pci_type(const struct pci *pci)
{ PCI_GETTER_NO_DTC(pci, PCI_BASE_TYPE, TYPE_SIZE, pdu_type_t); }
EXPORT_SYMBOL(pci_type);

pdu_flags_t pci_flags_get(const struct pci *pci)
{ PCI_GETTER_NO_DTC(pci, PCI_BASE_FLAGS, FLAGS_SIZE, pdu_flags_t); }
EXPORT_SYMBOL(pci_flags_get);

ssize_t pci_length(const struct pci *pci)
{ PCI_GETTER(pci, PCI_BASE_LEN, length_length, ssize_t); }
EXPORT_SYMBOL(pci_length);

/* Base setters */
int pci_version_set(struct pci *pci, version_t version)
{ PCI_SETTER_NO_DTC(pci, PCI_BASE_VERSION, VERSION_SIZE, version); }
EXPORT_SYMBOL(pci_version_set);

int pci_sequence_number_set(struct pci *pci, seq_num_t sn)
{ PCI_SETTER(pci, PCI_DT_MGMT_SN, seq_num_length, sn); }
EXPORT_SYMBOL(pci_sequence_number_set);

int pci_cep_source_set(struct pci *pci, cep_id_t src_cep_id)
{ PCI_SETTER(pci, PCI_BASE_SRC_CEP, cep_id_length, src_cep_id); }
EXPORT_SYMBOL(pci_cep_source_set);

int pci_cep_destination_set(struct pci *pci, cep_id_t dst_cep_id)
{ PCI_SETTER(pci, PCI_BASE_DST_CEP, cep_id_length, dst_cep_id); }
EXPORT_SYMBOL(pci_cep_destination_set);

int pci_destination_set(struct pci *pci, address_t dst_address)
{ PCI_SETTER(pci, PCI_BASE_DST_ADD, address_length, dst_address); }
EXPORT_SYMBOL(pci_destination_set);

int pci_source_set(struct pci *pci, address_t src_address)
{ PCI_SETTER(pci, PCI_BASE_SRC_ADD, address_length, src_address); }
EXPORT_SYMBOL(pci_source_set);

int pci_qos_id_set(struct pci *pci, qos_id_t qos_id)
{ PCI_SETTER(pci, PCI_BASE_QOS_ID, qos_id_length, qos_id); }
EXPORT_SYMBOL(pci_qos_id_set);

int pci_type_set(struct pci *pci, pdu_type_t type)
{ PCI_SETTER_NO_DTC(pci, PCI_BASE_TYPE, TYPE_SIZE, type); }
EXPORT_SYMBOL(pci_type_set);

int pci_flags_set(struct pci *pci, pdu_flags_t flags)
{ PCI_SETTER_NO_DTC(pci, PCI_BASE_FLAGS, FLAGS_SIZE, flags); }
EXPORT_SYMBOL(pci_flags_set);

int pci_len_set(struct pci *pci, ssize_t len)
{ PCI_SETTER(pci, PCI_BASE_LEN, length_length, len); }
EXPORT_SYMBOL(pci_len_set);

int pci_format(struct pci *pci,
	       cep_id_t src_cep_id,
	       cep_id_t dst_cep_id,
	       address_t src_address,
	       address_t dst_address,
	       seq_num_t sequence_number,
	       qos_id_t  qos_id,
	       pdu_flags_t flags,
	       ssize_t   length,
	       pdu_type_t type)
{
	if (pci_version_set(pci, VERSION)                 ||
	    pci_type_set(pci, type)                       ||
	    pci_cep_destination_set(pci, dst_cep_id)      ||
	    pci_cep_source_set(pci, src_cep_id)           ||
	    pci_destination_set(pci, dst_address)         ||
	    pci_source_set(pci, src_address)              ||
	    pci_sequence_number_set(pci, sequence_number) ||
	    pci_qos_id_set(pci, qos_id)			  ||
	    pci_flags_set(pci, flags)			  ||
	    pci_len_set(pci, length)) {
		return -1;
	}
	return 0;
}
EXPORT_SYMBOL(pci_format);

/*static int check_pdu_type(struct pci *pci, int ret_val, int n_types, ...)
{
	va_list args;
	int i;

	va_start(args, n_types);
	for (i = 0; i < n_types; i++) {
		if (va_arg(args, pdu_type_t) == pci_type(pci)) {
			va_end(args);
			return ret_val;
		}
	}
	va_end(args);
	return -ret_val;
}*/

ssize_t pci_calculate_size(struct efcp_config *cfg, pdu_type_t type)
{
	switch (type) {
		case PDU_TYPE_DT:
		case PDU_TYPE_MGMT:
			return cfg->pci_offset_table[PCI_DT_MGMT_SIZE];
		case PDU_TYPE_FC:
			return cfg->pci_offset_table[PCI_FC_SIZE];
		case PDU_TYPE_ACK:
			return cfg->pci_offset_table[PCI_ACK_SIZE];
		case PDU_TYPE_ACK_AND_FC:
			return cfg->pci_offset_table[PCI_ACK_FC_SIZE];
		case PDU_TYPE_CACK:
			return cfg->pci_offset_table[PCI_CACK_SIZE];
		case PDU_TYPE_RENDEZVOUS:
			return cfg->pci_offset_table[PCI_RVOUS_SIZE];
		default:
			return -1;
	}
}
EXPORT_SYMBOL(pci_calculate_size);

/* Custom getters */
seq_num_t pci_sequence_number_get(const struct pci *pci)
{
	switch (pci_type(pci)) {
	case PDU_TYPE_DT:
	case PDU_TYPE_MGMT:
		PCI_GETTER(pci, PCI_DT_MGMT_SN, seq_num_length, seq_num_t);
	/* FIXME: we need to make sure the type exists, maybe redefine
	 * pdu_type_t as union
	 */
	default:
		PCI_GETTER(pci, PCI_CTRL_SN, ctrl_seq_num_length, seq_num_t);
	}
}
EXPORT_SYMBOL(pci_sequence_number_get);

seq_num_t pci_control_ack_seq_num(const struct pci *pci)
{
	switch (pci_type(pci)) {
	case PDU_TYPE_ACK:
		PCI_GETTER(pci, PCI_ACK_ACKED_SN, seq_num_length, seq_num_t);
	case PDU_TYPE_ACK_AND_FC:
		PCI_GETTER(pci, PCI_ACK_FC_ACKED_SN, seq_num_length, seq_num_t);
	default:
		return -1;
	}
}
EXPORT_SYMBOL(pci_control_ack_seq_num);

seq_num_t pci_control_new_rt_wind_edge(const struct pci *pci)
{
	switch (pci_type(pci)) {
	case PDU_TYPE_FC:
		PCI_GETTER(pci, PCI_FC_NEW_RWE, seq_num_length, seq_num_t);
	case PDU_TYPE_RENDEZVOUS:
	case PDU_TYPE_CACK:
		PCI_GETTER(pci, PCI_CACK_NEW_RWE, seq_num_length, seq_num_t);
	case PDU_TYPE_ACK_AND_FC:
		PCI_GETTER(pci, PCI_ACK_FC_NEW_RWE, seq_num_length, seq_num_t);
	default:
		return -1;
	}
}
EXPORT_SYMBOL(pci_control_new_rt_wind_edge);

seq_num_t pci_control_new_left_wind_edge(const struct pci *pci)
{
	switch (pci_type(pci)) {
	case PDU_TYPE_RENDEZVOUS:
	case PDU_TYPE_CACK:
		PCI_GETTER(pci, PCI_CACK_NEW_LWE, seq_num_length, seq_num_t);
	case PDU_TYPE_ACK_AND_FC:
		PCI_GETTER(pci, PCI_ACK_FC_NEW_LWE, seq_num_length, seq_num_t);
	default:
		return -1;
	}
}
EXPORT_SYMBOL(pci_control_new_left_wind_edge);

seq_num_t pci_control_my_rt_wind_edge(const struct pci *pci)
{
	switch (pci_type(pci)) {
	case PDU_TYPE_FC:
		PCI_GETTER(pci, PCI_FC_MY_RWE, seq_num_length, seq_num_t);
	case PDU_TYPE_RENDEZVOUS:
	case PDU_TYPE_CACK:
		PCI_GETTER(pci, PCI_CACK_MY_RWE, seq_num_length, seq_num_t);
	case PDU_TYPE_ACK_AND_FC:
		PCI_GETTER(pci, PCI_ACK_FC_MY_RWE, seq_num_length, seq_num_t);
	default:
		return -1;
	}
}
EXPORT_SYMBOL(pci_control_my_rt_wind_edge);

seq_num_t pci_control_my_left_wind_edge(const struct pci *pci)
{
	switch (pci_type(pci)) {
	case PDU_TYPE_FC:
		PCI_GETTER(pci, PCI_FC_MY_LWE, seq_num_length, seq_num_t);
	case PDU_TYPE_RENDEZVOUS:
	case PDU_TYPE_CACK:
		PCI_GETTER(pci, PCI_CACK_MY_LWE, seq_num_length, seq_num_t);
	case PDU_TYPE_ACK_AND_FC:
		PCI_GETTER(pci, PCI_ACK_FC_MY_LWE, seq_num_length, seq_num_t);
	default:
		return -1;
	}
}
EXPORT_SYMBOL(pci_control_my_left_wind_edge);

seq_num_t pci_control_last_seq_num_rcvd(const struct pci *pci)
{
	switch (pci_type(pci)) {
	case PDU_TYPE_RENDEZVOUS:
	case PDU_TYPE_CACK:
		PCI_GETTER(pci, PCI_CACK_LAST_CSN_RCVD, seq_num_length, seq_num_t);
	case PDU_TYPE_ACK_AND_FC:
		PCI_GETTER(pci, PCI_ACK_FC_LAST_CSN_RCVD, seq_num_length, seq_num_t);
	default:
		return -1;
	}
}
EXPORT_SYMBOL(pci_control_last_seq_num_rcvd);

u_int32_t pci_control_sndr_rate(const struct pci *pci)
{
	switch (pci_type(pci)) {
	case PDU_TYPE_FC:
		PCI_GETTER(pci, PCI_FC_SNDR_RATE, rate_length, u_int32_t);
	case PDU_TYPE_RENDEZVOUS:
	case PDU_TYPE_CACK:
		PCI_GETTER(pci, PCI_CACK_SNDR_RATE, rate_length, u_int32_t);
	case PDU_TYPE_ACK_AND_FC:
		PCI_GETTER(pci, PCI_ACK_FC_SNDR_RATE, rate_length, u_int32_t);
	default:
		return 0;
	}
}
EXPORT_SYMBOL(pci_control_sndr_rate);

u_int32_t pci_control_time_frame(const struct pci *pci)
{
	switch (pci_type(pci)) {
	case PDU_TYPE_FC:
		PCI_GETTER(pci, PCI_FC_TIME_FRAME, frame_length, u_int32_t);
	case PDU_TYPE_CACK:
		PCI_GETTER(pci, PCI_CACK_TIME_FRAME, frame_length, u_int32_t);
	case PDU_TYPE_ACK_AND_FC:
		PCI_GETTER(pci, PCI_ACK_FC_TIME_FRAME, frame_length, u_int32_t);
	default:
		return 0;
	}
}
EXPORT_SYMBOL(pci_control_time_frame);

/* Custom setters */
int pci_control_ack_seq_num_set(struct pci *pci, seq_num_t seq)
{
	switch (pci_type(pci)) {
	case PDU_TYPE_ACK:
		PCI_SETTER(pci, PCI_ACK_ACKED_SN, seq_num_length, seq);
	case PDU_TYPE_ACK_AND_FC:
		PCI_SETTER(pci, PCI_ACK_FC_ACKED_SN, seq_num_length, seq);
	default:
		return -1;
	}
}
EXPORT_SYMBOL(pci_control_ack_seq_num_set);

int pci_control_new_rt_wind_edge_set(struct pci *pci, seq_num_t seq)
{
	switch (pci_type(pci)) {
	case PDU_TYPE_FC:
		PCI_SETTER(pci, PCI_FC_NEW_RWE, seq_num_length, seq);
	case PDU_TYPE_RENDEZVOUS:
	case PDU_TYPE_CACK:
		PCI_SETTER(pci, PCI_CACK_NEW_RWE, seq_num_length, seq);
	case PDU_TYPE_ACK_AND_FC:
		PCI_SETTER(pci, PCI_ACK_FC_NEW_RWE, seq_num_length, seq);
	default:
		return -1;
	}
}
EXPORT_SYMBOL(pci_control_new_rt_wind_edge_set);

int pci_control_new_left_wind_edge_set(struct pci *pci, seq_num_t seq)
{
	switch (pci_type(pci)) {
	case PDU_TYPE_RENDEZVOUS:
	case PDU_TYPE_CACK:
		PCI_SETTER(pci, PCI_CACK_NEW_LWE, seq_num_length, seq);
	case PDU_TYPE_ACK_AND_FC:
		PCI_SETTER(pci, PCI_ACK_FC_NEW_LWE, seq_num_length, seq);
	default:
		return -1;
	}
}
EXPORT_SYMBOL(pci_control_new_left_wind_edge_set);

int pci_control_my_rt_wind_edge_set(struct pci *pci, seq_num_t seq)
{
	switch (pci_type(pci)) {
	case PDU_TYPE_FC:
		PCI_SETTER(pci, PCI_FC_MY_RWE, seq_num_length, seq);
	case PDU_TYPE_RENDEZVOUS:
	case PDU_TYPE_CACK:
		PCI_SETTER(pci, PCI_CACK_MY_RWE, seq_num_length, seq);
	case PDU_TYPE_ACK_AND_FC:
		PCI_SETTER(pci, PCI_ACK_FC_MY_RWE, seq_num_length, seq);
	default:
		return -1;
	}
}
EXPORT_SYMBOL(pci_control_my_rt_wind_edge_set);

int pci_control_my_left_wind_edge_set(struct pci *pci, seq_num_t seq)
{
	switch (pci_type(pci)) {
	case PDU_TYPE_FC:
		PCI_SETTER(pci, PCI_FC_MY_LWE, seq_num_length, seq);
	case PDU_TYPE_RENDEZVOUS:
	case PDU_TYPE_CACK:
		PCI_SETTER(pci, PCI_CACK_MY_LWE, seq_num_length, seq);
	case PDU_TYPE_ACK_AND_FC:
		PCI_SETTER(pci, PCI_ACK_FC_MY_LWE, seq_num_length, seq);
	default:
		return -1;
	}
}
EXPORT_SYMBOL(pci_control_my_left_wind_edge_set);

int pci_control_last_seq_num_rcvd_set(struct pci *pci, seq_num_t seq)
{
	switch (pci_type(pci)) {
	case PDU_TYPE_RENDEZVOUS:
	case PDU_TYPE_CACK:
		PCI_SETTER(pci, PCI_CACK_LAST_CSN_RCVD, seq_num_length, seq);
	case PDU_TYPE_ACK_AND_FC:
		PCI_SETTER(pci, PCI_ACK_FC_LAST_CSN_RCVD, seq_num_length, seq);
	default:
		return -1;
	}
}
EXPORT_SYMBOL(pci_control_last_seq_num_rcvd_set);

int pci_control_sndr_rate_set(struct pci *pci, u_int32_t rate)
{
	switch (pci_type(pci)) {
	case PDU_TYPE_FC:
		PCI_SETTER(pci, PCI_FC_SNDR_RATE, rate_length, rate);
	case PDU_TYPE_RENDEZVOUS:
	case PDU_TYPE_CACK:
		PCI_SETTER(pci, PCI_CACK_SNDR_RATE, rate_length, rate);
	case PDU_TYPE_ACK_AND_FC:
		PCI_SETTER(pci, PCI_ACK_FC_SNDR_RATE, rate_length, rate);
	default:
		return -1;
	}
}
EXPORT_SYMBOL(pci_control_sndr_rate_set);

int pci_control_time_frame_set(struct pci *pci, u_int32_t frame)
{
	switch (pci_type(pci)) {
	case PDU_TYPE_FC:
		PCI_SETTER(pci, PCI_FC_TIME_FRAME, frame_length, frame);
	case PDU_TYPE_CACK:
		PCI_SETTER(pci, PCI_CACK_TIME_FRAME, frame_length, frame);
	case PDU_TYPE_ACK_AND_FC:
		PCI_SETTER(pci, PCI_ACK_FC_TIME_FRAME, frame_length, frame);
	default:
		return -1;
	}
}
EXPORT_SYMBOL(pci_control_time_frame_set);

/* Needed only for process_A_expiration */
int pci_get(struct pci *pci)
{
	struct du *du;
	ASSERT(pci_is_ok(pci));

	du = container_of(pci, struct du, pci);
	if (!du)
		return -1;

	skb_get(du->skb);
	return 0;
}
EXPORT_SYMBOL(pci_get);

int pci_release(struct pci *pci)
{
	struct du *du;
	ASSERT(pci_is_ok(pci));

	du = container_of(pci, struct du, pci);
	if (!du)
		return -1;

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,13,0)
	if (unlikely(atomic_read(&du->skb->users) == 1))
#else
	if (unlikely(atomic_read(&du->skb->users.refs) == 1))
#endif
		return du_destroy(du);

	kfree_skb(du->skb);
	return 0;
}
EXPORT_SYMBOL(pci_release);

#if 0

#include "ipcp-utils.h"
#include "sdu.h"
#include "pdu.h"

bool pci_getset_test(void)
{
	struct du *du;
	struct efcp_config *cfg;

	cep_id_t src_cep_id = 1;
	cep_id_t dst_cep_id = 2;
	address_t src_address = 22;
	address_t dst_address = 23;
	seq_num_t sequence_number = 2533;
	qos_id_t  qos_id = 1;
	pdu_type_t type = PDU_TYPE_MGMT;

	cfg = efcp_config_create();
	cfg->dt_cons->address_length = 2;
	cfg->dt_cons->cep_id_length = 2;
	cfg->dt_cons->length_length = 2;
	cfg->dt_cons->port_id_length = 2;
	cfg->dt_cons->qos_id_length = 2;
	cfg->dt_cons->seq_num_length = 4;
	cfg->dt_cons->ctrl_seq_num_length = 4;
	cfg->dt_cons->rate_length = 4;
	cfg->dt_cons->frame_length = 4;

	cfg->pci_offset_table = pci_offset_table_create(cfg->dt_cons);
	if (!cfg->pci_offset_table) {
		LOG_ERR("Could not create PCI offsets");
		efcp_config_destroy(cfg);
		return false;
	}

	du = du_create(2);
	if (!is_du_ok(du)) {
		LOG_ERR("Could not create SDU");
		rkfree(cfg->pci_offset_table);
		efcp_config_destroy(cfg);
		du_destroy(du);
		return true;
	}
	du->cfg = cfg;

	du_encap(du, type);
	if (!du->pci) {
		LOG_ERR("Could not retrieve pci");
		goto fail;
	}

	if (pci_format(du->pci,
		       src_cep_id,
		       dst_cep_id,
		       src_address,
		       dst_address,
		       sequence_number,
		       qos_id,
		       type)) {
		LOG_ERR("Could not format PCI");
		goto fail;
	}
	if (pci_type(du->pci) != type                           ||
	    pci_destination(du->pci) != dst_address             ||
	    pci_source(du->pci) != src_address                  ||
	    pci_cep_source(du->pci) != src_cep_id               ||
	    pci_cep_destination(du->pci) != dst_cep_id          ||
	    pci_sequence_number_get(du->pci) != sequence_number ||
	    pci_qos_id(du->pci) != qos_id) {
		LOG_ERR("Somethig after format does not match");
		goto fail;
	}

	pci_type_set(du->pci, type);
	pci_destination_set(du->pci, dst_address);
	pci_source_set(du->pci, src_address);
	pci_cep_source_set(du->pci, src_cep_id);
	pci_cep_destination_set(du->pci, dst_cep_id);
	pci_qos_id_set(du->pci, qos_id);
	pci_sequence_number_set(du->pci, sequence_number);

	if (pci_type(du->pci) != type) {
		LOG_ERR("type id does not match");
		goto fail;
	}
	if (pci_destination(du->pci) != dst_address) {
		LOG_ERR("destination address does not match");
		goto fail;
	}
	if (pci_source(du->pci) != src_address) {
		LOG_ERR("source address does not match");
		goto fail;
	}
	if (pci_cep_source(du->pci) != src_cep_id) {
		LOG_ERR("source cep id does not match");
		goto fail;
	}
	if (pci_cep_destination(du->pci) != dst_cep_id) {
		LOG_ERR("destination cep id does not match");
		goto fail;
	}
	if (pci_sequence_number_get(du->pci) != sequence_number) {
		LOG_ERR("seq num does not match");
		goto fail;
	}
	if (pci_qos_id(du->pci) != qos_id) {
		LOG_ERR("qos id does not match");
		goto fail;
	}

	rkfree(cfg->pci_offset_table);
	efcp_config_destroy(cfg);
	du_destroy(du);
	return false;
fail:
	rkfree(cfg->pci_offset_table);
	efcp_config_destroy(cfg);
	du_destroy(du);
	return true;

}
EXPORT_SYMBOL(pci_getset_test);
#endif

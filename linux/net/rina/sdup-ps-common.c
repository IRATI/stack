/*
 * Common parts of the common policy set for SDUP.
 *
 *    Ondrej Lichtner <ilichtner@fit.vutbr.cz>
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
#include <linux/module.h>
#include <linux/string.h>
#include <linux/hashtable.h>
#include <linux/crc32.h>
#include <linux/crypto.h>
#include <linux/scatterlist.h>

#define RINA_PREFIX "sdup-ps-common"

#include "logs.h"
#include "rds/rmem.h"
#include "sdup-ps.h"
#include "sdup-ps-common.h"
#include "debug.h"

static bool pdu_ser_data_and_length(struct pdu_ser * pdu,
				    unsigned char ** data,
				    ssize_t *        len)
{
	struct buffer * buf;

	buf = pdu_ser_buffer(pdu);
	if (!buffer_is_ok(buf))
		return -1;

	ASSERT(data);
	ASSERT(len);

	*data = (unsigned char *) buffer_data_rw(buf);
	if (!*data) {
		LOG_ERR("Cannot get data from serialised PDU");
		return -1;
	}

	*len = buffer_length(buf);

	return 0;
}

int common_sdup_add_padding_policy(struct sdup_ps * ps,
				   struct pdu_ser * pdu,
				   struct sdup_port_conf * port_conf)
{
	struct buffer * buf;
	ssize_t		buffer_size;
	ssize_t		padded_size;
	ssize_t		blk_size;
	char *		data;
	int i;

	if (!ps || !pdu || !port_conf){
		LOG_ERR("Encryption arguments not initialized!");
		return -1;
	}

	/* encryption and therefore padding is disabled */
	if (port_conf->blkcipher == NULL ||
	    !port_conf->enable_encryption)
		return 0;

	LOG_DBG("PADDING!");

	buf = pdu_ser_buffer(pdu);
	blk_size = crypto_blkcipher_blocksize(port_conf->blkcipher);
	buffer_size = buffer_length(buf);
	padded_size = (buffer_size/blk_size + 1) * blk_size;

	if (pdu_ser_tail_grow_gfp(pdu, padded_size - buffer_size)){
		LOG_ERR("Failed to grow ser PDU");
		return -1;
	}

	/* PADDING */
	data = buffer_data_rw(buf);
	for (i=padded_size-1; i>=buffer_size; i--){
		data[i] = padded_size - buffer_size;
	}
	return 0;
}

int common_sdup_remove_padding_policy(struct sdup_ps * ps,
				      struct pdu_ser * pdu,
				      struct sdup_port_conf * port_conf)
{
	struct buffer *	buf;
	const char *	data;
	ssize_t		len;
	ssize_t		pad_len;
	int		i;

	if (!ps || !pdu || !port_conf){
		LOG_ERR("Encryption arguments not initialized!");
		return -1;
	}

	/* decryption and therefore padding is disabled */
	if (port_conf->blkcipher == NULL ||
	    !port_conf->enable_decryption)
		return 0;

	LOG_DBG("UNPADDING!");
	buf = pdu_ser_buffer(pdu);
	data = buffer_data_ro(buf);
	len = buffer_length(buf);
	pad_len = data[len-1];

	//check padding
	for (i=len-1; i >= len-pad_len; --i){
		if (data[i] != pad_len){
			LOG_ERR("Padding check failed!");
			return -1;
		}
	}

	//remove padding
	if (pdu_ser_tail_shrink_gfp(pdu, pad_len)){
		LOG_ERR("Failed to shrink serialized PDU");
		return -1;
	}
	return 0;
}

int common_sdup_encrypt_policy(struct sdup_ps * ps,
			       struct pdu_ser * pdu,
			       struct sdup_port_conf * port_conf)
{
	struct blkcipher_desc	desc;
	struct scatterlist	sg_src;
	struct scatterlist	sg_dst;
	struct buffer *		buf;
	ssize_t			buffer_size;
	void *			data;

	if (!ps || !pdu || !port_conf){
		LOG_ERR("Encryption arguments not initialized!");
		return -1;
	}

	/* encryption is disabled */
	if (port_conf->blkcipher == NULL ||
	    !port_conf->enable_encryption)
		return 0;

	LOG_DBG("ENCRYPT!");

	desc.flags = 0;
	desc.tfm = port_conf->blkcipher;
	buf = pdu_ser_buffer(pdu);
	buffer_size = buffer_length(buf);
	data = buffer_data_rw(buf);

	sg_init_one(&sg_src, data, buffer_size);
	sg_init_one(&sg_dst, data, buffer_size);

	if (crypto_blkcipher_encrypt(&desc, &sg_dst, &sg_src, buffer_size)) {
		LOG_ERR("Encryption failed!");
		return -1;
	}
	return 0;
}

int common_sdup_decrypt_policy(struct sdup_ps * ps,
			       struct pdu_ser * pdu,
			       struct sdup_port_conf * port_conf)
{
	struct blkcipher_desc	desc;
	struct scatterlist	sg_src;
	struct scatterlist	sg_dst;
	struct buffer *		buf;
	ssize_t			buffer_size;
	void *			data;

	if (!ps || !pdu || !port_conf){
		LOG_ERR("Failed decryption");
		return -1;
	}

	/* decryption is disabled */
	if (port_conf->blkcipher == NULL ||
	    !port_conf->enable_decryption)
		return 0;

	LOG_DBG("DECRYPT!");

	desc.flags = 0;
	desc.tfm = port_conf->blkcipher;
	buf = pdu_ser_buffer(pdu);
	buffer_size = buffer_length(buf);
	data = buffer_data_rw(buf);

	sg_init_one(&sg_src, data, buffer_size);
	sg_init_one(&sg_dst, data, buffer_size);

	if (crypto_blkcipher_decrypt(&desc, &sg_dst, &sg_src, buffer_size)) {
		LOG_ERR("Decryption failed!");
		return -1;
	}
	return 0;
}

int common_sdup_set_lifetime_limit_policy(struct sdup_ps * ps,
					  struct pdu_ser * pdu,
					  struct sdup_port_conf * port_conf,
					  size_t pci_ttl)
{
	unsigned char * data;
	size_t          len;
	size_t		ttl;

	if (!ps || !pdu || !port_conf){
		LOG_ERR("Failed set_lifetime_limit");
		return -1;
	}

	ttl = 0;

	if (pci_ttl > 0)
		ttl = pci_ttl;
	else if (port_conf->initial_ttl_value > 0){
		ttl = port_conf->initial_ttl_value;
	}

	if (port_conf->initial_ttl_value > 0){
		if (pdu_ser_head_grow_gfp(GFP_ATOMIC, pdu, sizeof(ttl))) {
			LOG_ERR("Failed to grow ser PDU");
			return -1;
		}

		if (!pdu_ser_is_ok(pdu))
			return -1;

		if (pdu_ser_data_and_length(pdu, &data, &len))
			return -1;

		memcpy(data, &ttl, sizeof(ttl));
		LOG_DBG("SETTTL! %d", (int)ttl);
	}
	return 0;
}

int common_sdup_get_lifetime_limit_policy(struct sdup_ps * ps,
					  struct pdu_ser * pdu,
					  struct sdup_port_conf * port_conf,
					  size_t * ttl)
{
	unsigned char * data;
	size_t          len;

	if (!ps || !pdu || !port_conf || !ttl){
		LOG_ERR("Failed get_lifetime_limit");
		return -1;
	}

	if (port_conf->initial_ttl_value > 0){
		if (pdu_ser_data_and_length(pdu, &data, &len))
			return -1;

		memcpy(ttl, data, sizeof(*ttl));

		if (pdu_ser_head_shrink_gfp(GFP_ATOMIC , pdu, sizeof(*ttl))) {
			LOG_ERR("Failed to shrink ser PDU");
			return -1;
		}
		LOG_DBG("GET_TTL! %d", (int)*ttl);
	}
	return 0;
}

int common_sdup_dec_check_lifetime_limit_policy(struct sdup_ps * ps,
						struct pdu * pdu,
						struct sdup_port_conf * port_conf)
{
	struct pci * pci;
	size_t ttl;

	if (!ps || !pdu || !port_conf){
		LOG_ERR("Failed dec_check_lifetime_limit");
		return -1;
	}

	if (port_conf->initial_ttl_value > 0){
		pci = pdu_pci_get_rw(pdu);
		ttl = pci_ttl(pci);

		ttl = ttl-1;
		if (ttl == 0){
			LOG_DBG("TTL dropped to 0, dropping pdu.");
			return -1;
		}
		pci_ttl_set(pci, ttl);
		LOG_DBG("DEC_CHECK_TTL! new ttl: %d", (int)ttl);
	}
	return 0;
}

static bool pdu_ser_crc32(struct pdu_ser * pdu,
			  u32 *            crc)
{
	ssize_t         len;
	unsigned char * data;

	ASSERT(pdu_ser_is_ok(pdu));
	ASSERT(crc);

	data = 0;
	len  = 0;

	if (pdu_ser_data_and_length(pdu, &data, &len))
		return -1;

	*crc = crc32_le(0, data, len - sizeof(*crc));
	LOG_DBG("CRC32(%p, %zd) = %X", data, len, *crc);

	return 0;
}

int common_sdup_add_error_check_policy(struct sdup_ps * ps,
				       struct pdu_ser * pdu,
				       struct sdup_port_conf * port_conf)
{
	u32             crc;
	unsigned char * data;
	ssize_t         len;

	if (!ps || !pdu || !port_conf){
		LOG_ERR("Error check arguments not initialized!");
		return -1;
	}

	if (port_conf->dup_conf == NULL)
		return 0;

	if (port_conf->dup_conf->error_check_policy != NULL){
		/* TODO Assuming CRC32 */
		if (pdu_ser_tail_grow_gfp(pdu, sizeof(u32))) {
			LOG_ERR("Failed to grow ser PDU");
			return -1;
		}

		crc  = 0;
		data = 0;
		len  = 0;

		if (!pdu_ser_is_ok(pdu))
			return -1;

		if (pdu_ser_crc32(pdu, &crc))
			return -1;

		if (pdu_ser_data_and_length(pdu, &data, &len))
			return -1;

		memcpy(data+len-sizeof(crc), &crc, sizeof(crc));
		LOG_DBG("CRC!");
	}
	return 0;
}

int common_sdup_check_error_check_policy(struct sdup_ps * ps,
					 struct pdu_ser * pdu,
					 struct sdup_port_conf * port_conf)
{
	u32             crc;
	unsigned char * data;
	ssize_t         len;

	if (!ps || !pdu || !port_conf){
		LOG_ERR("Error check arguments not initialized!");
		return -1;
	}

	if (!pdu_ser_is_ok(pdu))
		return -1;

	if (port_conf->dup_conf == NULL)
		return 0;

	if (port_conf->dup_conf->error_check_policy != NULL){
		data = 0;
		crc  = 0;
		len  = 0;

		if (pdu_ser_crc32(pdu, &crc))
			return -1;

		if (pdu_ser_data_and_length(pdu, &data, &len))
			return -1;

		if (memcmp(&crc, data+len-sizeof(crc), sizeof(crc)))
			return -1;

		/* Assuming CRC32 */
		if (pdu_ser_tail_shrink_gfp(pdu, sizeof(u32))) {
			LOG_ERR("Failed to shrink ser PDU");
			return -1;
		}
		LOG_DBG("CHECKCRC!");
	}
	return 0;
}

int common_sdup_add_hmac(struct sdup_ps * ps,
			 struct pdu_ser * pdu,
			 struct sdup_port_conf * port_conf)
{
	LOG_DBG("Adding HMAC");
	return 0;
}

int common_sdup_verify_hmac(struct sdup_ps * ps,
			    struct pdu_ser * pdu,
			    struct sdup_port_conf * port_conf)
{
	LOG_DBG("Checking HMAC");
	return 0;
}

int common_sdup_compress(struct sdup_ps * ps,
			 struct pdu_ser * pdu,
			 struct sdup_port_conf * port_conf)
{
	LOG_DBG("COMPRESSION");
	return 0;
}

int common_sdup_decompress(struct sdup_ps * ps,
			   struct pdu_ser * pdu,
			   struct sdup_port_conf * port_conf)
{
	LOG_DBG("DECOMPRESSION");
	return 0;
}

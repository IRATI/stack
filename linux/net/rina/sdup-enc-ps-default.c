/*
 * Default policy set for SDUP Encryption
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
#include <linux/crypto.h>
#include <linux/scatterlist.h>

#define RINA_PREFIX "sdup-enc-ps-default"

#include "logs.h"
#include "rds/rmem.h"
#include "sdup.h"
#include "sdup-enc-ps.h"
#include "debug.h"

struct sdup_enc_ps_default_data {
	struct crypto_blkcipher * blkcipher;
	bool 			  enable_encryption;
	bool			  enable_decryption;
};

static bool _pdu_ser_data_and_length(struct pdu_ser * pdu,
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

static int _default_sdup_add_padding_policy(struct sdup_ps * ps,
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

static int _default_sdup_remove_padding_policy(struct sdup_ps * ps,
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

static int _default_sdup_encrypt_policy(struct sdup_ps * ps,
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

static int _default_sdup_decrypt_policy(struct sdup_ps * ps,
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

static int _default_sdup_compress(struct sdup_ps * ps,
			  	  struct pdu_ser * pdu,
			  	  struct sdup_port_conf * port_conf)
{
	LOG_DBG("COMPRESSION");
	return 0;
}

static int _default_sdup_decompress(struct sdup_ps * ps,
			    	    struct pdu_ser * pdu,
			    	    struct sdup_port_conf * port_conf)
{
	LOG_DBG("DECOMPRESSION");
	return 0;
}

int default_sdup_encrypt_policy(struct sdup_ps * ps,
				struct pdu_ser * pdu,
				struct sdup_port_conf * port_conf)
{
	int result = 0;

	result = _default_sdup_compress(ps, pdu, port_conf);
	if (result)
		return result;

	result = _default_sdup_add_padding_policy(ps, pdu, port_conf);
	if (result)
		return result;

	return _default_sdup_encrypt_policy(ps, pdu, port_conf);
}

int default_sdup_decrypt_policy(struct sdup_ps * ps,
				struct pdu_ser * pdu,
				struct sdup_port_conf * port_conf)
{
	int result = 0;

	result = _default_sdup_decrypt_policy(ps, pdu, port_conf);
	if (result)
		return result;

	result = _default_sdup_remove_padding_policy(ps, pdu, port_conf);
	if (result)
		return result;

	return _default_sdup_decompress(ps, pdu, port_conf);
}

int default_sdup_add_hmac(struct sdup_ps * ps,
			  struct pdu_ser * pdu,
			  struct sdup_port_conf * port_conf)
{
	LOG_DBG("Adding HMAC");
	return 0;
}

int default_sdup_verify_hmac(struct sdup_ps * ps,
			     struct pdu_ser * pdu,
			     struct sdup_port_conf * port_conf)
{
	LOG_DBG("Checking HMAC");
	return 0;
}

static struct ps_base *
sdup_enc_ps_default_create(struct rina_component * component)
{
	struct sdup * sdup = sdup_from_component(component);
	struct sdup_enc_ps * ps = rkzalloc(sizeof(*ps), GFP_KERNEL);

	if (!ps) {
		return NULL;
	}

	ps->dm          = sdup;
        ps->priv        = NULL;

	/* SDUP policy functions*/
	ps->sdup_encrypt_policy			= default_sdup_encrypt_policy;
	ps->sdup_decrypt_policy			= default_sdup_decrypt_policy;
	ps->sdup_add_hmac			= default_sdup_add_hmac;
	ps->sdup_verify_hmac			= default_sdup_verify_hmac;
	ps->sdup_compress			= default_sdup_compress;
	ps->sdup_decompress			= default_sdup_decompress;

	return &ps->base;
}

static void
sdup_enc_ps_default_destroy(struct ps_base * bps)
{
	struct sdup_enc_ps *ps = container_of(bps, struct sdup_enc_ps, base);

	if (bps) {
		rkfree(ps);
	}
}

struct ps_factory default_sdup_enc_ps_factory = {
	.owner   = THIS_MODULE,
	.create  = sdup_enc_ps_default_create,
	.destroy = sdup_enc_ps_default_destroy,
};
EXPORT_SYMBOL(default_sdup_enc_ps_factory);

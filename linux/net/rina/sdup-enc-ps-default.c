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
#include "policies.h"
#include "rds/rmem.h"
#include "sdup.h"
#include "sdup-enc-ps.h"
#include "debug.h"

struct sdup_enc_ps_default_data {
	struct crypto_blkcipher * blkcipher;
	bool 		enable_encryption;
	bool		enable_decryption;
	string_t * 	encryption_cipher;
	string_t * 	message_digest;
	string_t * 	compress_alg;
};

static struct sdup_enc_ps_default_data * priv_data_create(void)
{
	struct sdup_enc_ps_default_data * data =
			rkmalloc(sizeof(*data), GFP_KERNEL);
	if (!data)
		return NULL;

	data->blkcipher = NULL;
	data->enable_decryption = false;
	data->enable_encryption = false;
	data->encryption_cipher = NULL;
	data->message_digest = NULL;
	data->compress_alg = NULL;

	return data;
}

static void priv_data_destroy(struct sdup_enc_ps_default_data * data)
{
	if (!data)
		return;

	if (data->blkcipher)
		crypto_free_blkcipher(data->blkcipher);

	if (data->compress_alg) {
		rkfree(data->compress_alg);
		data->compress_alg = NULL;
	}

	if (data->encryption_cipher) {
		rkfree(data->encryption_cipher);
		data->encryption_cipher = NULL;
	}

	if (data->message_digest) {
		rkfree(data->message_digest);
		data->message_digest = NULL;
	}

	rkfree(data);
}

static int default_sdup_add_padding_policy(struct sdup_enc_ps_default_data * priv_data,
				    	   struct pdu_ser * pdu)
{
	struct buffer * buf;
	ssize_t		buffer_size;
	ssize_t		padded_size;
	ssize_t		blk_size;
	char *		data;
	int i;

	if (!priv_data || !pdu){
		LOG_ERR("Encryption arguments not initialized!");
		return -1;
	}

	/* encryption and therefore padding is disabled */
	if (priv_data->blkcipher == NULL ||
	    !priv_data->enable_encryption)
		return 0;

	LOG_DBG("PADDING!");

	buf = pdu_ser_buffer(pdu);
	blk_size = crypto_blkcipher_blocksize(priv_data->blkcipher);
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

static int default_sdup_remove_padding_policy(struct sdup_enc_ps_default_data * priv_data,
				       	      struct pdu_ser * pdu)
{
	struct buffer *	buf;
	const char *	data;
	ssize_t		len;
	ssize_t		pad_len;
	int		i;

	if (!priv_data || !pdu){
		LOG_ERR("Encryption arguments not initialized!");
		return -1;
	}

	/* decryption and therefore padding is disabled */
	if (priv_data->blkcipher == NULL ||
	    !priv_data->enable_decryption)
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

static int default_sdup_encrypt_policy(struct sdup_enc_ps_default_data * priv_data,
				       struct pdu_ser * pdu)
{
	struct blkcipher_desc	desc;
	struct scatterlist	sg_src;
	struct scatterlist	sg_dst;
	struct buffer *		buf;
	ssize_t			buffer_size;
	void *			data;

	if (!priv_data || !pdu){
		LOG_ERR("Encryption arguments not initialized!");
		return -1;
	}

	/* encryption is disabled */
	if (priv_data->blkcipher == NULL ||
	    !priv_data->enable_encryption)
		return 0;

	LOG_DBG("ENCRYPT!");

	desc.flags = 0;
	desc.tfm = priv_data->blkcipher;
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

static int default_sdup_decrypt_policy(struct sdup_enc_ps_default_data * priv_data,
				       struct pdu_ser * pdu)
{
	struct blkcipher_desc	desc;
	struct scatterlist	sg_src;
	struct scatterlist	sg_dst;
	struct buffer *		buf;
	ssize_t			buffer_size;
	void *			data;

	if (!priv_data || !pdu){
		LOG_ERR("Failed decryption");
		return -1;
	}

	/* decryption is disabled */
	if (priv_data->blkcipher == NULL ||
	    !priv_data->enable_decryption)
		return 0;

	LOG_DBG("DECRYPT!");

	desc.flags = 0;
	desc.tfm = priv_data->blkcipher;
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

static int default_sdup_compress(struct sdup_enc_ps_default_data * priv_data,
			  	  struct pdu_ser * pdu)
{
	LOG_DBG("COMPRESSION");
	return 0;
}

static int default_sdup_decompress(struct sdup_enc_ps_default_data * priv_data,
			    	    struct pdu_ser * pdu)
{
	LOG_DBG("DECOMPRESSION");
	return 0;
}

int default_sdup_encrypt(struct sdup_enc_ps * ps,
			 struct pdu_ser * pdu)
{
	int result = 0;
	struct sdup_enc_ps_default_data * priv_data = ps->priv;

	result = default_sdup_compress(priv_data, pdu);
	if (result)
		return result;

	result = default_sdup_add_padding_policy(priv_data, pdu);
	if (result)
		return result;

	return default_sdup_encrypt_policy(priv_data, pdu);
}

int default_sdup_decrypt(struct sdup_enc_ps * ps,
			 struct pdu_ser * pdu)
{
	int result = 0;
	struct sdup_enc_ps_default_data * priv_data = ps->priv;

	result = default_sdup_decrypt_policy(priv_data, pdu);
	if (result)
		return result;

	result = default_sdup_remove_padding_policy(priv_data, pdu);
	if (result)
		return result;

	return default_sdup_decompress(priv_data, pdu);
}

int default_sdup_add_hmac(struct sdup_enc_ps * ps,
			  struct pdu_ser * pdu)
{
	LOG_DBG("Adding HMAC");
	return 0;
}

int default_sdup_verify_hmac(struct sdup_enc_ps * ps,
			     struct pdu_ser * pdu)
{
	LOG_DBG("Checking HMAC");
	return 0;
}

int default_sdup_enable_encryption(struct sdup_enc_ps * ps,
				   bool enable_encryption,
				   bool enable_decryption,
				   struct buffer * encrypt_key)
{
	struct sdup_enc_ps_default_data * priv_data;

	if (!ps) {
		LOG_ERR("Bogus SDUP Encryption PS passed");
		return -1;
	}

	if (!encrypt_key) {
		LOG_ERR("Bogus encryption key passed");
		return -1;
	}

	priv_data = ps->priv;

	if (!priv_data->blkcipher) {
		LOG_ERR("Block cipher is not set for N-1 port %d",
			ps->dm->port_id);
		return -1;
	}

	if (!priv_data->enable_decryption &&
	    !priv_data->enable_encryption) {
		if (crypto_blkcipher_setkey(priv_data->blkcipher,
					    buffer_data_ro(encrypt_key),
					    buffer_length(encrypt_key))) {
			LOG_ERR("Could not set encryption key for N-1 port %d",
				ps->dm->port_id);
			return -1;
		}
	}

	if (!priv_data->enable_decryption){
		priv_data->enable_decryption = enable_decryption;
	}
	if (!priv_data->enable_encryption){
		priv_data->enable_encryption = enable_encryption;
	}

	LOG_DBG("Encryption enabled state: %d", port_conf->enable_encryption);
	LOG_DBG("Decryption enabled state: %d", port_conf->enable_decryption);

	return 0;
}

static struct ps_base *
sdup_enc_ps_default_create(struct rina_component * component)
{
	struct dup_config_entry * conf;
	struct sdup_comp * sdup_comp;
	struct sdup_enc_ps * ps;
	struct sdup_port * sdup_port;
	struct sdup_enc_ps_default_data * data;
	struct policy_parm * parameter;
	const string_t * aux;

	sdup_comp = sdup_comp_from_component(component);
	if (!sdup_comp)
		return NULL;

	sdup_port = sdup_comp->parent;
	if (!sdup_port)
		return NULL;

	conf = sdup_port->conf;
	if (!conf)
		return NULL;

	ps = rkzalloc(sizeof(*ps), GFP_KERNEL);
	if (!ps)
		return NULL;

	data = priv_data_create();
	if (!data) {
		rkfree(ps);
		return NULL;
	}

	ps->dm          = sdup_port;
        ps->priv        = data;

        /* Parse policy parameters */
        if (conf->encryption_policy) {
        	parameter = policy_param_find(conf->encryption_policy,
        				      "encryptAlg");
        	if (!parameter) {
        		LOG_ERR("Could not find 'encryptAlg' in Encr. policy");
        		rkfree(ps);
        		priv_data_destroy(data);
        		return NULL;
        	}

        	aux = policy_param_value(parameter);
        	if (string_cmp(aux, "AES128") == 0 ||
        			string_cmp(aux, "AES256") == 0) {
        		if (string_dup("ecb(aes)",
        				&data->encryption_cipher)) {
        			LOG_ERR("Problems copying 'encryptAlg' value");
        			rkfree(ps);
        			priv_data_destroy(data);
        			return NULL;
        		}
        		LOG_DBG("Encryption cipher is %s",
        				entry->encryption_cipher);
        	} else {
        		LOG_DBG("Unsupported encryption cipher %s", aux);
        	}

        	parameter = policy_param_find(conf->encryption_policy,
        				      "macAlg");
        	if (!parameter) {
        		LOG_ERR("Could not find 'macAlg' in Encrypt. policy");
        		rkfree(ps);
        		priv_data_destroy(data);
        		return NULL;
        	}

        	aux = policy_param_value(parameter);
        	if (string_cmp(aux, "SHA1") == 0) {
        		if (string_dup("sha1", &data->message_digest)) {
        			LOG_ERR("Problems copying 'digest' value");
        			rkfree(ps);
        			priv_data_destroy(data);
        			return NULL;
        		}
        		LOG_DBG("Message digest is %s", entry->message_digest);
        	} else if (string_cmp(aux, "MD5") == 0) {
        		if (string_dup("md5", &data->message_digest)) {
        			LOG_ERR("Problems copying 'digest' value)");
           			rkfree(ps);
           			priv_data_destroy(data);
           			return NULL;
        		}
        		LOG_DBG("Message digest is %s", entry->message_digest);
        	} else {
        		LOG_DBG("Unsupported message digest %s", aux);
        	}
        } else {
        	LOG_ERR("Bogus configuration passed");
		rkfree(ps);
		priv_data_destroy(data);
		return NULL;
        }

        /* Instantiate block cipher */
        data->blkcipher =
        		crypto_alloc_blkcipher(data->encryption_cipher,
        				       0,
        				       0);
        if (IS_ERR(data->blkcipher)) {
        	LOG_ERR("could not allocate blkcipher handle for %s\n",
        		data->encryption_cipher);
		rkfree(ps);
		priv_data_destroy(data);
        	return NULL;
        }

	/* SDUP policy functions*/
	ps->sdup_encrypt		= default_sdup_encrypt;
	ps->sdup_decrypt		= default_sdup_decrypt;
	ps->sdup_add_hmac		= default_sdup_add_hmac;
	ps->sdup_verify_hmac		= default_sdup_verify_hmac;
	ps->sdup_enable_encryption	= default_sdup_enable_encryption;

	return &ps->base;
}

static void
sdup_enc_ps_default_destroy(struct ps_base * bps)
{
	struct sdup_enc_ps_default_data * data;
	struct sdup_enc_ps *ps;

	ps = container_of(bps, struct sdup_enc_ps, base);
	data = ps->priv;

	if (bps) {
		if (data)
			priv_data_destroy(data);
		rkfree(ps);
	}
}

struct ps_factory default_sdup_enc_ps_factory = {
	.owner   = THIS_MODULE,
	.create  = sdup_enc_ps_default_create,
	.destroy = sdup_enc_ps_default_destroy,
};
EXPORT_SYMBOL(default_sdup_enc_ps_factory);

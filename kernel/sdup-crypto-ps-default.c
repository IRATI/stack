/*
 * SDU Protection Cryptographic Policy Set (includes a combination of
 * compression, Message Authentication Codes and Encryption mechanisms)
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
#include <linux/random.h>
#include <linux/version.h>
#include <crypto/hash.h>

#define RINA_PREFIX "sdup-crypto-ps-default"

#include "logs.h"
#include "policies.h"
#include "rds/rmem.h"
#include "sdup-crypto-ps-default.h"
#include "debug.h"


struct sdup_crypto_ps_default_crypto_state {
	struct crypto_blkcipher * blkcipher;

	struct crypto_shash * shash;

	struct crypto_comp * compress;
	unsigned int comp_scratch_size;
	char * comp_scratch;

	string_t * 	enc_alg;
	string_t * 	mac_alg;
	string_t * 	compress_alg;
};

struct sdup_crypto_ps_default_data {
	struct sdup_crypto_ps_default_crypto_state * current_rx_state;
	struct sdup_crypto_ps_default_crypto_state * current_tx_state;
	struct sdup_crypto_ps_default_crypto_state * next_rx_state;
	struct sdup_crypto_ps_default_crypto_state * next_tx_state;

	/*next seq num to be used on tx*/
	unsigned int    tx_seq_num;
	/*highest received seq number (most significant bit of bitmap)*/
	unsigned int    rx_seq_num;
	/*bitmap of received seq numbers*/
	unsigned char * seq_bmap;
	unsigned int seq_bmap_len;
	unsigned int seq_win_size;

};

static struct sdup_crypto_ps_default_crypto_state * crypto_state_create(void)
{
	struct sdup_crypto_ps_default_crypto_state * state =
		rkmalloc(sizeof(*state), GFP_KERNEL);

	state->blkcipher = NULL;

	state->shash = NULL;

	state->compress = NULL;
	state->comp_scratch = NULL;
	state->comp_scratch_size = 0;

	state->enc_alg = NULL;
	state->mac_alg = NULL;
	state->compress_alg = NULL;

	return state;
}

static void crypto_state_destroy(struct sdup_crypto_ps_default_crypto_state * state)
{
	if (state->blkcipher)
		crypto_free_blkcipher(state->blkcipher);

	if (state->shash)
		crypto_free_shash(state->shash);

	if (state->compress)
		crypto_free_comp(state->compress);
	if (state->comp_scratch)
		rkfree(state->comp_scratch);

	if (state->compress_alg) {
		rkfree(state->compress_alg);
		state->compress_alg = NULL;
	}

	if (state->enc_alg) {
		rkfree(state->enc_alg);
		state->enc_alg = NULL;
	}

	if (state->mac_alg) {
		rkfree(state->mac_alg);
		state->mac_alg = NULL;
	}

	rkfree(state);
}

static struct sdup_crypto_ps_default_data * priv_data_create(void);
static void priv_data_destroy(struct sdup_crypto_ps_default_data * data);

static struct sdup_crypto_ps_default_data * priv_data_create(void)
{
	struct sdup_crypto_ps_default_data * data =
		rkmalloc(sizeof(*data), GFP_KERNEL);
	if (!data)
		return NULL;

	data->current_tx_state = crypto_state_create();
	if (!data->current_tx_state){
		priv_data_destroy(data);
		return NULL;
	}

	data->current_rx_state = crypto_state_create();
	if (!data->current_rx_state){
		priv_data_destroy(data);
		return NULL;
	}

	data->next_tx_state = crypto_state_create();
	if (!data->next_tx_state){
		priv_data_destroy(data);
		return NULL;
	}

	data->next_rx_state = crypto_state_create();
	if (!data->next_rx_state){
		priv_data_destroy(data);
		return NULL;
	}

	data->tx_seq_num = 0;
	data->rx_seq_num = 0;
	data->seq_bmap = NULL;
	data->seq_win_size = 0;
	data->seq_bmap_len = 0;

	return data;
}

static void priv_data_destroy(struct sdup_crypto_ps_default_data * data)
{
	if (!data)
		return;

	if (data->current_tx_state)
		crypto_state_destroy(data->current_tx_state);

	if (data->current_rx_state)
		crypto_state_destroy(data->current_rx_state);

	if (data->next_tx_state)
		crypto_state_destroy(data->next_tx_state);

	if (data->next_rx_state)
		crypto_state_destroy(data->next_rx_state);

	if (data->seq_bmap) {
		rkfree(data->seq_bmap);
		data->seq_bmap = NULL;
	}

	rkfree(data);
}

static int add_padding(struct sdup_crypto_ps_default_data * priv_data,
		       struct du * du)
{
	struct sdup_crypto_ps_default_crypto_state * state;
	unsigned int	buffer_size;
	unsigned int	padded_size;
	unsigned int	blk_size;
	char *		buf_data;
	int i;

	if (!priv_data || !du || !priv_data->current_tx_state){
		LOG_ERR("Encryption arguments not initialized!");
		return -1;
	}

	state = priv_data->current_tx_state;

	/* encryption and therefore padding is disabled */
	if (!state->blkcipher)
		return 0;

	LOG_DBG("PADDING!");

	blk_size = crypto_blkcipher_blocksize(state->blkcipher);
	buffer_size = du_len(du);
	padded_size = (buffer_size/blk_size + 1) * blk_size;

	if (du_tail_grow(du, padded_size - buffer_size)){
		LOG_ERR("Failed to grow ser PDU");
		return -1;
	}

	/* PADDING */
	buf_data = du_buffer(du);
	for (i=padded_size-1; i>=buffer_size; i--){
		buf_data[i] = padded_size - buffer_size;
	}

	return 0;
}

static int remove_padding(struct sdup_crypto_ps_default_data * priv_data,
			  struct du * du)
{
	struct sdup_crypto_ps_default_crypto_state * state;
	const char *	buf_data;
	ssize_t		len;
	ssize_t		pad_len;
	int		i;

	if (!priv_data || !du || !priv_data->current_rx_state){
		LOG_ERR("Encryption arguments not initialized!");
		return -1;
	}

	state = priv_data->current_rx_state;

	/* decryption and therefore padding is disabled */
	if (!state->blkcipher)
		return 0;

	LOG_DBG("UNPADDING!");
	buf_data = du_buffer(du);
	len = du_len(du);
	pad_len = buf_data[len-1];

	//check padding
	for (i=len-1; i >= len-pad_len; --i){
		if (buf_data[i] != pad_len){
			LOG_ERR("Padding check failed!");
			return -1;
		}
	}

	//remove padding
	if (du_tail_shrink(du, pad_len)){
		LOG_ERR("Failed to shrink serialized PDU");
		return -1;
	}

	return 0;
}

static int encrypt(struct sdup_crypto_ps_default_data * priv_data,
		   struct du * du)
{
	struct sdup_crypto_ps_default_crypto_state * state;
	struct blkcipher_desc	desc;
	struct scatterlist	sg;
	ssize_t			buffer_size;
	void *			data;
	char *                  iv;
	unsigned int		ivsize;

	if (!priv_data || !du || !priv_data->current_tx_state){
		LOG_ERR("Encryption arguments not initialized!");
		return -1;
	}

	state = priv_data->current_tx_state;

	/* encryption is disabled */
	if (state->blkcipher == NULL)
		return 0;

	desc.flags = 0;
	desc.tfm = state->blkcipher;
	buffer_size = du_len(du);
	data = du_buffer(du);

	iv = NULL;
	ivsize = crypto_blkcipher_ivsize(state->blkcipher);
	if (ivsize) {
		if(du_head_grow(du, ivsize)){
			LOG_ERR("IV allocation failed!");
		}
		iv = du_buffer(du);
		get_random_bytes(iv, ivsize);
	}

	sg_init_one(&sg, data, buffer_size);

	if (iv)
		crypto_blkcipher_set_iv(state->blkcipher, iv, ivsize);

	if (crypto_blkcipher_encrypt(&desc, &sg, &sg, buffer_size)) {
		LOG_ERR("Encryption failed!");
		if (iv)
			du_head_shrink(du, ivsize);
		return -1;
	}

	return 0;
}

static int decrypt(struct sdup_crypto_ps_default_data * priv_data,
		   struct du * du)
{
	struct sdup_crypto_ps_default_crypto_state * state;
	struct blkcipher_desc	desc;
	struct scatterlist	sg;
	unsigned int		buffer_size;
	void *			data;
	char *                  iv;
	unsigned int		ivsize;

	if (!priv_data || !du || !priv_data->current_rx_state){
		LOG_ERR("Failed decryption");
		return -1;
	}

	state = priv_data->current_rx_state;

	/* decryption is disabled */
	if (state->blkcipher == NULL)
		return 0;

	buffer_size = du_len(du);
	data = du_buffer(du);

	LOG_DBG("DECRYPT original buffer_size %d", buffer_size);

	iv = NULL;
	ivsize = crypto_blkcipher_ivsize(state->blkcipher);
	if (ivsize) {
		iv = data;
		if(du_head_shrink(du, ivsize)){
			LOG_ERR("Failed to shrink ser PDU by IV");
			return -1;
		}
		data = du_buffer(du);
		buffer_size = du_len(du);
	}

	desc.flags = 0;
	desc.tfm = state->blkcipher;

	sg_init_one(&sg, data, buffer_size);

	if (iv)
		crypto_blkcipher_set_iv(state->blkcipher, iv, ivsize);

	if (crypto_blkcipher_decrypt(&desc, &sg, &sg, buffer_size)) {
		LOG_ERR("Decryption failed!");
		return -1;
	}

	return 0;
}

static int add_hmac(struct sdup_crypto_ps_default_data * priv_data,
		    struct du * du)
{
	struct sdup_crypto_ps_default_crypto_state * state;
	unsigned int		buffer_size;
	void *			data;
	unsigned int		digest_size;
	SHASH_DESC_ON_STACK(shash, priv_data->current_tx_state->shash);

	if (!priv_data || !du){
		LOG_ERR("HMAC arguments not initialized!");
		return -1;
	}

	state = priv_data->current_tx_state;

	/* encryption is disabled so hmac is disabled*/
	if (state->shash == NULL)
		return 0;

	buffer_size = du_len(du);
	digest_size = crypto_shash_digestsize(state->shash);
	data = du_buffer(du);

	if (du_tail_grow(du, digest_size)){
		LOG_ERR("Failed to grow ser PDU for HMAC");
		return -1;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,2,0)
	shash->flags = 0;
#else
#endif
	shash->tfm = state->shash;

	if (crypto_shash_digest(shash, data, buffer_size, data+buffer_size)) {
		LOG_ERR("HMAC calculation failed!");
		return -1;
	}

	return 0;
}

static int check_hmac(struct sdup_crypto_ps_default_data * priv_data,
		      struct du * du)
{
	struct sdup_crypto_ps_default_crypto_state * state;
	unsigned int		buffer_size;
	void *			data;
	unsigned int		digest_size;
	char *			verify_digest;
	SHASH_DESC_ON_STACK(shash, priv_data->current_rx_state->shash);

	if (!priv_data || !du){
		LOG_ERR("HMAC arguments not initialized!");
		return -1;
	}

	state = priv_data->current_rx_state;

	/* decryption is disabled so hmac is disabled*/
	if (state->shash == NULL)
		return 0;

	buffer_size = du_len(du);
	data = du_buffer(du);

	digest_size = crypto_shash_digestsize(state->shash);
	verify_digest = rkzalloc(digest_size, GFP_KERNEL);
	if(!verify_digest){
		LOG_ERR("HMAC allocation failed!");
		return -1;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,2,0)
	shash->flags = 0;
#else
#endif
	shash->tfm = state->shash;

	if (crypto_shash_digest(shash, data, buffer_size-digest_size, verify_digest)) {
		LOG_ERR("HMAC calculation failed!");
		rkfree(verify_digest);
		return -1;
	}

	if (memcmp(verify_digest, data+buffer_size-digest_size, digest_size)){
		LOG_ERR("HMAC verification FAILED!");
		rkfree(verify_digest);
		return -1;
	}

	if (du_tail_shrink(du, digest_size)){
		LOG_ERR("Failed to shrink serialized PDU");
		rkfree(verify_digest);
		return -1;
	}

	rkfree(verify_digest);
	return 0;
}

static int compress(struct sdup_crypto_ps_default_data * priv_data,
		    struct du* du)
{
	struct sdup_crypto_ps_default_crypto_state * state;
	unsigned int		buffer_size;
	void *			data;
	unsigned int		compressed_size;
	char *                  compressed_data;
	int err;

	if (!priv_data || !du || !priv_data->current_tx_state){
		LOG_ERR("Compression arguments not initialized!");
		return -1;
	}

	state = priv_data->current_tx_state;

	/* encryption is disabled so compression is disabled*/
	if (state->compress == NULL)
		return 0;

	buffer_size = du_len(du);
	data = du_buffer(du);

	compressed_size = state->comp_scratch_size;
	compressed_data = state->comp_scratch;

	err = crypto_comp_compress(state->compress,
				   data, buffer_size,
				   compressed_data, &compressed_size);

	if (err){
		LOG_ERR("Failed compression!");
		return -1;
	}

	LOG_DBG("DBG Compressed size (%d), uncompressed size (%d)",
							compressed_size,
							buffer_size);
	if (buffer_size > compressed_size){
		if (du_head_shrink(du, buffer_size -compressed_size)){
			LOG_ERR("Failed to shrink PDU for compressed data!");
			return -1;
		}
	}else{
		LOG_DBG("Compressed size (%d) is > than uncompressed size (%d) after compression",
							compressed_size,
							buffer_size);
		if (du_head_grow(du, compressed_size - buffer_size)){
			LOG_ERR("Failed to groe PDU for compressed data!");
			return -1;
		}
	}
	data = du_buffer(du);

	memcpy(data, compressed_data, compressed_size);

	return 0;
}

static int decompress(struct sdup_crypto_ps_default_data * priv_data,
		      u_int32_t max_pdu_size,
		      struct du * du)
{
	struct sdup_crypto_ps_default_crypto_state * state;
	unsigned int		buffer_size;
	void *			data;
	void *			decompressed_data;
	unsigned int		decompressed_size;
	int err;

	if (!priv_data || !du || !priv_data->current_rx_state){
		LOG_ERR("Decompression arguments not initialized!");
		return -1;
	}

	state = priv_data->current_rx_state;

	/* decryption is disabled so decompression is disabled*/
	if (state->compress == NULL)
		return 0;

	buffer_size = du_len(du);
	data = du_buffer(du);

	decompressed_size = max_pdu_size;
	decompressed_data = rkzalloc(decompressed_size, GFP_KERNEL);
	if (!decompressed_data){
		LOG_ERR("Failed to alloc data for compression");
		return -1;
	}

	err = crypto_comp_decompress(state->compress,
				     data, buffer_size,
				     decompressed_data, &decompressed_size);

	if (err){
		LOG_ERR("Failed decompression!");
		rkfree(decompressed_data);
		return -1;
	}

	LOG_DBG("Compressed size (%d), decompressed size (%d)",
							buffer_size,
							decompressed_size);
	if (buffer_size > decompressed_size){
		LOG_DBG("Compressed size (%d) is > than decompressed size (%d) after decompression",
							buffer_size,
							decompressed_size);

		if (du_head_shrink(du, buffer_size - decompressed_size)){
			LOG_ERR("Failed to shrink PDU header for uncompressed data!");
			rkfree(decompressed_data);
			return -1;
		}
	}else{
		if (du_head_grow(du, decompressed_size - buffer_size)){
			LOG_ERR("Failed to grow PDU header for uncompressed data!");
			rkfree(decompressed_data);
			return -1;
		}
	}
	data = du_buffer(du);

	memcpy(data, decompressed_data, decompressed_size);
	rkfree(decompressed_data);

	return 0;
}

static int add_seq_num(struct sdup_crypto_ps_default_data * priv_data,
		       struct du * du)
{
	struct sdup_crypto_ps_default_crypto_state * state;
	char *		data;

	if (!priv_data || !du || !priv_data->current_tx_state){
		LOG_ERR("Encryption arguments not initialized!");
		return -1;
	}

	state = priv_data->current_tx_state;

	/* encryption and therefore sequence numbers are disabled */
	if (!state->blkcipher)
		return 0;

	if (du_head_grow(du, sizeof(priv_data->tx_seq_num))){
		LOG_ERR("Failed to grow ser PDU");
		return -1;
	}

	data = du_buffer(du);
	memcpy(data, &priv_data->tx_seq_num, sizeof(priv_data->tx_seq_num));

	LOG_DBG("Added sequence number %u", priv_data->tx_seq_num);

	priv_data->tx_seq_num += 1;

	return 0;
}

static int del_seq_num(struct sdup_crypto_ps_default_data * priv_data,
		       struct du * du)
{
	struct sdup_crypto_ps_default_crypto_state * state;
	char *		data;
	unsigned int    seq_num;
	long int	min_seq_num;
	int		i, shift_length;
	unsigned char * bmap;
	unsigned int	bit_pos, byte_pos;

	if (!priv_data || !du || !priv_data->current_rx_state){
		LOG_ERR("Encryption arguments not initialized!");
		return -1;
	}

	state = priv_data->current_rx_state;

	/* decryption and therefore sequence numbers are disabled */
	if (!state->blkcipher)
		return 0;

	data = du_buffer(du);

	memcpy(&seq_num, data, sizeof(priv_data->rx_seq_num));

	if (du_head_shrink(du, sizeof(priv_data->tx_seq_num))){
		LOG_ERR("Failed to grow ser PDU");
		return -1;
	}

	LOG_DBG("Got sequence number %u", seq_num);

	if (priv_data->seq_win_size == 0)
		return 0;

	min_seq_num = (long int)priv_data->rx_seq_num - priv_data->seq_win_size;
	if (seq_num < min_seq_num){
		LOG_ERR("Sequence number %u is too old", seq_num);
		return -1;
	} else {
		//shift bitmap to right
		shift_length = seq_num - priv_data->rx_seq_num;
		bmap = priv_data->seq_bmap;
		while (shift_length-- > 0){
			for (i = priv_data->seq_bmap_len - 1; i>=0; i--){
				bmap[i] >>= 1;
				if (i>0 && bmap[i-1] & 1){
					bmap[i] |= 1<<7;
				}
			}
		}

		//set new highest received sequence number
		if (seq_num > priv_data->rx_seq_num)
			priv_data->rx_seq_num = seq_num;

		bit_pos = priv_data->rx_seq_num - seq_num;
		byte_pos = bit_pos / 8;
		bit_pos = 7 - (bit_pos % 8);

		//check bitmap for duplicate sequence number
		if (bmap[byte_pos] & (1 << bit_pos)){
			LOG_ERR("Sequence number %u already received.",
				seq_num);
			return -1;
		} else {
			bmap[byte_pos] |= 1 << bit_pos;
			return 0;
		}
	}
	return 0;
}

int default_sdup_apply_crypto(struct sdup_crypto_ps * ps,
			      struct du * du)
{
	int result = 0;
	struct sdup_crypto_ps_default_data * priv_data = ps->priv;

	result = compress(priv_data, du);
	if (result)
		return result;

	result = add_seq_num(priv_data, du);
	if (result)
		return result;

	if (priv_data->current_tx_state->shash)
		result = add_hmac(priv_data, du);
	if (result)
		return result;

	result = add_padding(priv_data, du);
	if (result)
		return result;

	result = encrypt(priv_data, du);
	if (result)
		return result;

	return result;
}
EXPORT_SYMBOL(default_sdup_apply_crypto);

int default_sdup_remove_crypto(struct sdup_crypto_ps * ps,
			       struct du * du)
{
	int result = 0;
	struct sdup_crypto_ps_default_data * priv_data = ps->priv;
	struct sdup_port * port = ps->dm;
	struct dt_cons * dt_cons = port->dt_cons;

	result = decrypt(priv_data, du);
	if (result)
		return result;

	result = remove_padding(priv_data, du);
	if (result)
		return result;

	if (priv_data->current_rx_state->shash)
		result = check_hmac(priv_data, du);
	if (result)
		return result;

	result = del_seq_num(priv_data, du);
	if (result)
		return result;

	result = decompress(priv_data, dt_cons->max_pdu_size, du);
	if (result)
		return result;

	return result;
}
EXPORT_SYMBOL(default_sdup_remove_crypto);


int default_sdup_update_crypto_state(struct sdup_crypto_ps * ps,
				     struct sdup_crypto_state * state)
{
	struct sdup_crypto_ps_default_data * priv_data;
	struct sdup_crypto_ps_default_crypto_state * next_tx_state;
	struct sdup_crypto_ps_default_crypto_state * next_rx_state;
	struct sdup_port * sdup_port;

	if (!ps || !state) {
		LOG_ERR("Bogus input parameters passed");
		return -1;
	}

	sdup_port = ps->dm;
	priv_data = ps->priv;
	next_tx_state = priv_data->next_tx_state;
	next_rx_state = priv_data->next_rx_state;

	if (state->enc_alg && string_cmp(state->enc_alg, "") != 0) {
		if (string_cmp(state->enc_alg, "AES128") == 0 ||
		    string_cmp(state->enc_alg, "AES256") == 0) {
			if (string_dup("cbc(aes)",
				       &next_tx_state->enc_alg)) {
				LOG_ERR("Problems copying 'enc_alg' value");
				return -1;
			}
			if (string_dup("cbc(aes)",
				       &next_rx_state->enc_alg)) {
				LOG_ERR("Problems copying 'enc_alg' value");
				return -1;
			}
			LOG_DBG("TX encryption cipher is %s", next_tx_state->enc_alg);
			LOG_DBG("RX encryption cipher is %s", next_rx_state->enc_alg);
		} else {
			LOG_ERR("Unsupported encryption algorithm %s",
				state->enc_alg);
			return -1;
		}

		next_tx_state->blkcipher = crypto_alloc_blkcipher(next_tx_state->enc_alg,
								  0,0);
		next_rx_state->blkcipher = crypto_alloc_blkcipher(next_rx_state->enc_alg,
								  0,0);
		if (IS_ERR(next_tx_state->blkcipher)) {
			LOG_ERR("could not allocate tx blkcipher handle for %s\n",
				next_tx_state->enc_alg);
			return -1;
		}
		if (IS_ERR(next_rx_state->blkcipher)) {
			LOG_ERR("could not allocate rx blkcipher handle for %s\n",
				next_rx_state->enc_alg);
			return -1;
		}
	}

	if (state->encrypt_key_tx) {
		if (crypto_blkcipher_setkey(next_tx_state->blkcipher,
					    buffer_data_ro(state->encrypt_key_tx),
					    buffer_length(state->encrypt_key_tx))) {
			LOG_ERR("Could not set tx encryption key for N-1 port %d",
				ps->dm->port_id);
			return -1;
		}
	}
	if (state->encrypt_key_rx) {
		if (crypto_blkcipher_setkey(next_rx_state->blkcipher,
					    buffer_data_ro(state->encrypt_key_rx),
					    buffer_length(state->encrypt_key_rx))) {
			LOG_ERR("Could not set rx encryption key for N-1 port %d",
				ps->dm->port_id);
			return -1;
		}
	}

	if (state->mac_alg && string_cmp(state->mac_alg, "") != 0) {
		if (string_cmp(state->mac_alg, "SHA256") == 0) {
			if (string_dup("hmac(sha256)", &next_tx_state->mac_alg)) {
				LOG_ERR("Problems copying 'mac_alg' value");
				return -1;
			}
			if (string_dup("hmac(sha256)", &next_rx_state->mac_alg)) {
				LOG_ERR("Problems copying 'mac_alg' value");
				return -1;
			}
			LOG_DBG("TX message digest is %s", next_tx_state->mac_alg);
			LOG_DBG("RX message digest is %s", next_rx_state->mac_alg);
		} else if (string_cmp(state->mac_alg, "MD5") == 0) {
			if (string_dup("hmac(md5)", &next_tx_state->mac_alg)) {
				LOG_ERR("Problems copying 'mac_alg' value");
				return -1;
			}
			if (string_dup("hmac(md5)", &next_rx_state->mac_alg)) {
				LOG_ERR("Problems copying 'mac_alg' value");
				return -1;
			}
			LOG_DBG("TX message digest is %s", next_tx_state->mac_alg);
			LOG_DBG("RX message digest is %s", next_rx_state->mac_alg);
		} else {
			LOG_ERR("Unsupported mac_alg %s", state->mac_alg);
			return -1;
		}

		next_tx_state->shash = crypto_alloc_shash(next_tx_state->mac_alg, 0,0);
		next_rx_state->shash = crypto_alloc_shash(next_rx_state->mac_alg, 0,0);

		if (IS_ERR(next_tx_state->shash)) {
			LOG_ERR("Could not allocate tx shash handle for %s\n",
				next_tx_state->mac_alg);
			return -1;
		}
		if (IS_ERR(next_rx_state->shash)) {
			LOG_ERR("Could not allocate rx shash handle for %s\n",
				next_rx_state->mac_alg);
			return -1;
		}
	}
	if (state->mac_key_tx) {
		if (crypto_shash_setkey(next_tx_state->shash,
					buffer_data_ro(state->mac_key_tx),
					buffer_length(state->mac_key_tx))) {
			LOG_ERR("Could not set tx mac key for N-1 port %d",
				ps->dm->port_id);
			return -1;
		}
	}
	if (state->mac_key_rx) {
		if (crypto_shash_setkey(next_rx_state->shash,
					buffer_data_ro(state->mac_key_rx),
					buffer_length(state->mac_key_rx))) {
			LOG_ERR("Could not set rx mac key for N-1 port %d",
				ps->dm->port_id);
			return -1;
		}
	}

	if (state->compress_alg && string_cmp(state->compress_alg, "") != 0) {
		if (string_cmp(state->compress_alg, "deflate") == 0) {
			if (string_dup("deflate", &next_tx_state->compress_alg)) {
				LOG_ERR("Problems copying 'compress_alg' value");
				return -1;
			}
			if (string_dup("deflate", &next_rx_state->compress_alg)) {
				LOG_ERR("Problems copying 'compress_alg' value");
				return -1;
			}
			LOG_DBG("TX compress algorithm is %s", next_tx_state->compress_alg);
			LOG_DBG("RX compress algorithm is %s", next_rx_state->compress_alg);
		} else {
			LOG_ERR("Unsupported compress algorithm %s", state->compress_alg);
			return -1;
		}

		next_tx_state->compress = crypto_alloc_comp(next_tx_state->compress_alg,
							    0,0);
		next_rx_state->compress = crypto_alloc_comp(next_tx_state->compress_alg,
							    0,0);

		if (IS_ERR(next_tx_state->compress)) {
			LOG_ERR("Could not allocate tx compress handle for %s\n",
				next_tx_state->compress_alg);
			return -1;
		}
		if (IS_ERR(next_rx_state->compress)) {
			LOG_ERR("Could not allocate rx compress handle for %s\n",
				next_rx_state->compress_alg);
			return -1;
		}

		next_tx_state->comp_scratch_size = sdup_port->dt_cons->max_pdu_size +
			MAX_COMP_INFLATION;
		next_tx_state->comp_scratch = rkzalloc(next_tx_state->comp_scratch_size,
						       GFP_KERNEL);
		if (!next_tx_state->comp_scratch) {
			LOG_ERR("Could not allocate scratch space for tx compression");
			return -1;
		}

		next_rx_state->comp_scratch_size = sdup_port->dt_cons->max_pdu_size +
			MAX_COMP_INFLATION;
		next_rx_state->comp_scratch = rkzalloc(next_rx_state->comp_scratch_size,
						       GFP_KERNEL);
		if (!next_rx_state->comp_scratch) {
			LOG_ERR("Could not allocate scratch space for rx compression");
			return -1;
		}
	}

	if (state->enable_crypto_rx){
		crypto_state_destroy(priv_data->current_rx_state);
		priv_data->current_rx_state = next_rx_state;
		priv_data->next_rx_state = crypto_state_create();
	}
	if (state->enable_crypto_tx){
		crypto_state_destroy(priv_data->current_tx_state);
		priv_data->current_tx_state = next_tx_state;
		priv_data->next_tx_state = crypto_state_create();
	}
	return 0;
}
EXPORT_SYMBOL(default_sdup_update_crypto_state);

struct ps_base * sdup_crypto_ps_default_create(struct rina_component * component)
{
	struct auth_sdup_profile * conf;
	struct sdup_comp * sdup_comp;
	struct sdup_crypto_ps * ps;
	struct sdup_port * sdup_port;
	struct sdup_crypto_ps_default_data * data;
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

	if (conf->encrypt) {
		parameter = policy_param_find(conf->encrypt,
					      "seq_win_size");
		if (!parameter) {
			data->seq_win_size = 0;
		} else {
			aux = policy_param_value(parameter);
			if (kstrtoint(aux, 10, &data->seq_win_size)) {
				LOG_ERR("Problems copying 'seq_win_size' value");
				rkfree(ps);
				priv_data_destroy(data);
				return NULL;
			}

			data->seq_bmap_len = data->seq_win_size / 8;
			if (data->seq_win_size % 8) {
				data->seq_bmap_len += 1;
			}

			data->seq_bmap = rkzalloc(data->seq_bmap_len,
						  GFP_KERNEL);
			if (!data->seq_bmap) {
				LOG_ERR("Problems allocating sequence number window.");
				rkfree(ps);
				priv_data_destroy(data);
				return NULL;
			}

			LOG_DBG("Sequence number window size is %d",
				data->seq_win_size);
		}
	} else {
		LOG_ERR("Bogus configuration passed");
		rkfree(ps);
		priv_data_destroy(data);
		return NULL;
	}

	/* SDUP policy functions*/
	ps->sdup_apply_crypto		= default_sdup_apply_crypto;
	ps->sdup_remove_crypto		= default_sdup_remove_crypto;
	ps->sdup_update_crypto_state	= default_sdup_update_crypto_state;

	return &ps->base;
}
EXPORT_SYMBOL(sdup_crypto_ps_default_create);

void sdup_crypto_ps_default_destroy(struct ps_base * bps)
{
	struct sdup_crypto_ps_default_data * data;
	struct sdup_crypto_ps *ps;

	ps = container_of(bps, struct sdup_crypto_ps, base);
	data = ps->priv;

	if (bps) {
		if (data)
			priv_data_destroy(data);
		rkfree(ps);
	}
}
EXPORT_SYMBOL(sdup_crypto_ps_default_destroy);

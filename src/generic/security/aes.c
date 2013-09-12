/* @@@LICENSE
*
*      Copyright (c) 2013 LG Electronics, Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* LICENSE@@@ */

#include <security.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/obj_mac.h>

static const struct aes_algo_data_t
{
	const char *name;
	int keylen;
	nyx_security_aes_block_mode_t mode;
	const EVP_CIPHER *(*cipher_fun)(void);
} aes_algo_data[] =
{
	{ SN_aes_128_cbc, 128, NYX_SECURITY_AES_CBC, EVP_aes_128_cbc },
	{ SN_aes_256_cbc, 256, NYX_SECURITY_AES_CBC, EVP_aes_256_cbc }
};

static int aes_supported_keylength(int keylen)
{
	int i;

	for (i = 0; i < sizeof(aes_algo_data) / sizeof(aes_algo_data[0]); ++i)
	{
		if (aes_algo_data[i].keylen == keylen)
		{
			return 1;
		}
	}

	return 0;
}

static const struct aes_algo_data_t *aes_algo_data_lookup(int keylen,
        nyx_security_aes_block_mode_t mode)
{
	int i;

	for (i = 0; i < sizeof(aes_algo_data) / sizeof(aes_algo_data[0]); ++i)
	{
		if (aes_algo_data[i].keylen == keylen && aes_algo_data[i].mode == mode)
		{
			return &aes_algo_data[i];
		}
	}

	return NULL;
}

nyx_error_t aes_generate_key(int keylen, int *index)
{
	if (!aes_supported_keylength(keylen))
	{
		return NYX_ERROR_INVALID_VALUE;
	}

	struct aes_key_t *aes_key = g_malloc(sizeof(struct aes_key_t));

	if (aes_key == NULL)
	{
		return NYX_ERROR_OUT_OF_MEMORY;
	}

	aes_key->keylen = keylen;

	if (!RAND_bytes(aes_key->key, aes_key->keylen / 8))
	{
		goto error;
	}

	keystore_key_replace(keystore.aes, aes_key, index);

	return NYX_ERROR_NONE;

error:
	aes_destroy_key(aes_key);
	return NYX_ERROR_GENERIC;
}

nyx_error_t aes_crypt(int index, int encrypt,
                      nyx_security_aes_block_mode_t mode, const char *src, int srclen, char *dest,
                      int *destlen, int *ivlen)
{
	struct aes_key_t *aes_key = (struct aes_key_t *) keystore_key_lookup(
	                                keystore.aes, index);

	if (aes_key == NULL)
	{
		nyx_debug("%s: invalid key", __FUNCTION__);
		return NYX_ERROR_INVALID_VALUE;
	}

	const struct aes_algo_data_t *algo = aes_algo_data_lookup(aes_key->keylen,
	                                     mode);

	if (algo == NULL)
	{
		return NYX_ERROR_INVALID_VALUE;
	}

	nyx_error_t result = NYX_ERROR_NONE;

	/* IV saved at beginning of encryption buffer */
	unsigned char *iv = encrypt ? (unsigned char *)dest : (unsigned char *)src;

	if (encrypt)
	{
		*ivlen = AES_BLOCK_SIZE;

		/* skip IV */
		dest += AES_BLOCK_SIZE;

		/* generate IV */
		if (!RAND_bytes(iv, AES_BLOCK_SIZE))
		{
			return NYX_ERROR_GENERIC;
		}
	}
	else
	{
		g_assert(AES_BLOCK_SIZE == *ivlen);

		/* skip IV */
		src += *ivlen;
		srclen -= *ivlen;
	}

	EVP_CIPHER_CTX ctx;
	EVP_CIPHER_CTX_init(&ctx);
	EVP_CipherInit_ex(&ctx, algo->cipher_fun(), NULL, NULL, NULL, encrypt);
	EVP_CIPHER_CTX_set_key_length(&ctx, aes_key->keylen);
	EVP_CipherInit_ex(&ctx, NULL, NULL, aes_key->key, iv, encrypt);

	if (!EVP_CipherUpdate(&ctx, (unsigned char *)dest, destlen,
	                      (unsigned char *)src, srclen))
	{
		nyx_debug("EVP_CipherUpdate failed");
		ERR_print_errors_fp(stderr);
		result = NYX_ERROR_GENERIC;
		goto out;
	}

	int tmplen;

	if (!EVP_CipherFinal_ex(&ctx, (unsigned char *)dest + *destlen, &tmplen))
	{
		nyx_debug("EVP_CipherFinal_ex failed");
		ERR_print_errors_fp(stderr);
		result = NYX_ERROR_GENERIC;
		goto out;
	}

	*destlen += tmplen;

	if (encrypt)
	{
		*destlen += *ivlen;
	}

out:
	EVP_CIPHER_CTX_cleanup(&ctx);
	return result;
}


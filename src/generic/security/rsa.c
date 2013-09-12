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
#include <openssl/bn.h>
#include <openssl/err.h>

nyx_error_t rsa_generate_key(int keylen, int *key_index)
{
	switch (keylen)
	{
		case 2048:
		case 4096:
			break;

		default:
			return NYX_ERROR_INVALID_VALUE;
	}

	struct rsa_key_t *rsa_key = g_malloc(sizeof(struct rsa_key_t));

	if (rsa_key == NULL)
	{
		return NYX_ERROR_OUT_OF_MEMORY;
	}

	rsa_key->keylen = keylen;
	rsa_key->rsa = RSA_new();
	BIGNUM *bn = BN_new();
	BN_set_word(bn, RSA_F4);

	if (RSA_generate_key_ex(rsa_key->rsa, rsa_key->keylen, bn, NULL) == -1)
	{
		nyx_debug("RSA_generate_key_ex failed");
		ERR_print_errors_fp(stderr);
		goto error;
	}

	BN_free(bn);

	keystore_key_replace(keystore.rsa, rsa_key, key_index);

	return NYX_ERROR_NONE;

error:
	rsa_destroy_key(rsa_key);
	return NYX_ERROR_GENERIC;
}

nyx_error_t rsa_crypt(int key_index, int encrypt, const char *src, int srclen,
                      char *dest, int *destlen)
{
	struct rsa_key_t *rsa_key = (struct rsa_key_t *) keystore_key_lookup(
	                                keystore.rsa, key_index);

	if (rsa_key == NULL)
	{
		nyx_debug("%s: invalid key", __FUNCTION__);
		return NYX_ERROR_INVALID_VALUE;
	}

	if (encrypt)
	{
		/* RSA_PKCS1_OAEP_PADDING limit */
		if (srclen >= (RSA_size(rsa_key->rsa) - 41))
		{
			nyx_debug("src buffer too big");
			return NYX_ERROR_INVALID_VALUE;
		}
	}

	*destlen = RSA_size(rsa_key->rsa);

	int (*crypt_fun)(int, const unsigned char *, unsigned char *, RSA *, int) =
	    encrypt ? RSA_public_encrypt : RSA_private_decrypt;

	*destlen = crypt_fun(srclen, (unsigned char *)src, (unsigned char *)dest,
	                     rsa_key->rsa, RSA_PKCS1_OAEP_PADDING);

	if (*destlen == -1)
	{
		nyx_debug("RSA crypt failed");
		ERR_print_errors_fp(stderr);
		return NYX_ERROR_GENERIC;
	}

	return NYX_ERROR_NONE;
}


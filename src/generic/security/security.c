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

/*
********************************************************************************
* @file security.c
*
* @brief The SECURITY module implementation.
********************************************************************************
*/

#include <security.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/err.h>

NYX_DECLARE_MODULE(NYX_DEVICE_SECURITY, "Security");

struct keystore_t keystore;

nyx_error_t nyx_module_open(nyx_instance_t i, nyx_device_t **d)
{
	if ((NULL == d) || (NULL != *d))
	{
		return NYX_ERROR_INVALID_VALUE;
	}

	*d = (nyx_device_t *) calloc(sizeof(nyx_device_t), 1);

	if (NULL == *d)
	{
		return NYX_ERROR_OUT_OF_MEMORY;
	}

	/* register methods */
	struct
	{
		module_method_t method;
		const char *function;
	} methods[] =
	{
		{ NYX_SECURITY_CREATE_AES_KEY_MODULE_METHOD, "security_create_aes_key" },
		{ NYX_SECURITY_CREATE_RSA_KEY_MODULE_METHOD, "security_create_rsa_key" },
		{ NYX_SECURITY_CRYPT_AES_MODULE_METHOD,      "security_aes_crypt" },
		{ NYX_SECURITY_CRYPT_RSA_MODULE_METHOD,      "security_rsa_crypt" },
		{ NYX_SECURITY_INIT_HASH_MODULE_METHOD,      "security_init_hash" },
		{ NYX_SECURITY_UPDATE_HASH_MODULE_METHOD,    "security_update_hash" },
		{ NYX_SECURITY_FINALIZE_HASH_MODULE_METHOD,  "security_finalize_hash" },
	};

	int m;

	for (m = 0; m < sizeof(methods) / sizeof(methods[0]); ++m)
	{
		nyx_error_t result = nyx_module_register_method(i, *d, methods[m].method,
		                     methods[m].function);

		if (result != NYX_ERROR_NONE)
		{
			return result;
		}
	}

	ERR_load_crypto_strings();

	/* initialize keystore */
	nyx_error_t result = keystore_init(&keystore);

	if (result == NYX_ERROR_NONE)
	{
		result = keystore_load(&keystore);
	}

	if (result != NYX_ERROR_NONE)
	{
		keystore_destroy(&keystore);
		free(*d);
	}

	return result;
}

nyx_error_t nyx_module_close(nyx_device_handle_t d)
{
	/* dump keystore for debugging */
	keystore_dump(&keystore);
	keystore_save(&keystore);
	keystore_destroy(&keystore);
	ERR_free_strings();
	free(d);

	return NYX_ERROR_NONE;
}

nyx_error_t security_create_aes_key(nyx_device_handle_t d, int keylen,
                                    int *key_index)
{
	return aes_generate_key(keylen, key_index);
}

nyx_error_t security_aes_crypt(nyx_device_handle_t d, int key_index,
                               nyx_security_aes_block_mode_t mode, int encrypt, const char *src, int srclen,
                               char *dest, int *destlen, int *ivlen)
{
	return aes_crypt(key_index, encrypt, mode, src, srclen, dest, destlen, ivlen);
}

nyx_error_t security_create_rsa_key(nyx_device_handle_t d, int keylen,
                                    int *key_index)
{
	return rsa_generate_key(keylen, key_index);
}

nyx_error_t security_rsa_crypt(nyx_device_handle_t d, int key_index,
                               int encrypt, const char *src, int srclen, char *dest, int *destlen)
{
	return rsa_crypt(key_index, encrypt, src, srclen, dest, destlen);
}

nyx_error_t security_init_hash(nyx_device_handle_t d, const char *hash_algo)
{
	return sha_init(hash_algo);
}

nyx_error_t security_update_hash(nyx_device_handle_t d, const char *src,
                                 int srclen)
{
	return sha_update(src, srclen);
}

nyx_error_t security_finalize_hash(nyx_device_handle_t d, char *dest)
{
	int destlen = -1;
	nyx_error_t result = sha_finalize(dest, &destlen);

	if (result == NYX_ERROR_NONE)
	{
		/* base64 encode hash */
		gchar *b64 = g_base64_encode((const guchar *)dest, destlen);
		memcpy(dest, b64, strlen(b64) + 1);
		g_free(b64);
	}

	return result;
}

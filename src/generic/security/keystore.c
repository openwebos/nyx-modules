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
#include <string.h>
#include <openssl/pem.h>

void aes_destroy_key(gpointer p)
{
	struct aes_key_t *key = (struct aes_key_t *) p;
	g_free(key);
}

void rsa_destroy_key(gpointer p)
{
	struct rsa_key_t *key = (struct rsa_key_t *) p;

	if (key != NULL)
	{
		RSA_free(key->rsa);
	}

	g_free(key);
}

int keystore_init(struct keystore_t *store)
{
	store->aes = g_hash_table_new_full(NULL, NULL, NULL, aes_destroy_key);

	if (store->aes == NULL)
	{
		return NYX_ERROR_OUT_OF_MEMORY;
	}

	store->rsa = g_hash_table_new_full(NULL, NULL, NULL, rsa_destroy_key);

	if (store->rsa == NULL)
	{
		return NYX_ERROR_OUT_OF_MEMORY;
	}

	return NYX_ERROR_NONE;
}

void keystore_destroy(struct keystore_t *store)
{
	g_hash_table_destroy(store->aes);
	g_hash_table_destroy(store->rsa);
}

gpointer keystore_key_lookup(GHashTable *keys, int index)
{
	return g_hash_table_lookup(keys, GINT_TO_POINTER(index));
}

void keystore_key_replace(GHashTable *keys, gpointer key, int *index)
{
	if (*index == -1)
	{
		/* client don't care where to put key, find next free index */
		int i;

		for (i = 0; g_hash_table_lookup(keys, GINT_TO_POINTER(i)) != NULL; ++i);

		*index = i;
	}

	g_hash_table_replace(keys, GINT_TO_POINTER(*index), key);
}

/* key store file format
 *
 * Plain key-value file using GKeyFile
 *
 * [aes]
 * <index>=<keylen>;<base64_encoded_key>
 * [rsa]
 * <index>=<keylen>;<base64_encoded_asn1_rsa_key>
 */

void keystore_save_aeskey(gpointer key, gpointer value, gpointer user_data)
{
	const int key_index = GPOINTER_TO_INT(key);
	const struct aes_key_t *aes_key = (const struct aes_key_t *)value;
	GKeyFile *key_file = (GKeyFile *)user_data;

	g_assert(aes_key != NULL);
	g_assert(key_file != NULL);

	gchar *index = g_strdup_printf("%d", key_index);
	const gchar *const list[] =
	{
		g_strdup_printf("%d", aes_key->keylen),
		g_base64_encode(aes_key->key, aes_key->keylen / 8)
	};

	g_key_file_set_string_list(key_file, "aes", index, list, 2);

	g_free(index);
	g_free((gpointer)list[0]);
	g_free((gpointer)list[1]);
}

static void keystore_load_aeskey(GKeyFile *keyfile, gchar *key,
                                 struct keystore_t *keystore)
{
	g_assert(NULL != keyfile);
	g_assert(NULL != key);
	g_assert(NULL != keystore);
	gsize length = 0;
	gchar **list = g_key_file_get_string_list(keyfile, "aes", key, &length, NULL);

	if (length == 2)
	{
		int key_index = atoi(key);
		int keylen = atoi(list[0]);
		gsize keybits_len = 0;
		guchar *keybits = g_base64_decode(list[1], &keybits_len);
		g_assert(keylen == keybits_len * 8);

		struct aes_key_t *aes_key = g_malloc(sizeof(struct aes_key_t));
		aes_key->keylen = keylen;
		memcpy(aes_key->key, keybits, keybits_len);

		keystore_key_replace(keystore->aes, aes_key, &key_index);

		g_free(keybits);
	}

	g_strfreev(list);
}

static void keystore_save_rsakey(gpointer key, gpointer value,
                                 gpointer user_data)
{
	const int key_index = GPOINTER_TO_INT(key);
	const struct rsa_key_t *rsa_key = (const struct rsa_key_t *)value;
	GKeyFile *key_file = (GKeyFile *)user_data;

	g_assert(rsa_key != NULL);
	g_assert(key_file != NULL);

	gchar *index = g_strdup_printf("%d", key_index);
	int pkcs_len = i2d_RSAPrivateKey(rsa_key->rsa, NULL);
	unsigned char *pkcs_buf = malloc(pkcs_len);
	unsigned char *tmp = pkcs_buf;
	i2d_RSAPrivateKey(rsa_key->rsa, &tmp);

	const gchar *const list[] =
	{
		g_strdup_printf("%d", rsa_key->keylen),
		g_base64_encode(pkcs_buf, pkcs_len),
	};

	g_key_file_set_string_list(key_file, "rsa", index, list, 2);

	g_free(index);
	free(pkcs_buf);
	g_free((gpointer)list[0]);
	g_free((gpointer)list[1]);
}

static void keystore_load_rsakey(GKeyFile *keyfile, gchar *key,
                                 struct keystore_t *keystore)
{
	g_assert(NULL != keyfile);
	g_assert(NULL != key);
	g_assert(NULL != keystore);
	gsize length = 0;
	gchar **list = g_key_file_get_string_list(keyfile, "rsa", key, &length, NULL);

	if (length == 2)
	{
		int key_index = atoi(key);
		int keylen = atoi(list[0]);
		gsize keybits_len = 0;
		guchar *keybits = g_base64_decode(list[1], &keybits_len);
		const guchar *tmp = keybits;

		struct rsa_key_t *rsa_key = g_malloc(sizeof(struct rsa_key_t));
		rsa_key->keylen = keylen;
		rsa_key->rsa = d2i_RSAPrivateKey(NULL, (const unsigned char **)&tmp,
		                                 keybits_len);

		if (NULL != rsa_key->rsa)
		{
			keystore_key_replace(keystore->rsa, rsa_key, &key_index);
		}

		g_free(keybits);
	}

	g_strfreev(list);
}

nyx_error_t keystore_load(struct keystore_t *keystore)
{
	if (FALSE == g_file_test(SECURITY_KEYSTORE_PATH, G_FILE_TEST_EXISTS))
	{
		return NYX_ERROR_NONE;
	}

	GKeyFile *keyfile = g_key_file_new();
	GKeyFileFlags flags = G_KEY_FILE_NONE;

	if (!g_key_file_load_from_file(keyfile, SECURITY_KEYSTORE_PATH, flags, NULL))
	{
		nyx_debug("%s: g_key_file_load_from_file error", __FUNCTION__);
		g_key_file_free(keyfile);
		return NYX_ERROR_GENERIC;
	}

	int i;

	/* load aes keys */
	gchar **aes_keys = g_key_file_get_keys(keyfile, "aes", NULL, NULL);

	if (NULL != aes_keys)
	{
		for (i = 0; aes_keys[i] != NULL; ++i)
		{
			keystore_load_aeskey(keyfile, aes_keys[i], keystore);
		}

		g_strfreev(aes_keys);
	}

	/* load rsa keys */
	gchar **rsa_keys = g_key_file_get_keys(keyfile, "rsa", NULL, NULL);

	if (NULL != rsa_keys)
	{
		for (i = 0; rsa_keys[i] != NULL; ++i)
		{
			keystore_load_rsakey(keyfile, rsa_keys[i], keystore);
		}

		g_strfreev(rsa_keys);
	}

	g_key_file_free(keyfile);

	return NYX_ERROR_NONE;
}

void keystore_save(struct keystore_t *keystore)
{
	g_assert(keystore);

	GKeyFile *keyfile = g_key_file_new();

	if (NULL != keystore->aes)
	{
		g_hash_table_foreach(keystore->aes, keystore_save_aeskey, keyfile);
	}

	if (NULL != keystore->rsa)
	{
		g_hash_table_foreach(keystore->rsa, keystore_save_rsakey, keyfile);
	}

	/* save to disk */

	if (0 == g_mkdir_with_parents(SECURITY_KEYSTORE_DIR, 0700))
	{
		gsize length = 0;
		gchar *keystore_data = g_key_file_to_data(keyfile, &length, NULL);

		if (!g_file_set_contents(SECURITY_KEYSTORE_PATH, keystore_data, length, NULL))
		{
			nyx_debug("%s: g_file_set_contents error", __FUNCTION__);
		}

		g_free(keystore_data);
	}

	g_key_file_free(keyfile);
}

static void aeskey_hexdump(gpointer key, gpointer value, gpointer user_data)
{
	struct aes_key_t *aes_key = (struct aes_key_t *)value;
	int i;
	/* HACK: dump key for openssl verification
	 * e.g. openssl aes-<keylen>-cbc -in <encoded.bin> -K <key> -iv <iv> -d
	 * delete when keys stored to file */
	printf("AES: keylen: %d, key: ", aes_key->keylen);

	for (i = 0; i < aes_key->keylen / 8; ++i)
	{
		printf("%02X", aes_key->key[i]);
	}

	printf("\n");
}

static void rsakey_hexdump(gpointer key, gpointer value, gpointer user_data)
{
	struct rsa_key_t *rsa_key = (struct rsa_key_t *)value;
	/* HACK: dump key for openssl verification
	 * e.g. echo -n <something> | openssl rsautl -in <encoded.bin> -inkey <privkey>  -oaep -d
	 * delete when keys stored to file */
	printf("RSA: keylen: %d, privkey:\n", rsa_key->keylen);
	BIO *bio = BIO_new_fp(stdout, BIO_NOCLOSE);
	PEM_write_bio_RSAPrivateKey(bio, rsa_key->rsa, NULL, NULL, 0, NULL, NULL);
	BIO_free(bio);
	printf("\n");
}

void keystore_dump(struct keystore_t *keystore)
{
	g_hash_table_foreach(keystore->aes, aeskey_hexdump, NULL);
	g_hash_table_foreach(keystore->rsa, rsakey_hexdump, NULL);
}

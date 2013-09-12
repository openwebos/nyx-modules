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

#include <nyx/nyx_client.h>
#include <assert.h>
#include <stdio.h>
#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/obj_mac.h>
#include <openssl/evp.h>

struct Fixture
{
	nyx_device_handle_t device;
};

static void fixture_setup(struct Fixture *f, gconstpointer userdata)
{
	g_assert(NYX_ERROR_NONE == nyx_init());

	f->device = NULL;
	g_assert_cmpint(NYX_ERROR_NONE, == , nyx_device_open(NYX_DEVICE_SECURITY,
	                "Main", &f->device));
	g_assert(f->device != NULL);
}

static void fixture_teardown(struct Fixture *f, gconstpointer userdata)
{
	g_assert_cmpint(NYX_ERROR_NONE, == , nyx_device_close(f->device));
	g_assert_cmpint(NYX_ERROR_NONE, == , nyx_deinit());
}

static void test_create_aes_key(struct Fixture *f, gconstpointer userdata)
{
	g_assert(userdata != NULL); /* key length as userdata pointer */

	const int keylen = atoi((const char *)userdata);
	int index = -1;
	g_assert_cmpint(NYX_ERROR_NONE, == , nyx_security_create_aes_key(f->device,
	                keylen, &index));
	g_assert_cmpint(index, != , -1); /* module assigned next (first) free index */

	index = 1;
	g_assert_cmpint(NYX_ERROR_NONE, == , nyx_security_create_aes_key(f->device,
	                keylen, &index));
	g_assert_cmpint(index, == , 1); /* module must not change index */

	index = -1;
	g_assert_cmpint(NYX_ERROR_NONE, == , nyx_security_create_aes_key(f->device,
	                keylen, &index));
	g_assert_cmpint(index, >= , 2); /* index 0,1 assigned */
}

static void test_create_rsa_key(struct Fixture *f, gconstpointer userdata)
{
	g_assert(userdata != NULL); /* key length as userdata pointer */

	const int keylen = atoi((const char *)userdata);
	int index = -1;

	g_assert_cmpint(NYX_ERROR_NONE, == , nyx_security_create_rsa_key(f->device,
	                keylen, &index));
	g_assert_cmpint(index, != , -1); /* module assigned next (first) free index */

	index = 1;
	g_assert_cmpint(NYX_ERROR_NONE, == , nyx_security_create_rsa_key(f->device,
	                keylen, &index));
	g_assert_cmpint(index, == , 1); /* module must not change index */

	index = -1;
	g_assert_cmpint(NYX_ERROR_NONE, == , nyx_security_create_rsa_key(f->device,
	                keylen, &index));
	g_assert_cmpint(index, >= , 2); /* index 0,1 assigned */
}

static void test_crypt_aes(struct Fixture *f, gconstpointer userdata)
{
	nyx_security_aes_block_mode_t block_mode = (nyx_security_aes_block_mode_t)
	        GPOINTER_TO_INT(userdata);
	int index = -1;
	g_assert_cmpint(NYX_ERROR_NONE, == , nyx_security_create_aes_key(f->device, 128,
	                &index));

	const char *src = "foobar";
	char enc[sizeof(src) + EVP_MAX_BLOCK_LENGTH + EVP_MAX_IV_LENGTH];
	char dec[sizeof(src) + EVP_MAX_BLOCK_LENGTH + EVP_MAX_IV_LENGTH];
	int enclen = -1;
	int declen = -1;
	int ivlen = 0;

	g_assert_cmpint(NYX_ERROR_NONE, == , nyx_security_crypt_aes(f->device, index,
	                block_mode, 1, src, strlen(src) + 1, enc, &enclen, &ivlen));
	g_assert_cmpint(enclen, > , 0);

	g_assert_cmpint(NYX_ERROR_NONE, == , nyx_security_crypt_aes(f->device, index,
	                block_mode, 0, enc, enclen, dec, &declen, &ivlen));
	g_assert_cmpint(declen, > , 0);
	g_assert_cmpstr(src, == , dec);
}

static void test_crypt_rsa(struct Fixture *f, gconstpointer userdata)
{
	const int keylen = atoi((const char *)userdata);
	int index = -1;

	g_assert_cmpint(keylen % 8, == , 0);

	g_assert_cmpint(NYX_ERROR_NONE, == , nyx_security_create_rsa_key(f->device,
	                keylen, &index));

	const char *src = "foobar";
	char enc[keylen / 8];
	char dec[keylen / 8];
	int enclen = -1;
	int declen = -1;

	g_assert_cmpint(NYX_ERROR_NONE, == , nyx_security_crypt_rsa(f->device, index, 1,
	                src, strlen(src) + 1, enc, &enclen));
	g_assert_cmpint(enclen, > , 0);

	g_assert_cmpint(NYX_ERROR_NONE, == , nyx_security_crypt_rsa(f->device, index, 0,
	                enc, enclen, dec, &declen));
	g_assert_cmpint(declen, > , 0);
	g_assert_cmpstr(src, == , dec);
}

static void test_calculate_sha(struct Fixture *f, gconstpointer userdata)
{
	g_assert(userdata !=
	         NULL); /* "hash_type;expected_result" as user data pointer */

	gchar **tokens = g_strsplit((const char *)userdata, ";", 2);
	g_assert(tokens != NULL && tokens[0] != NULL && tokens[1] != NULL);

	char hash[EVP_MAX_MD_SIZE * 2] = {0};
	const char *msg = "foobar";

	g_assert_cmpint(NYX_ERROR_NONE, == , nyx_security_init_hash(f->device,
	                tokens[0]));
	g_assert_cmpint(NYX_ERROR_NONE, == , nyx_security_update_hash(f->device, msg,
	                strlen(msg)));
	g_assert_cmpint(NYX_ERROR_NONE, == , nyx_security_finalize_hash(f->device,
	                hash));
	/* echo -n foobar | openssl dgst -sha{256,512} | openssl base64 -e */
	g_assert_cmpstr(hash, == , tokens[1]);

	g_strfreev(tokens);
}

#define TEST_ADD(path, func, data) \
    g_test_add(path, struct Fixture, data, fixture_setup, func, fixture_teardown)

int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);

	TEST_ADD("/nyx/security/create_key_aes128cbc", test_create_aes_key,
	         "128");
	TEST_ADD("/nyx/security/create_key_aes256cbc", test_create_aes_key,
	         "256");
	TEST_ADD("/nyx/security/create_key_rsa2048", test_create_rsa_key,
	         "2048");

	TEST_ADD("/nyx/security/crypt_aes128cbc", test_crypt_aes,
	         GINT_TO_POINTER(NYX_SECURITY_AES_CBC));
	TEST_ADD("/nyx/security/crypt_rsa2048", test_crypt_rsa,
	         "2048");
	TEST_ADD("/nyx/security/crypt_rsa2048", test_crypt_rsa,
	         "4096");

	TEST_ADD("/nyx/security/calculate_sha256", test_calculate_sha,
	         SN_sha256";w6uP8Tcg6K2QR905Rms8iXTlksL6OD1KOWBxTK7wxPI=");
	TEST_ADD("/nyx/security/calculate_sha512", test_calculate_sha,
	         SN_sha512";ClAmHr0aOQ/tK/Mm8mc8FFWCpjQtUjIElz0CGTN/gWFqgGmwElh89WNfaSXxtWw2AjDBmyc1AO4BPgMGAb8kJQ==");

	return g_test_run();
}


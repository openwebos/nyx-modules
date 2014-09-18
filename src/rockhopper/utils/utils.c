/* @@@LICENSE
*
*      Copyright (c) 2014 LG Electronics, Inc.
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

/**
* @file utils.c
*
* @brief Common methods to read values from a file, evaluate sysfs path, etc
*
*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <glib.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>

#include <nyx/module/nyx_log.h>

/**
 * Returns string in pre-allocated buffer.
 */

int FileGetString(const char *path, char *ret_string, size_t maxlen)
{
	GError *gerror = NULL;
	char *contents = NULL;
	gsize len;

	if (!path || !g_file_get_contents(path, &contents, &len, &gerror))
	{
		if (gerror)
		{
			nyx_error( "%s: %s", __FUNCTION__, gerror->message);
			g_error_free(gerror);
		}
		return -1;
	}

	g_strstrip(contents);
	g_strlcpy(ret_string, contents, maxlen);
	g_free(contents);

	return 0;
}

int FileGetDouble(const char *path, double *ret_data)
{
	GError *gerror = NULL;
	char *contents = NULL;
	char *endptr;
	gsize len;
	float val;

	if (!path || !g_file_get_contents(path, &contents, &len, &gerror))
	{
		if (gerror)
		{
		nyx_error( "%s: %s", __FUNCTION__, gerror->message);
		g_error_free(gerror);
		}
		return -1;
	}

	val = strtod(contents, &endptr);
	if (endptr == contents)
	{
		nyx_error( "%s: Invalid input in %s.", __FUNCTION__, path);
		goto end;
	}

	if (ret_data)
	{
		*ret_data = val;
	}

end:
	g_free(contents);
	return 0;
}

char* find_power_supply_sysfs_path(const char *device_type)
{
	GError *gerror = NULL;
	GDir* dir;
	GDir* subdir;
	gchar* dir_path = NULL;
	gchar* full_path = NULL;
	const char *sub_dir_name;
	const char *file_name;
	char file_contents[64];
	char base_dir[64] = "/sys/class/power_supply/";

	dir = g_dir_open(base_dir, 0, &gerror);
	if (gerror)
	{
		nyx_error( "%s: %s", __FUNCTION__, gerror->message);
		g_error_free(gerror);
		return NULL;
	}

	while (sub_dir_name = g_dir_read_name(dir))
	{
		// ignore hidden files
		if ('.' == sub_dir_name[0])
		{
			continue;
		}
		dir_path = g_build_filename(base_dir, sub_dir_name, NULL);
		if (g_file_test(dir_path, G_FILE_TEST_IS_DIR))
		{
			subdir = g_dir_open(dir_path, 0, &gerror);
			if (gerror)
			{
				nyx_error( "%s: %s", __FUNCTION__, gerror->message);
				g_error_free(gerror);
				return NULL;
			}

			while (file_name = g_dir_read_name(subdir))
			{
				if (strcmp(file_name, "type") == 0)
				{
					full_path = g_build_filename(dir_path, file_name, NULL);
					FileGetString(full_path, file_contents, 64);
					if (strcmp(file_contents, device_type) == 0)
					{
						return dir_path;
					}
					g_free(full_path);
					full_path = NULL;
				}
			}
		}
		g_free(dir_path);
		dir_path = NULL;
	}

	return NULL;
}

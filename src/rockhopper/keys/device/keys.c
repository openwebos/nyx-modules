/* @@@LICENSE
*
*      Copyright (c) 2010-2012 Hewlett-Packard Development Company, L.P.
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

#include <stdbool.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>
#include <errno.h>
#include <poll.h>
#include <glib.h>
#include <stdio.h>

#include <nyx/nyx_module.h>

#include "../keys_common.h"

enum {
    VOLUME_DOWN,
    VOLUME_UP,
    POWER
};

int lookup_key(keys_device_t* d, uint16_t keyCode, int32_t keyValue,
        nyx_key_type_t* key_type_out_ptr)
{
    int key = 0;
    switch (keyCode) {
    case KEY_HOME:
        key = NYX_KEYS_CUSTOM_KEY_HOME;
        *key_type_out_ptr = NYX_KEY_TYPE_CUSTOM;
        break;
    case KEY_POWER:
        key = NYX_KEYS_CUSTOM_KEY_POWER_ON;
        *key_type_out_ptr = NYX_KEY_TYPE_CUSTOM;
        break;
    case KEY_VOLUMEUP:
        key = NYX_KEYS_CUSTOM_KEY_VOL_UP;
        *key_type_out_ptr = NYX_KEY_TYPE_CUSTOM;
        break;
    case KEY_VOLUMEDOWN:
        key = NYX_KEYS_CUSTOM_KEY_VOL_DOWN;
        *key_type_out_ptr = NYX_KEY_TYPE_CUSTOM;
        break;
    default:
        break;
    }

    return key;
}

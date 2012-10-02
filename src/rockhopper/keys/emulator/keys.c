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
    F1 = 0x276C, /* Function keys */
    F2 = 0x276D,
    F3 = 0x276E,
    F4 = 0x276F,
    F5 = 0x2770,
    F6 = 0x2771,
    F7 = 0x2772,
    F8 = 0x2773,
    F9 = 0x2774,
    F10 = 0x2775,
    KEY_SYM = 0xf6,
    KEY_ORANGE = 0x64
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
    case KEY_VOLUMEUP:
        key = NYX_KEYS_CUSTOM_KEY_VOL_UP;
        *key_type_out_ptr = NYX_KEY_TYPE_CUSTOM;
        break;
    case KEY_VOLUMEDOWN:
        key = NYX_KEYS_CUSTOM_KEY_VOL_DOWN;
        *key_type_out_ptr = NYX_KEY_TYPE_CUSTOM;
        break;
    case KEY_END:
        key = NYX_KEYS_CUSTOM_KEY_POWER_ON;
        *key_type_out_ptr = NYX_KEY_TYPE_CUSTOM;
        break;
    case KEY_PLAY:
        key = NYX_KEYS_CUSTOM_KEY_MEDIA_PLAY;
        *key_type_out_ptr = NYX_KEY_TYPE_CUSTOM;
        break;
    case KEY_PAUSE:
        key = NYX_KEYS_CUSTOM_KEY_MEDIA_PAUSE;
        *key_type_out_ptr = NYX_KEY_TYPE_CUSTOM;
        break;
    case KEY_STOP:
        key = NYX_KEYS_CUSTOM_KEY_MEDIA_STOP;
        *key_type_out_ptr = NYX_KEY_TYPE_CUSTOM;
        break;
    case KEY_NEXT:
        key = NYX_KEYS_CUSTOM_KEY_MEDIA_NEXT;
        *key_type_out_ptr = NYX_KEY_TYPE_CUSTOM;
        break;
    case KEY_PREVIOUS:
        key = NYX_KEYS_CUSTOM_KEY_MEDIA_PREVIOUS;
        *key_type_out_ptr = NYX_KEY_TYPE_CUSTOM;
        break;
        // add keyboard function keys
    case KEY_SEARCH:
        key = NYX_KEYS_CUSTOM_KEY_SEARCH;
        *key_type_out_ptr = NYX_KEY_TYPE_CUSTOM;
        break;
    case KEY_BRIGHTNESSDOWN:
        key = NYX_KEYS_CUSTOM_KEY_BRIGHTNESS_DOWN;
        *key_type_out_ptr = NYX_KEY_TYPE_CUSTOM;
        break;
    case KEY_BRIGHTNESSUP:
        key = NYX_KEYS_CUSTOM_KEY_BRIGHTNESS_UP;
        *key_type_out_ptr = NYX_KEY_TYPE_CUSTOM;
        break;
    case KEY_MUTE:
        key = NYX_KEYS_CUSTOM_KEY_VOL_MUTE;
        *key_type_out_ptr = NYX_KEY_TYPE_CUSTOM;
        break;
    case KEY_REWIND:
        key = NYX_KEYS_CUSTOM_KEY_MEDIA_REWIND;
        *key_type_out_ptr = NYX_KEY_TYPE_CUSTOM;
        break;
    case KEY_FASTFORWARD:
        key = NYX_KEYS_CUSTOM_KEY_MEDIA_FASTFORWARD;
        *key_type_out_ptr = NYX_KEY_TYPE_CUSTOM;
        break;

    default:
        break;
    }

    if (*key_type_out_ptr != NYX_KEY_TYPE_CUSTOM) {
        if (keyCode == KEY_LEFTSHIFT)
            key = KEY_LEFTSHIFT;
        else
            key = keyCode;
    }

    return key;
}

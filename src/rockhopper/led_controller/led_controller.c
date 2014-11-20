/* @@@LICENSE
*
*      Copyright (c) 2010-2012 Hewlett-Packard Development Company, L.P.
*      Copyright (c) 2012 Simon Busch <morphis@gravedo.de>
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

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <glib.h>

#include <nyx/nyx_module.h>
#include <nyx/module/nyx_utils.h>

NYX_DECLARE_MODULE(NYX_DEVICE_LED_CONTROLLER, "LedControllers");

nyx_error_t nyx_module_open (nyx_instance_t i, nyx_device_t** d)
{
    nyx_device_t *nyxDev = (nyx_device_t*)calloc(sizeof(nyx_device_t), 1);
    if (NULL == nyxDev)
        return NYX_ERROR_OUT_OF_MEMORY;

    nyx_module_register_method(i, (nyx_device_t*)nyxDev, NYX_LED_CONTROLLER_EXECUTE_EFFECT_MODULE_METHOD,
        "led_controller_execute_effect");

    nyx_module_register_method(i, (nyx_device_t*)nyxDev, NYX_LED_CONTROLLER_GET_STATE_MODULE_METHOD,
        "led_controller_get_state");

    *d = (nyx_device_t*)nyxDev;

    return NYX_ERROR_NONE;
}

nyx_error_t nyx_module_close (nyx_device_t* d)
{
    free(d);
    return NYX_ERROR_NONE;
}

static int FileGetInt(const char *path, int *ret_data)
{
    GError *gerror = NULL;
    char *contents = NULL;
    char *endptr;
    gsize len;
    long int val;

    if (!path || !g_file_get_contents(path, &contents, &len, &gerror)) {
        if (gerror) {
            nyx_critical( "%s: %s", __FUNCTION__, gerror->message);
            g_error_free(gerror);
        }
        return -1;
    }

    val = strtol(contents, &endptr, 10);
    if (endptr == contents) {
        nyx_critical( "%s: Invalid input in %s.",
            __FUNCTION__, path);
        goto end;
    }

    if (ret_data)
        *ret_data = val;
end:
    g_free(contents);
    return 0;
}

static int FileWriteInt(const char *path, int value)
{
    FILE *fp;

    fp = fopen(path, "w");
    if (fp == NULL)
        return -1;

    fprintf(fp, "%i", value);
    fclose(fp);

    return 0;
}

static nyx_error_t handle_backlight_effect(nyx_device_handle_t handle, nyx_led_controller_effect_t effect)
{
    int max_brightness;
    int value, power;

    switch(effect.required.effect)
    {
    case NYX_LED_CONTROLLER_EFFECT_LED_SET:
        if (FileGetInt(DISPLAY_SYSFS_PATH "bl_power", &power) < 0)
            return NYX_ERROR_DEVICE_UNAVAILABLE;

        if (FileGetInt(DISPLAY_SYSFS_PATH "max_brightness", &max_brightness) < 0)
            return NYX_ERROR_DEVICE_UNAVAILABLE;

        value = (int)(100.0 / max_brightness * effect.backlight.brightness_lcd);

        if (power == 0 && value > 0)
        {
            /* activate display when it's powered off and we want to set some brightness
             * greater than zero */
            if (FileWriteInt(DISPLAY_SYSFS_PATH "bl_power", 1) < 0)
                return NYX_ERROR_DEVICE_UNAVAILABLE;
        }
        else if (power== 1 && value <= 0)
        {
            /* Otherwise deactivate display when brightness should be set to zero */
            if (FileWriteInt(DISPLAY_SYSFS_PATH "bl_power", 0) < 0)
                return NYX_ERROR_DEVICE_UNAVAILABLE;
        }

        if (FileWriteInt(DISPLAY_SYSFS_PATH "brightness", value) < 0)
            return NYX_ERROR_DEVICE_UNAVAILABLE;

        break;
    default:
        break;
    }

    effect.backlight.callback(handle, NYX_CALLBACK_STATUS_DONE,
                                  effect.backlight.callback_context);

    return NYX_ERROR_NONE;
}

nyx_error_t led_controller_execute_effect(nyx_device_handle_t handle, nyx_led_controller_effect_t effect)
{
    switch (effect.required.led) {
    case NYX_LED_CONTROLLER_BACKLIGHT_LEDS:
        return handle_backlight_effect(handle, effect);
    default:
        break;
    }

    return NYX_ERROR_DEVICE_UNAVAILABLE;
}

nyx_error_t led_controller_get_state(nyx_device_handle_t handle, nyx_led_controller_led_t led, nyx_led_controller_state_t *state)
{
    int power = 0;

    switch (led) {
    case NYX_LED_CONTROLLER_BACKLIGHT_LEDS:
        if (FileGetInt(DISPLAY_SYSFS_PATH "bl_power", &power) < 0)
            return NYX_ERROR_DEVICE_UNAVAILABLE;

        *state = power ? NYX_LED_CONTROLLER_STATE_ON : NYX_LED_CONTROLLER_STATE_OFF;

        return NYX_ERROR_NONE;
    default:
        break;
    }

    return NYX_ERROR_DEVICE_UNAVAILABLE;
}

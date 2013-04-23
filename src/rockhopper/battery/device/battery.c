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


/**
* @file fake_battery.c
*
* @brief Interface for reading all the battery registers in case of qemux86 or using fake batteries.
*
*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <glib.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>

#include "batterylib.h"
#include "battery_read.h"
#include "fileutils.h"

#include <nyx/module/nyx_log.h>

#include <glib.h>
#include <libudev.h>

#define LOG_DOMAIN "common_linux_battery: "

#define CHARGE_MIN_TEMPERATURE_C 0
#define CHARGE_MAX_TEMPERATURE_C 57
#define BATTERY_MAX_TEMPERATURE_C  60

nyx_battery_ctia_t battery_ctia_params;

GIOChannel *channel;

struct udev *udev;
struct udev_monitor *mon;

extern nyx_device_t *nyxDev;
extern void *battery_callback_context;
extern nyx_device_callback_function_t battery_callback;

nyx_battery_ctia_t *get_battery_ctia_params(void)
{
    battery_ctia_params.charge_min_temp_c=0;
    battery_ctia_params.charge_max_temp_c=CHARGE_MAX_TEMPERATURE_C;
    battery_ctia_params.battery_crit_max_temp=BATTERY_MAX_TEMPERATURE_C;
    battery_ctia_params.skip_battery_authentication=true;

    return &battery_ctia_params;
}

/**
 * @brief Read battery percentage
 *
 * @retval Battery percentage (integer)
 */
int battery_percent(void)
{
    int now, full;
    int capacity;

    /* try capacity node first but keep in mind it's not supported by all power class
     * devices */
    if (!g_file_test(BATTERY_SYSFS_PATH "capacity", G_FILE_TEST_EXISTS) ||
        FileGetInt(BATTERY_SYSFS_PATH "capacity", &capacity) < 0)
    {
        /* capacity node is not available so next try is energy_now/energy_full */
        if (g_file_test(BATTERY_SYSFS_PATH "energy_now", G_FILE_TEST_EXISTS) &&
            g_file_test(BATTERY_SYSFS_PATH "energy_full", G_FILE_TEST_EXISTS))
        {
            if (FileGetInt(BATTERY_SYSFS_PATH "energy_now", &now) < 0)
                return -1;

            if (FileGetInt(BATTERY_SYSFS_PATH "energy_full", &full) < 0)
                return -1;

            capacity = (now / full);
        }
        /* as last try we can use charge_full or charge_now */
        else if (g_file_test(BATTERY_SYSFS_PATH "charge_full", G_FILE_TEST_EXISTS) &&
                 g_file_test(BATTERY_SYSFS_PATH "charge_now", G_FILE_TEST_EXISTS))
        {
            if (FileGetInt(BATTERY_SYSFS_PATH "charge_full", &full) < 0)
                return -1;

            if (FileGetInt(BATTERY_SYSFS_PATH "charge_now", &now) < 0)
                return -1;

            capacity = (now / full);
        }
        else
        {
            return -1;
        }
    }

    return capacity;
}
/**
 * @brief Read battery temperature
 *
 * @retval Battery temperature (integer)
 */
int battery_temperature(void)
{
    int temp;

    if (!g_file_test(BATTERY_SYSFS_PATH "temp", G_FILE_TEST_EXISTS) ||
        FileGetInt(BATTERY_SYSFS_PATH "temp", &temp) < 0)
        return -1;

    return temp;
}

/**
 * @brief Read battery voltage
 *
 * @retval Battery voltage (integer)
 */

int battery_voltage(void)
{
    int voltage;

    if (!g_file_test(BATTERY_SYSFS_PATH "voltage_now", G_FILE_TEST_EXISTS) ||
        FileGetInt(BATTERY_SYSFS_PATH "voltage_now", &voltage) < 0)
        return -1;

    return voltage;
}

/**
 * @brief Read the amount of current being drawn by the battery.
 *
 * @retval Current (integer)
 */
int battery_current(void)
{
    return -1;
}

/**
 * @brief Read average current being drawn by the battery.
 *
 * @retval Current (integer)
 */

int battery_avg_current(void)
{
    return -1;
}

/**
 * @brief Read battery full capacity
 *
 * @retval Battery capacity (double)
 */
double battery_full40(void)
{
    int energy_full;

    if (g_file_test(BATTERY_SYSFS_PATH "energy_full", G_FILE_TEST_EXISTS) &&
        FileGetInt(BATTERY_SYSFS_PATH "energy_full", &energy_full) < 0)
        return -1;

   return (double) energy_full;
}

/**
 * @brief Read battery current raw capacity
 *
 * @retval Battery capacity (double)
 */

double battery_rawcoulomb(void)
{
    return -1;
}

/**
 * @brief Read battery current capacity
 *
 * @retval Battery capacity (double)
 */

double battery_coulomb(void)
{
    return -1;
}

/**
 * @brief Read battery age
 *
 * @retval Battery age (double)
 */
double battery_age(void)
{
    return -1;
}


bool battery_is_present(void)
{
    int present;

    /* We can either take present or online property into account */
    if (!g_file_test(BATTERY_SYSFS_PATH "present", G_FILE_TEST_EXISTS) ||
        FileGetInt(BATTERY_SYSFS_PATH "present", &present) < 0)
    {
        if (!g_file_test(BATTERY_SYSFS_PATH "online", G_FILE_TEST_EXISTS) ||
            FileGetInt(BATTERY_SYSFS_PATH "online", &present) < 0)
        {
            return -1;
        }
    }

    return present == 1 ? true : false;
}

gboolean _handle_event(GIOChannel *channel, GIOCondition condition, gpointer data)
{
    struct udev_device *dev;

    if ((condition  & G_IO_IN) == G_IO_IN) {
        dev = udev_monitor_receive_device(mon);
        if (dev) {
            battery_callback(nyxDev, NYX_CALLBACK_STATUS_DONE, battery_callback_context);
        }
    }

    return TRUE;
}

void battery_read_init(void)
{
    int fd;

    udev = udev_new();
    if (!udev) {
        nyx_error("Could not initialize udev component; battery status updates will not be available");
        return;
    }

    mon = udev_monitor_new_from_netlink(udev, "kernel");
    udev_monitor_filter_add_match_subsystem_devtype(mon, "power_supply", NULL);
    udev_monitor_enable_receiving(mon);
    fd = udev_monitor_get_fd(mon);

    channel = g_io_channel_unix_new(fd);
    g_io_add_watch(channel, G_IO_IN | G_IO_HUP | G_IO_NVAL, _handle_event, NULL);
}

bool battery_is_authenticated(const char *pair_challenge, const char *pair_response)
{
    /* not supported */
    return true;
}

bool battery_authenticate(void)
{
    /* not supported */
    return true;
}

void battery_set_wakeup_percent(int percentage)
{
    /* no supported */
    return;
}

